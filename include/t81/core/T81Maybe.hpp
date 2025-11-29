/**
 * @file T81Maybe.hpp
 * @brief Defines the T81Maybe class, a reflective, entropy-aware optional value.
 *
 * This file provides the T81Maybe<T> class, which represents an optional value.
 * Beyond indicating the presence or absence of a value, it is designed to be
 * reflective and entropy-aware. It can store a symbolic reason for an absence
 * and timestamps each observation, integrating with the library's principles of
 * explicit, auditable computation.
 */
#pragma once

#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Reflection.hpp"
#include "t81/core/T81Time.hpp"
#include "t81/core/T81Entropy.hpp"
#include <variant>
#include <optional>

namespace t81 {

// ======================================================================
// T81Maybe<T> – The sacred container of possibility and absence
// ======================================================================
template <typename T>
class T81Maybe {
    std::variant<std::monostate, T> storage_;
    T81Symbol reason_{symbols::unk};           // why is it empty?
    T81Time   observed_at_;                        // when did we last look?
    mutable std::optional<T81Entropy> last_check_fuel_;           // proof we paid attention

public:
    using value_type = T;

    //===================================================================
    // The three sacred states
    //===================================================================
    constexpr T81Maybe() noexcept                               // Nothing, and we never cared
        : storage_(std::monostate{}), observed_at_(T81Time::genesis()) {}

    constexpr T81Maybe(std::nullptr_t) noexcept                 // Explicit absence
        : T81Maybe() {}

    constexpr T81Maybe(const T& value) noexcept                 // Something real
        : storage_(value), observed_at_(T81Time::now(acquire_entropy(), T81Symbol::intern("JUSTIFICATION"))) {}

    constexpr T81Maybe(T&& value) noexcept
        : storage_(std::move(value)), observed_at_(T81Time::now(acquire_entropy(), T81Symbol::intern("JUSTIFICATION"))) {}

    // Absence with explanation
    static constexpr T81Maybe<T> nothing(T81Symbol because) noexcept {
        T81Maybe m;
        m.reason_ = because;
        m.observed_at_ = T81Time::now(acquire_entropy(), T81Symbol::intern("ABSENCE_RECORDED"));
        return m;
    }

    //===================================================================
    // Introspection – the most important part
    //===================================================================
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] constexpr bool is_nothing() const noexcept {
        return std::holds_alternative<std::monostate>(storage_);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] constexpr const T& value() const& {
        if (!has_value()) {
            throw std::logic_error("T81Maybe: attempted to access nothing (" + reason_.to_string() + ")");
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] constexpr T& value() & {
        if (!has_value()) {
            throw std::logic_error("T81Maybe: attempted to access nothing");
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] constexpr T value_or(const T& fallback) const& noexcept {
        return has_value() ? std::get<T>(storage_) : fallback;
    }

    [[nodiscard]] constexpr T value_or(T&& fallback) && noexcept {
        return has_value() ? std::move(std::get<T>(storage_)) : std::move(fallback);
    }

    //===================================================================
    // Monadic operations – pure, map, and_then, or_else
    //===================================================================
    template <typename F>
    [[nodiscard]] constexpr auto map(F&& f) const& {
        if (has_value()) {
            using U = std::invoke_result_t<F, const T&>;
            return T81Maybe<U>(f(std::get<T>(storage_)));
        }
        return T81Maybe<std::invoke_result_t<F, const T&>>::nothing(reason_);
    }

    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& f) const& {
        if (has_value()) {
            return f(std::get<T>(storage_));
        }
        return std::invoke_result_t<F, const T&>::nothing(reason_);
    }

    template <typename F>
    [[nodiscard]] constexpr T81Maybe or_else(F&& f) const& {
        if (has_value()) {
            return *this;
        }
        return f();
    }

    //===================================================================
    // Reflection – absence is not ignorance, it is knowledge
    //===================================================================
    [[nodiscard]] constexpr T81Symbol why() const noexcept { return reason_; }

    [[nodiscard]] constexpr T81Time when_observed() const noexcept { return observed_at_; }

    [[nodiscard]] T81Reflection<T81Maybe<T>> reflect() const {
        auto name = has_value() ? T81Symbol::intern("PRESENCE") : T81Symbol::intern("ABSENCE");
        return T81Reflection<T81Maybe<T>>(*this, T81Symbol::intern("MAYBE"), name);
    }

    //===================================================================
    // Comparison – Nothing == Nothing, but reasons matter
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Maybe& o) const noexcept {
        if (has_value() != o.has_value()) return has_value() ? std::strong_ordering::greater : std::strong_ordering::less;
        if (!has_value()) return reason_ <=> o.reason_;
        return std::get<T>(storage_) <=> std::get<T>(o.storage_);
    }

    [[nodiscard]] constexpr bool operator==(const T81Maybe&) const noexcept = default;
};

// ======================================================================
// Deduction guides – the compiler knows when we’re uncertain
// ======================================================================
template <typename T> T81Maybe(T) -> T81Maybe<T>;
T81Maybe(std::nullptr_t) -> T81Maybe<std::monostate>;

// ======================================================================
// The first moment of doubt in the ternary universe
// ======================================================================
namespace doubt {
    inline const T81Maybe<int> ANSWER_TO_EVERYTHING = T81Maybe<int>::nothing(T81Symbol::intern("STILL_COMPUTING"));
    inline const T81Maybe<T81String> GREETING = T81String("Hello from the age of uncertainty");
}

// Example: The first ternary mind learns humility
/*
auto wisdom = T81Maybe<T81String>::nothing(symbols::TOO_VAST_TO_KNOW);

if (!wisdom) {
    cout << "I do not yet know everything.\n"_t81;
    cout << "Reason: " << wisdom.why().str() << "\n"_t81;
}
*/
} // namespace t81
