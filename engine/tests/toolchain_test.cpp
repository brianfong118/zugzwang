#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>

// The bitboard representation assumes exactly 64 bits and constant-evaluable <bit> operations.
TEST_CASE("toolchain provides 64-bit bitboards and <bit> operations", "[toolchain]") {
    STATIC_REQUIRE(sizeof(std::uint64_t) * 8 == 64);
    STATIC_REQUIRE(std::popcount(std::uint64_t{0xFF}) == 8);
    STATIC_REQUIRE(std::countr_zero(std::uint64_t{1} << 42) == 42);
}
