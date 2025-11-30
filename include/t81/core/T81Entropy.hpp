/**
 * @file T81Entropy.hpp
 * @brief Provenanced, move-only entropy tokens for thermodynamic computing.
 */
#pragma once
#include "t81/core/T81Int.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstdint>
#include <compare>
#include <utility>

namespace t81 {

class T81Entropy {
public:
    using Raw = T81Int<81>;  // ← moved to public so EntropyPool can see it

private:
    Raw entropy_;
    T81Symbol source_;
    uint64_t sequence_;
    mutable bool consumed_ = false;

    friend class EntropyPool;

    constexpr T81Entropy(T81Symbol src, uint64_t seq, Raw raw) noexcept
        : entropy_(std::move(raw)), source_(src), sequence_(seq) {}

public:
    T81Entropy() noexcept = delete;
    T81Entropy(const T81Entropy&) = delete;
    T81Entropy& operator=(const T81Entropy&) = delete;

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
            entropy_ = std::move(o.entropy_);
            source_ = o.source_;
            sequence_ = o.sequence_;
            consumed_ = o.consumed_;
            o.consumed_ = true;
        }
        return *this;
    }

    [[nodiscard]] Raw consume() const noexcept {
        if (consumed_) std::terminate();
        consumed_ = true;
        return entropy_;
    }

    [[nodiscard]] constexpr T81Symbol source() const noexcept { return source_; }
    [[nodiscard]] constexpr uint64_t  sequence() const noexcept { return sequence_; }
    [[nodiscard]] constexpr bool      is_consumed() const noexcept { return consumed_; }
    [[nodiscard]] constexpr Raw       value() const noexcept { return entropy_; }

    [[nodiscard]] constexpr auto operator<=>(const T81Entropy& o) const noexcept {
        if (source_ != o.source_) return source_ <=> o.source_;
        return sequence_ <=> o.sequence_;
    }

    [[nodiscard]] constexpr bool operator==(const T81Entropy& o) const noexcept = default;
};

// ======================================================================
// Global entropy pool
// ======================================================================
class EntropyPool {
    alignas(64) std::atomic<uint64_t> counter_{0};

    static T81Entropy::Raw hardware_trng() noexcept;

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

inline T81Entropy::Raw EntropyPool::hardware_trng() noexcept {
    static uint64_t x = 0x517cc1b727220a95ULL;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return T81Int<81>(static_cast<std::int64_t>(x));
}

inline T81Entropy acquire_entropy(T81Symbol who = T81Symbol::intern("KERNEL")) {
    return EntropyPool::global().acquire(who);
}

} // namespace t81
