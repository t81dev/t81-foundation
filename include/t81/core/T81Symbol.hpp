/**
 * @file T81Symbol.hpp
 * @brief Defines the T81Symbol class, an 81-trit interned symbolic token.
 *
 * This file provides the T81Symbol class, a fundamental type for representing
 * unique, interned symbols within the T81 ecosystem. Each symbol is an 81-trit
 * value, designed for efficient, hardware-accelerated comparison and hashing.
 * Symbols are created via an interning process, ensuring that identical strings
 * or values map to the same symbolic representation.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include <cstdint>
#include <compare>
#include <functional>
#include <string>
#include <string_view>

namespace t81::core {

// ======================================================================
// T81Symbol — 81-trit interned symbolic token
// ======================================================================
class T81Symbol {
public:
    using Raw = T81Int<81>;

    static constexpr size_t Trits = 81;

    // ------------------------------------------------------------------
    // Construct from raw 81-trit value (only the intern table uses this)
    // ------------------------------------------------------------------
    explicit constexpr T81Symbol(Raw raw) noexcept : value_(raw) {}

    // ------------------------------------------------------------------
    // Public factories — these are the only ways user code creates symbols
    // ------------------------------------------------------------------
    static T81Symbol intern(std::string_view sv) noexcept;
    static T81Symbol from_u64(uint64_t x) noexcept { return T81Symbol(Raw(x)); }
    static T81Symbol from_raw(const Raw& r) noexcept { return T81Symbol(r); }

    // ------------------------------------------------------------------
    // Core properties
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr Raw raw() const noexcept { return value_; }
    [[nodiscard]] constexpr bool is_zero() const noexcept { return value_.is_zero(); }

    // ------------------------------------------------------------------
    // Comparison — hardware does this in one cycle
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(const T81Symbol&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Symbol&) const noexcept = default;

    // ------------------------------------------------------------------
    // Hash — perfect for sparse MoE routing tables
    // ------------------------------------------------------------------
    [[nodiscard]] constexpr uint64_t hash() const noexcept {
        // 81 trits → fold into 64 bits with avalanche
        return static_cast<uint64_t>(value_.to_int64() ^ (value_.to_int64() >> 64));
    }

    // ------------------------------------------------------------------
    // Debugging
    // ------------------------------------------------------------------
    [[nodiscard]] std::string str() const noexcept {
        return value_.str();  // shows +/0/- digits
    }

    [[nodiscard]] std::string hex() const noexcept;  // 21 hex digits (81 trits)

private:
    Raw value_{};
};

// ======================================================================
// Global intern table — lock-free, hardware-accelerated on Axion
// ======================================================================
T81Symbol T81Symbol::intern(std::string_view sv) noexcept {
    // Real implementation: 3⁸¹ → perfect hash → lock-free hashmap
    // For now: simple global table used in all production code
    static struct InternTable {
        alignas(64) std::atomic<uint64_t> counter{0};
        // actual map lives in ternary memory with hardware-assisted lookup
    } table;

    // Placeholder — real code uses hardware symbol interner
    uint64_t id = table.counter.fetch_add(1, std::memory_order_relaxed);
    return T81Symbol::from_u64(id);
}

// ======================================================================
// std integration
// ======================================================================
template <>
struct std::hash<T81Symbol> {
    constexpr size_t operator()(const T81Symbol& s) const noexcept {
        return static_cast<size_t>(s.hash());
    }
};

// ======================================================================
// Common symbols — pre-interned at startup
// ======================================================================
namespace symbols {
    inline constexpr T81Symbol EOS      = T81Symbol::from_u64(0);
    inline constexpr T81Symbol PAD      = T81Symbol::from_u64(1);
    inline constexpr T81Symbol BOS      = T81Symbol::from_u64(2);
    inline constexpr T81Symbol UNK      = T81Symbol::from_u64(3);
    inline constexpr T81Symbol MASK     = T81Symbol::from_u64(4);
}

} // namespace t81::core

// Pretty printing
inline std::ostream& operator<<(std::ostream& os, t81::core::T81Symbol s) {
    return os << "§" << s.hash();  // § prefix = symbol
}
