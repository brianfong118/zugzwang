#pragma once

#include "types.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace zugzwang {

// Exact sets both bits, so a test for either bound also accepts an exact score.
enum Bound : std::uint8_t { kNoBound, kUpperBound, kLowerBound, kExactBound };

inline constexpr std::size_t kHashMegabytes = 64;

// One entry per slot, replaced by every store.
class TranspositionTable {
  public:
    struct Entry {
        Key key = 0;
        std::int16_t score = 0;
        std::uint8_t depth = 0;
        Bound bound = kNoBound;
    };
    // Four entries fill a cache line exactly, so no probe straddles two.
    static_assert(sizeof(Entry) == 16);

    // Rounded down to a power of two entries, so the slot is the low bits of the key.
    explicit TranspositionTable(std::size_t megabytes)
        : entries_(std::bit_floor(megabytes * 1024 * 1024 / sizeof(Entry))) {}

    // The full key is stored, so positions that only share a slot are never confused.
    std::optional<Entry> probe(Key key) const {
        const Entry& entry = entries_[index(key)];
        if (entry.key != key) return std::nullopt;
        return entry;
    }

    void store(Key key, int score, int depth, Bound bound) {
        entries_[index(key)] = {key, std::int16_t(score), std::uint8_t(depth), bound};
    }

    void clear() { std::ranges::fill(entries_, Entry{}); }

  private:
    std::size_t index(Key key) const { return key & (entries_.size() - 1); }

    std::vector<Entry> entries_;
};

} // namespace zugzwang
