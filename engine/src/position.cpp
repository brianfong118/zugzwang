#include "position.h"

#include "attacks.h"
#include "zobrist.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <charconv>
#include <cstdlib>

namespace zugzwang {

namespace {

constexpr std::string_view kPieceChars = "PNBRQKpnbrqk";
// The character at index i stands for the castling right 1 << i.
constexpr std::string_view kCastlingChars = "KQkq";

// A move from or onto one of these squares ends the rights that need a piece to stay there.
constexpr std::array<std::uint8_t, 64> kRightsLost = [] {
    std::array<std::uint8_t, 64> lost{};
    for (const auto& rules : kCastlingRules) {
        for (const CastlingRule& rule : rules) {
            lost[rule.kingFrom] |= rule.right;
            lost[rule.rookFrom] |= rule.right;
        }
    }
    return lost;
}();

constexpr const CastlingRule& castlingRule(Color c, Square kingTo) {
    const auto& rules = kCastlingRules[c];
    return rules[0].kingTo == kingTo ? rules[0] : rules[1];
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

// Far beyond any real game, and leaves make room to keep counting without overflow.
constexpr int kMaxFenCounter = 1'000'000;

bool parseCounter(std::string_view s, int& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size() && out >= 0 && out <= kMaxFenCounter;
}

} // namespace

void Position::putPiece(Piece p, Square s) {
    byPiece_[p] |= squareBb(s);
    byColor_[colorOf(p)] |= squareBb(s);
    board_[s] = p;
    key_ ^= zobrist::piece(p, s);
}

void Position::removePiece(Square s) {
    Piece p = board_[s];
    byPiece_[p] &= ~squareBb(s);
    byColor_[colorOf(p)] &= ~squareBb(s);
    board_[s] = kNoPiece;
    key_ ^= zobrist::piece(p, s);
}

Key Position::epKey() const {
    bool capturable = epSquare_ != kNoSquare &&
                      (attacks::pawn(~sideToMove_, epSquare_) & pieces(sideToMove_, kPawn));
    return capturable ? zobrist::enPassant(fileOf(epSquare_)) : 0;
}

std::optional<Position> Position::fromFen(std::string_view fen) {
    std::array<std::string_view, 6> fields;
    int n = 0;
    for (std::size_t i = 0; i < fen.size();) {
        while (i < fen.size() && std::isspace(static_cast<unsigned char>(fen[i])))
            ++i;
        std::size_t start = i;
        while (i < fen.size() && !std::isspace(static_cast<unsigned char>(fen[i])))
            ++i;
        if (i > start) {
            if (n == 6) return std::nullopt;
            fields[n++] = fen.substr(start, i - start);
        }
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
            auto i = kCastlingChars.find(c);
            if (i == std::string_view::npos) return std::nullopt;
            pos.castling_ |= 1 << i;
        }
        // A right whose king or rook has left home is dropped rather than rejected.
        for (Color c : {kWhite, kBlack}) {
            for (const CastlingRule& rule : kCastlingRules[c]) {
                if (pos.pieceOn(rule.kingFrom) != makePiece(c, kKing) ||
                    pos.pieceOn(rule.rookFrom) != makePiece(c, kRook))
                    pos.castling_ &= ~rule.right;
            }
        }
    }

    if (n > 3 && fields[3] != "-") {
        std::string_view ep = fields[3];
        bool whiteToMove = pos.sideToMove_ == kWhite;
        int epRank = whiteToMove ? 5 : 2;
        if (ep.size() != 2 || ep[0] < 'a' || ep[0] > 'h' || ep[1] != '1' + epRank)
            return std::nullopt;
        int epFile = ep[0] - 'a';
        // The double push must have been possible: pawn landed, and the squares behind it empty.
        if (pos.pieceOn(makeSquare(epFile, whiteToMove ? 4 : 3)) !=
                makePiece(~pos.sideToMove_, kPawn) ||
            pos.pieceOn(makeSquare(epFile, epRank)) != kNoPiece ||
            pos.pieceOn(makeSquare(epFile, whiteToMove ? 6 : 1)) != kNoPiece)
            return std::nullopt;
        pos.epSquare_ = makeSquare(epFile, epRank);
    }

    if (n > 4 && !parseCounter(fields[4], pos.halfmoveClock_)) return std::nullopt;
    if (n > 5 && (!parseCounter(fields[5], pos.fullmoveNumber_) || pos.fullmoveNumber_ < 1))
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
    for (std::size_t i = 0; i < kCastlingChars.size(); ++i)
        if (castling_ & (1 << i)) out += kCastlingChars[i];
    if (!castling_) out += '-';
    out += ' ';
    out += epSquare_ == kNoSquare ? "-" : squareName(epSquare_);
    out += ' ' + std::to_string(halfmoveClock_) + ' ' + std::to_string(fullmoveNumber_);
    return out;
}

bool Position::isAttacked(Square s, Color by) const {
    Bitboard queens = pieces(by, kQueen);
    return (attacks::pawn(~by, s) & pieces(by, kPawn)) |
           (attacks::knight(s) & pieces(by, kKnight)) | (attacks::king(s) & pieces(by, kKing)) |
           (attacks::rook(s, occupied()) & (pieces(by, kRook) | queens)) |
           (attacks::bishop(s, occupied()) & (pieces(by, kBishop) | queens));
}

bool Position::isDraw(int ply) const {
    if (halfmoveClock_ >= 100) return true;

    // history_[n - i] is the position i plies ago; DESIGN.md says why only these i are scanned.
    int n = int(history_.size());
    int end = std::min(halfmoveClock_, n);
    int beforeRoot = 0;
    for (int i = 4; i <= end; i += 2)
        if (history_[n - i].key == key_ && (i < ply || ++beforeRoot == 2)) return true;
    return false;
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
        const CastlingRule& rule = castlingRule(us, to);
        removePiece(rule.rookFrom);
        putPiece(makePiece(us, kRook), rule.rookTo);
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
        const CastlingRule& rule = castlingRule(us, to);
        removePiece(rule.rookTo);
        putPiece(makePiece(us, kRook), rule.rookFrom);
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
