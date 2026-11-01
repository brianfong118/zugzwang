#include "position.h"

#include <bit>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace zugzwang {

namespace {

constexpr std::string_view kPieceChars = "PNBRQKpnbrqk";

// A move from or onto one of these squares ends the rights that need a piece to stay there.
constexpr std::array<std::uint8_t, 64> kRightsLost = [] {
    std::array<std::uint8_t, 64> lost{};
    lost[kE1] = kWhiteKingSide | kWhiteQueenSide;
    lost[kH1] = kWhiteKingSide;
    lost[kA1] = kWhiteQueenSide;
    lost[kE8] = kBlackKingSide | kBlackQueenSide;
    lost[kH8] = kBlackKingSide;
    lost[kA8] = kBlackQueenSide;
    return lost;
}();

struct RookMove {
    Square from, to;
};

constexpr RookMove castlingRook(Square kingTo) {
    int rank = rankOf(kingTo);
    return fileOf(kingTo) == 6 ? RookMove{makeSquare(7, rank), makeSquare(5, rank)}
                               : RookMove{makeSquare(0, rank), makeSquare(3, rank)};
}

// The square between the ends of a double push or a castling king move.
constexpr Square midpoint(Square a, Square b) {
    return Square((a + b) / 2);
}

// An en passant victim stands on the capture's target file and the capturer's starting rank.
constexpr Square epVictim(Move m) {
    return makeSquare(fileOf(m.to()), rankOf(m.from()));
}

Piece pieceFromChar(char c) {
    auto i = kPieceChars.find(c);
    return i == std::string_view::npos ? kNoPiece : Piece(i);
}

bool parseInt(std::string_view s, int& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size() && out >= 0;
}

} // namespace

std::optional<Position> Position::fromFen(std::string_view fen) {
    std::array<std::string_view, 6> fields;
    int n = 0;
    for (std::size_t i = 0; i < fen.size() && n < 6;) {
        while (i < fen.size() && std::isspace(static_cast<unsigned char>(fen[i])))
            ++i;
        std::size_t start = i;
        while (i < fen.size() && !std::isspace(static_cast<unsigned char>(fen[i])))
            ++i;
        if (i > start) fields[n++] = fen.substr(start, i - start);
    }
    if (n < 2) return std::nullopt;

    Position pos;
    int file = 0, rank = 7;
    for (char c : fields[0]) {
        if (c == '/') {
            if (file != 8 || rank == 0) return std::nullopt;
            file = 0;
            --rank;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return std::nullopt;
        } else {
            Piece p = pieceFromChar(c);
            if (p == kNoPiece || file > 7) return std::nullopt;
            pos.putPiece(p, makeSquare(file++, rank));
        }
    }
    if (file != 8 || rank != 0) return std::nullopt;
    if (std::popcount(pos.pieces(kWhiteKing)) != 1 || std::popcount(pos.pieces(kBlackKing)) != 1)
        return std::nullopt;
    if (std::popcount(pos.pieces(kWhite)) > 16 || std::popcount(pos.pieces(kBlack)) > 16)
        return std::nullopt;
    if (pos.pieces(kPawn) & (kRank1 | kRank8)) return std::nullopt;

    if (fields[1] == "w")
        pos.sideToMove_ = kWhite;
    else if (fields[1] == "b")
        pos.sideToMove_ = kBlack;
    else
        return std::nullopt;

    // Pseudo-legal generation would otherwise offer a king capture, and search assumes two kings.
    if (pos.isAttacked(pos.kingSquare(~pos.sideToMove_), pos.sideToMove_)) return std::nullopt;

    if (n > 2 && fields[2] != "-") {
        for (char c : fields[2]) {
            switch (c) {
            case 'K':
                pos.castling_ |= kWhiteKingSide;
                break;
            case 'Q':
                pos.castling_ |= kWhiteQueenSide;
                break;
            case 'k':
                pos.castling_ |= kBlackKingSide;
                break;
            case 'q':
                pos.castling_ |= kBlackQueenSide;
                break;
            default:
                return std::nullopt;
            }
        }
        // A right whose king or rook has left home is dropped rather than rejected.
        auto check = [&](CastlingRights right, Color c, Square king, Square rook) {
            if (pos.pieceOn(king) != makePiece(c, kKing) ||
                pos.pieceOn(rook) != makePiece(c, kRook))
                pos.castling_ &= ~right;
        };
        check(kWhiteKingSide, kWhite, kE1, kH1);
        check(kWhiteQueenSide, kWhite, kE1, kA1);
        check(kBlackKingSide, kBlack, kE8, kH8);
        check(kBlackQueenSide, kBlack, kE8, kA8);
    }

    if (n > 3 && fields[3] != "-") {
        std::string_view ep = fields[3];
        int epRank = pos.sideToMove_ == kWhite ? 5 : 2;
        if (ep.size() != 2 || ep[0] < 'a' || ep[0] > 'h' || ep[1] != '1' + epRank)
            return std::nullopt;
        Square s = makeSquare(ep[0] - 'a', epRank);
        Square pawnSquare = s + (pos.sideToMove_ == kWhite ? kSouth : kNorth);
        if (pos.pieceOn(s) != kNoPiece ||
            pos.pieceOn(pawnSquare) != makePiece(~pos.sideToMove_, kPawn))
            return std::nullopt;
        pos.epSquare_ = s;
    }

    if (n > 4 && !parseInt(fields[4], pos.halfmoveClock_)) return std::nullopt;
    if (n > 5 && (!parseInt(fields[5], pos.fullmoveNumber_) || pos.fullmoveNumber_ < 1))
        return std::nullopt;

    // putPiece has already folded in the piece keys.
    pos.key_ ^= zobrist::castling(pos.castling_) ^ pos.epKey();
    if (pos.sideToMove_ == kBlack) pos.key_ ^= zobrist::side();
    return pos;
}

std::string Position::fen() const {
    std::string out;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            Piece p = pieceOn(makeSquare(file, rank));
            if (p == kNoPiece) {
                ++empty;
                continue;
            }
            if (empty) out += char('0' + empty);
            empty = 0;
            out += kPieceChars[p];
        }
        if (empty) out += char('0' + empty);
        if (rank) out += '/';
    }

    out += sideToMove_ == kWhite ? " w " : " b ";
    if (castling_ & kWhiteKingSide) out += 'K';
    if (castling_ & kWhiteQueenSide) out += 'Q';
    if (castling_ & kBlackKingSide) out += 'k';
    if (castling_ & kBlackQueenSide) out += 'q';
    if (!castling_) out += '-';
    out += ' ';
    out += epSquare_ == kNoSquare ? "-" : squareName(epSquare_);
    out += ' ' + std::to_string(halfmoveClock_) + ' ' + std::to_string(fullmoveNumber_);
    return out;
}

Bitboard Position::attackersTo(Square s, Bitboard occupied) const {
    return (attacks::pawn(kBlack, s) & pieces(kWhitePawn)) |
           (attacks::pawn(kWhite, s) & pieces(kBlackPawn)) |
           (attacks::knight(s) & pieces(kKnight)) | (attacks::king(s) & pieces(kKing)) |
           (attacks::rook(s, occupied) & (pieces(kRook) | pieces(kQueen))) |
           (attacks::bishop(s, occupied) & (pieces(kBishop) | pieces(kQueen)));
}

bool Position::make(Move m) {
    Square from = m.from(), to = m.to();
    Color us = sideToMove_, them = ~us;

    // The king's landing square is covered by the check at the end, like any other king move.
    if (m.flag() == kCastling && (isAttacked(from, them) || isAttacked(midpoint(from, to), them)))
        return false;

    Piece moved = board_[from], captured = board_[to];
    history_.push_back({key_, halfmoveClock_, epSquare_, castling_, captured});

    // epKey depends on the side to move and the pawns, so it is removed before either changes.
    key_ ^= epKey() ^ zobrist::castling(castling_) ^ zobrist::side();

    if (captured != kNoPiece) removePiece(to);
    removePiece(from);
    putPiece(m.isPromotion() ? makePiece(us, m.promotionType()) : moved, to);
    if (m.flag() == kCastling) {
        auto [rookFrom, rookTo] = castlingRook(to);
        removePiece(rookFrom);
        putPiece(makePiece(us, kRook), rookTo);
    } else if (m.flag() == kEnPassant) {
        removePiece(epVictim(m));
    }

    bool pawnMove = typeOf(moved) == kPawn;
    halfmoveClock_ = pawnMove || captured != kNoPiece ? 0 : halfmoveClock_ + 1;
    if (us == kBlack) ++fullmoveNumber_;
    castling_ &= ~(kRightsLost[from] | kRightsLost[to]);
    epSquare_ = pawnMove && std::abs(to - from) == 16 ? midpoint(from, to) : kNoSquare;
    sideToMove_ = them;
    key_ ^= epKey() ^ zobrist::castling(castling_);

    if (isAttacked(kingSquare(us), them)) {
        unmake(m);
        return false;
    }
    return true;
}

void Position::unmake(Move m) {
    Square from = m.from(), to = m.to();
    Color us = ~sideToMove_;
    const Undo& undo = history_.back();

    if (m.flag() == kCastling) {
        auto [rookFrom, rookTo] = castlingRook(to);
        removePiece(rookTo);
        putPiece(makePiece(us, kRook), rookFrom);
    }
    Piece moved = m.isPromotion() ? makePiece(us, kPawn) : board_[to];
    removePiece(to);
    putPiece(moved, from);
    if (undo.captured != kNoPiece)
        putPiece(undo.captured, to);
    else if (m.flag() == kEnPassant)
        putPiece(makePiece(~us, kPawn), epVictim(m));

    if (us == kBlack) --fullmoveNumber_;
    sideToMove_ = us;
    halfmoveClock_ = undo.halfmoveClock;
    epSquare_ = undo.epSquare;
    castling_ = undo.castling;
    // Restored last because putPiece and removePiece above also changed it.
    key_ = undo.key;
    history_.pop_back();
}

Key Position::computeKey() const {
    Key k = 0;
    for (Bitboard b = occupied(); b;) {
        Square s = popLsb(b);
        k ^= zobrist::piece(pieceOn(s), s);
    }
    k ^= zobrist::castling(castling_) ^ epKey();
    if (sideToMove_ == kBlack) k ^= zobrist::side();
    return k;
}

} // namespace zugzwang
