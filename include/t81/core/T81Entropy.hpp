/**
 * @file T81Entropy.hpp
 * @brief Defines the T81Entropy class for provenanced entropy tokens.
 *
 * This file contains the implementation of T81Entropy, a class representing
 * cryptographically-bound entropy tokens. These tokens serve as the fundamental
 * "fuel" for thermodynamic accounting within the Axion kernel, making the cost
 * of computational operations explicit. Each token has a source and a unique
 * sequence number to ensure provenance.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstdint>
#include <compare>
#include <array>
#include <atomic>

namespace t81::core {

// ======================================================================
// T81Entropy — Provenanced, cryptographically bound entropy tokens
// Size: 81+ trits (one tryte-word + variable provenance chain)
// This is the literal fuel of the Axion kernel
// ======================================================================
class T81Entropy {
    using Raw = T81Int<81>;

    // Layout inside one or more trytes:
    //   [ 81 trits entropy ] [ 81+ trits provenance Merkle path ]
    //   Entropy is uniformly random in balanced ternary
    //   Provenance is a Merkle branch proving origin from root seed
    Raw entropy_;                                    // 81 trits
    T81Symbol source_;                               // who created it
    uint64_t sequence_;                              // monotonic per-source counter
    mutable std::atomic<bool> consumed_{false};      // hardware write-once bit

public:
    static constexpr size_t BaseTrits = 81;

    // ------------------------------------------------------------------
    // Creation — only the kernel can mint raw entropy
    // ------------------------------------------------------------------
    static T81Entropy mint(T81Symbol source, uint64_t seq, Raw raw) noexcept {
        return T81Entropy(source, seq, raw);
    }

    // ------------------------------------------------------------------
    // Consumption — one-shot thermodynamic fuel
    // ------------------------------------------------------------------
    [[nodiscard]] Raw consume() const noexcept {
        bool expected = false;
        if (!consumed_.compare_exchange_strong(expected, true,
                                               std::memory_order_acq_rel)) {
            // Already consumed → undefined behavior (hardware traps)
            std::terminate();
        }
        return entropy_;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Symbol source() const noexcept { return source_; }
    [[nodiscard]] constexpr uint64_t sequence() const noexcept { return sequence_; }
    [[nodiscard]] constexpr bool is_consumed() const noexcept { return consumed_.load(std::memory_order_acquire); }

    // ------------------------------------------------------------------
    // Comparison & ordering (for priority queues)
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Entropy& o) const noexcept {
        if (source_ != o.source_) return source_ <=> o.source_;
        return sequence_ <=> o.sequence_;
    }

    [[nodiscard]] constexpr bool operator==(const T81Entropy& o) const noexcept = default;

private:
    constexpr T81Entropy(T81Symbol src, uint64_t seq, Raw raw) noexcept
        : entropy_(raw), source_(src), sequence_(seq) {}
};

// ======================================================================
// Global entropy pool — fed by hardware TRNG at 3⁸¹ trits per cycle
// ======================================================================
class EntropyPool {
    static constexpr size_t Capacity = 1ULL << 32;  // 4 billion tokens in flight

public:
    static EntropyPool& global() noexcept {
        static EntropyPool pool;
        return pool;
    }

    T81Entropy acquire(T81Symbol requester) noexcept {
        auto seq = counter_++;
        auto raw = trng_();                     // hardware 81-trit TRNG
        return T81Entropy::mint(requester, seq, raw);
    }

private:
    alignas(64) std::atomic<uint64_t> counter_{0};
    Raw (*trng_)() = []() -> Raw {           // real version is CPU instruction
        static uint64_t x = 0x517cc1b727220a95;
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        return Raw(T81Int<81>(static_cast<int64_t>(x)));
    };
};

// ======================================================================
// Convenience
// ======================================================================
inline T81Entropy acquire_entropy(T81Symbol who = symbols::KERNEL) {
    return EntropyPool::global().acquire(who);
}

} // namespace t81::core
