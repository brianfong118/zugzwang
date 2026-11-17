#include "tt.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>

using namespace zugzwang;

namespace {

constexpr Key kKey = 0x9e3779b97f4a7c15;
// 1 MB holds 2^16 entries, so keys this far apart share a slot.
constexpr Key kSlotStride = (1 << 20) / sizeof(TranspositionTable::Entry);

} // namespace

TEST_CASE("a probe returns what was stored under the same key", "[tt]") {
    TranspositionTable tt(1);
    tt.store(kKey, -31990, 7, kLowerBound);
    std::optional<TranspositionTable::Entry> entry = tt.probe(kKey);
    REQUIRE(entry);
    CHECK(entry->score == -31990);
    CHECK(entry->depth == 7);
    CHECK(entry->bound == kLowerBound);
    CHECK_FALSE(tt.probe(kKey + 1));
}

TEST_CASE("a key that only shares the slot misses, and storing it replaces the entry", "[tt]") {
    TranspositionTable tt(1);
    tt.store(kKey, 50, 3, kExactBound);
    CHECK_FALSE(tt.probe(kKey + kSlotStride));

    tt.store(kKey + kSlotStride, -50, 1, kUpperBound);
    CHECK_FALSE(tt.probe(kKey));
    std::optional<TranspositionTable::Entry> entry = tt.probe(kKey + kSlotStride);
    REQUIRE(entry);
    CHECK(entry->score == -50);
}

TEST_CASE("clear empties the table", "[tt]") {
    TranspositionTable tt(1);
    tt.store(kKey, 50, 3, kExactBound);
    tt.clear();
    CHECK_FALSE(tt.probe(kKey));
}
