#pragma once

#include <cstdint>

namespace zugzwang {

// SplitMix64: constexpr, so Zobrist keys are compile-time constants and magics are reproducible.
class Prng {
  public:
    constexpr explicit Prng(std::uint64_t seed) : state_(seed) {}

    constexpr std::uint64_t next() {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Magic multipliers with few set bits collide less, so candidates are ANDed together.
    constexpr std::uint64_t sparse() { return next() & next() & next(); }

  private:
    std::uint64_t state_;
};

} // namespace zugzwang
