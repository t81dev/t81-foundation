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

class T81Entropy {
    using Raw = T81Int<81>;

    Raw                 entropy_;    // 81 trits
    T81Symbol           source_;     // who created it
    uint64_t            sequence_;   // monotonic per-source counter
    mutable std::atomic<bool> consumed_{false};  // hardware write-once bit

    // Only EntropyPool may mint
    friend class EntropyPool;

    constexpr T81Entropy(T81Symbol src, uint64_t seq, Raw raw) noexcept
        : entropy_(raw), source_(src), sequence_(seq) {}

public:
    T81Entropy() noexcept = delete;
    T81Entropy(const T81Entropy&) = delete;
    T81Entropy& operator=(const T81Entropy&) = delete;

    // Move-only — entropy flows, never duplicates
    constexpr T81Entropy(T81Entropy&& o) noexcept
        : entropy_(o.entropy_)
        , source_(o.source_)
        , sequence_(o.sequence_)
        , consumed_(o.consumed_.load(std::memory_order_relaxed))
    {
        o.consumed_.store(true, std::memory_order_relaxed);
    }

    constexpr T81Entropy& operator=(T81Entropy&& o) noexcept {
        if (this != &o) {
            this->consume();  // old value is destroyed
            entropy_   = o.entropy_;
            source_    = o.source_;
            sequence_  = o.sequence_;
            consumed_.store(o.consumed_.load(std::memory_order_relaxed));
            o.consumed_.store(true, std::memory_order_relaxed);
        }
        return *this;
    }

    // ------------------------------------------------------------------
    // Consumption — one-shot thermodynamic fuel
    // ------------------------------------------------------------------
    [[nodiscard]] Raw consume() const noexcept {
        bool expected = false;
        if (!consumed_.compare_exchange_strong(expected, true,
                                               std::memory_order_acq_rel,
                                               std::memory_order_relaxed)) {
            std::terminate();  // double-spend = death
        }
        return entropy_;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Symbol source() const noexcept { return source_; }
    [[nodiscard]] constexpr uint64_t  sequence() const noexcept { return sequence_; }
    [[nodiscard]] constexpr bool      is_consumed() const noexcept {
        return consumed_.load(std::memory_order_acquire);
    }

    // ------------------------------------------------------------------
    // Comparison & ordering (for priority queues)
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Entropy& o) const noexcept {
        if (source_ != o.source_) return source_ <=> o.source_;
        return sequence_ <=> o.sequence_;
    }
    [[nodiscard]] constexpr bool operator==(const T81Entropy& o) const noexcept = default;
};

// ======================================================================
// Global entropy pool — fed by hardware TRNG at 3⁸¹ trits per cycle
// ======================================================================
class EntropyPool {
    alignas(64) std::atomic<uint64_t> counter_{0};

    static Raw hardware_trng() noexcept;  // real version = CPU instruction

public:
    static EntropyPool& global() noexcept {
        static EntropyPool pool;
        return pool;
    }

    T81Entropy acquire(T81Symbol requester) noexcept {
        auto seq = counter_.fetch_add(1, std::memory_order_relaxed);
        auto raw = hardware_trng();
        return T81Entropy(requester, seq, raw);
    }
};

inline T81Entropy::Raw EntropyPool::hardware_trng() noexcept {
    static uint64_t x = 0x517cc1b727220a95;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return T81Entropy::Raw(static_cast<int64_t>(x));
}

// ======================================================================
// Convenience
// ======================================================================
inline T81Entropy acquire_entropy(T81Symbol who = symbols::KERNEL) {
    return EntropyPool::global().acquire(who);
}

} // namespace t81::core
