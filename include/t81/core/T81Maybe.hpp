/**
 * @file T81Maybe.hpp
 * @brief T81Maybe — ternary-native optional / Maybe type.
 *
 * This is a thin wrapper around std::optional<T> with a small, focused API:
 *   • Presence/absence: has_value(), operator bool(), value(), value_or(...)
 *   • Constructors: default (empty), from value, just()/nothing() helpers
 *   • Combinators: map(F), and_then(F) for monadic chaining
 *
 * The original thermodynamic / reflection wiring (T81Entropy, T81Time,
 * T81Reflection, etc.) has been intentionally decoupled from this core
 * container so that it can be used freely in core code and tests without
 * pulling in the entire cognitive stack.
 */

#pragma once

#include <cstddef>   // std::nullptr_t
#include <optional>
#include <utility>
#include <functional>
#include <type_traits>

namespace t81 {

template <typename T>
class T81Maybe {
public:
    using value_type = T;

private:
    std::optional<T> value_;

public:
    //===================================================================
    // Construction
    //===================================================================

    // Default: nothing
    constexpr T81Maybe() noexcept = default;

    // Explicit "nothing" from std::nullopt_t
    constexpr T81Maybe(std::nullopt_t) noexcept
        : value_(std::nullopt) {}

    // Explicit "nothing" from nullptr (for legacy/tests)
    constexpr T81Maybe(std::nullptr_t) noexcept
        : value_(std::nullopt) {}

    // From const value
    constexpr T81Maybe(const T& v)
        : value_(v) {}

    // From rvalue value
    constexpr T81Maybe(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>)
        : value_(std::move(v)) {}

    // Factory helpers
    static constexpr T81Maybe just(const T& v) {
        return T81Maybe(v);
    }

    static constexpr T81Maybe just(T&& v) {
        return T81Maybe(std::move(v));
    }

    // Plain "nothing" (no reason)
    static constexpr T81Maybe nothing() noexcept {
        return T81Maybe(std::nullopt);
    }

    // "Nothing with reason" overload – reason currently ignored but
    // preserved in the signature for tests / future extension.
    template <typename Reason>
    static constexpr T81Maybe nothing(Reason&&) noexcept {
        return T81Maybe(std::nullopt);
    }

    //===================================================================
    // Observers
    //===================================================================

    [[nodiscard]] constexpr bool has_value() const noexcept {
        return value_.has_value();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] constexpr T& value() & {
        return *value_;
    }

    [[nodiscard]] constexpr const T& value() const & {
        return *value_;
    }

    [[nodiscard]] constexpr T&& value() && {
        return std::move(*value_);
    }

    [[nodiscard]] constexpr const T&& value() const && {
        return std::move(*value_);
    }

    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& fallback) const {
        if (value_) {
            return *value_;
        }
        return static_cast<T>(std::forward<U>(fallback));
    }

    //===================================================================
    // Combinators
    //===================================================================

    // map: T -> U, returns T81Maybe<U>
    template <typename F>
    [[nodiscard]] auto map(F&& f) const
        -> T81Maybe<std::invoke_result_t<F, const T&>>
    {
        using U = std::invoke_result_t<F, const T&>;
        if (!value_) {
            return T81Maybe<U>::nothing();
        }
        return T81Maybe<U>::just(std::invoke(std::forward<F>(f), *value_));
    }

    // and_then: T -> T81Maybe<U>, keeps the monadic structure
    template <typename F>
    [[nodiscard]] auto and_then(F&& f) const
        -> std::invoke_result_t<F, const T&>
    {
        using R = std::invoke_result_t<F, const T&>;
        if (!value_) {
            // Assume R is a Maybe-like type with static nothing()
            return R::nothing();
        }
        return std::invoke(std::forward<F>(f), *value_);
    }

    //===================================================================
    // Comparison
    //===================================================================

    [[nodiscard]] constexpr auto operator<=>(const T81Maybe&) const = default;
    [[nodiscard]] constexpr bool operator==(const T81Maybe&) const   = default;
};

// ======================================================================
// Free helpers – functional style
// ======================================================================

template <typename T>
[[nodiscard]] constexpr T81Maybe<T> just(T value) {
    return T81Maybe<T>::just(std::move(value));
}

template <typename T>
[[nodiscard]] constexpr T81Maybe<T> nothing() {
    return T81Maybe<T>::nothing();
}

} // namespace t81
