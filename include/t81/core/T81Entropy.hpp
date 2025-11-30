/**
 * @file T81Entropy.hpp
 * @brief Provenanced entropy tokens for thermodynamic accounting.
 */
#pragma once
#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstdint>
#include <compare>
#include <array>
#include <utility>

namespace t81 {

class T81Entropy {
    using Raw = T81Int<81>;

    Raw entropy_;          // 81-trit entropy payload
    T81Symbol source_;     // who created it
    uint64_t sequence_;    // monotonic counter per source
    mutable bool consumed_ = false;  // plain bool — works in constexpr

    // Only EntropyPool may mint
    friend class EntropyPool;

    constexpr T81Entropy(T81Symbol src, uint64_t seq, Raw raw) noexcept
        : entropy_(std::move(raw)), source_(src), sequence_(seq) {}

public:
    T81Entropy() noexcept = delete;
    T81Entropy(const T81Entropy&) = delete;
    T81Entropy& operator=(const T81Entropy&) = delete;

    // Move-only — entropy flows, never duplicates
    constexpr T81Entropy(T81Entropy&& o) noexcept
        : entropy_(std::move(o.entropy_))
        , source_(o.source_)
        , sequence_(o.sequence_)
        , consumed_(o.consumed_)
    {
        o.consumed_ = true;
    }

    constexpr T81Entropy& operator=(T81Entropy&& o) noexcept {
        if (this != &o) {
            entropy_   = std::move(o.entropy_);
            source_    = o.source_;
            sequence_  = o.sequence_;
            consumed_  = o.consumed_;
            o.consumed_ = true;
        }
        return *this;
    }

    // ------------------------------------------------------------------
    // One-shot consumption — thermodynamic fuel
    // ------------------------------------------------------------------
    [[nodiscard]] Raw consume() const noexcept {
        if (consumed_) {
            std::terminate(); // double-spend = instant death
        }
        consumed_ = true;
        return entropy_;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr T81Symbol source()   const noexcept { return source_; }
    [[nodiscard]] constexpr uint64_t  sequence() const noexcept { return sequence_; }
    [[nodiscard]] constexpr bool      is_consumed() const noexcept { return consumed_; }
    [[nodiscard]] constexpr Raw       value()     const noexcept { return entropy_; }

    // ------------------------------------------------------------------
    // Comparison (for priority queues, maps, etc.)
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Entropy& o) const noexcept {
        if (source_ != o.source_) return source_ <=> o.source_;
        return sequence_ <=> o.sequence_;
    }

    [[nodiscard]] constexpr bool operator==(const T81Entropy& o) const noexcept = default;
};

// ======================================================================
// Global entropy pool — deterministic fallback for tests, real TRNG in prod
// ======================================================================
class EntropyPool {
    alignas(64) std::atomic<uint64_t> counter_{0};

    static Raw hardware_trng() noexcept; // real version uses CPU RNG

public:
    static EntropyPool& global() noexcept {
        static EntropyPool pool;
        return pool;
    }

    T81Entropy acquire(T81Symbol requester = T81Symbol::intern("KERNEL")) noexcept {
        auto seq = counter_.fetch_add(1, std::memory_order_relaxed);
        auto raw = hardware_trng();
        return T81Entropy(requester, seq, std::move(raw));
    }
};

// Deterministic fallback — perfect for tests and constexpr contexts
inline T81Entropy::Raw EntropyPool::hardware_trng() noexcept {
    static uint64_t x = 0x517cc1b727220a95ULL; // FxHash constant
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return T81Int<81>(static_cast<std::int64_t>(x));
}

// ======================================================================
// Convenience
// ======================================================================
inline T81Entropy acquire_entropy(T81Symbol who = T81Symbol::intern("KERNEL")) {
    return EntropyPool::global().acquire(who);
}

} // namespace t81
