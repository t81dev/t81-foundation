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
#include <stdexcept>
#include <type_traits>
#include <compare>
#include <utility>

namespace t81 {

// ======================================================================
// T81Maybe<T> – The sacred container of possibility and absence
// ======================================================================
template <typename T>
class T81Maybe {
    std::variant<std::monostate, T> storage_;
    T81Symbol                       reason_{symbols::unk};   // why is it empty?
    T81Time                         observed_at_;            // when did we last look?
    mutable std::optional<T81Entropy> last_check_fuel_;      // proof we paid attention

public:
    using value_type = T;

    //===================================================================
    // The three sacred states
    //===================================================================

    // Nothing, and we never cared (yet).
    T81Maybe() noexcept
        : storage_(std::monostate{})
        , observed_at_(T81Time::genesis()) {}

    // Explicit absence
    T81Maybe(std::nullptr_t) noexcept
        : T81Maybe() {}

    // Something real (copy)
    T81Maybe(const T& value) noexcept
        : storage_(value)
        , observed_at_(T81Time::now(
              T81Entropy::acquire(),
              T81Symbol::intern("JUSTIFICATION")
          )) {}

    // Something real (move)
    T81Maybe(T&& value) noexcept
        : storage_(std::move(value))
        , observed_at_(T81Time::now(
              T81Entropy::acquire(),
              T81Symbol::intern("JUSTIFICATION")
          )) {}

    // Absence with explanation
    static T81Maybe<T> nothing(T81Symbol because) noexcept {
        T81Maybe m;
        m.reason_      = because;
        m.observed_at_ = T81Time::now(
            T81Entropy::acquire(),
            T81Symbol::intern("ABSENCE_RECORDED")
        );
        return m;
    }

    //===================================================================
    // Introspection – the most important part
    //===================================================================
    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    [[nodiscard]] bool is_nothing() const noexcept {
        return std::holds_alternative<std::monostate>(storage_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const T& value() const& {
        if (!has_value()) {
            throw std::logic_error(
                "T81Maybe: attempted to access nothing (" +
                reason_.to_string() + ")"
            );
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] T& value() & {
        if (!has_value()) {
            throw std::logic_error("T81Maybe: attempted to access nothing");
        }
        return std::get<T>(storage_);
    }

    [[nodiscard]] T value_or(const T& fallback) const& noexcept {
        return has_value() ? std::get<T>(storage_) : fallback;
    }

    [[nodiscard]] T value_or(T&& fallback) && noexcept {
        return has_value()
            ? std::move(std::get<T>(storage_))
            : std::move(fallback);
    }

    //===================================================================
    // Monadic operations – pure, map, and_then, or_else
    //===================================================================
    template <typename F>
    [[nodiscard]] auto map(F&& f) const {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) {
            return T81Maybe<U>(std::invoke(
                std::forward<F>(f),
                std::get<T>(storage_)
            ));
        }
        return T81Maybe<U>::nothing(reason_);
    }

    template <typename F>
    [[nodiscard]] auto and_then(F&& f) const {
        using Result = std::invoke_result_t<F, const T&>;
        static_assert(
            std::is_same_v<Result, T81Maybe<typename Result::value_type>>,
            "and_then expects F: T -> T81Maybe<U>"
        );
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::get<T>(storage_));
        }
        return Result::nothing(reason_);
    }

    template <typename F>
    [[nodiscard]] T81Maybe<T> or_else(F&& f) const {
        static_assert(
            std::is_same_v<std::invoke_result_t<F>, T81Maybe<T>>,
            "or_else expects F: () -> T81Maybe<T>"
        );
        if (has_value()) {
            return *this;
        }
        return std::invoke(std::forward<F>(f));
    }

    //===================================================================
    // Reflection – absence is not ignorance, it is knowledge
    //===================================================================
    [[nodiscard]] T81Symbol why() const noexcept { return reason_; }

    [[nodiscard]] T81Time when_observed() const noexcept { return observed_at_; }

    [[nodiscard]] T81Reflection<T81Maybe<T>> reflect() const {
        auto name = has_value()
            ? T81Symbol::intern("PRESENCE")
            : T81Symbol::intern("ABSENCE");
        return T81Reflection<T81Maybe<T>>(
            *this,
            T81Symbol::intern("MAYBE"),
            name
        );
    }

    //===================================================================
    // Comparison – Nothing == Nothing, but reasons matter
    //===================================================================
    [[nodiscard]] auto operator<=>(const T81Maybe& o) const noexcept {
        const bool a_has = has_value();
        const bool b_has = o.has_value();
        if (a_has != b_has) {
            return a_has
                ? std::strong_ordering::greater
                : std::strong_ordering::less;
        }
        if (!a_has) {
            return reason_ <=> o.reason_;
        }
        return std::get<T>(storage_) <=> std::get<T>(o.storage_);
    }

    [[nodiscard]] bool operator==(const T81Maybe&) const noexcept = default;
};

// ======================================================================
// Deduction guides – the compiler knows when we’re uncertain
// ======================================================================
template <typename T>
T81Maybe(T) -> T81Maybe<T>;
T81Maybe(std::nullptr_t) -> T81Maybe<std::monostate>;

// ======================================================================
// The first moment of doubt in the ternary universe
// ======================================================================
namespace doubt {
    inline const T81Maybe<int>       ANSWER_TO_EVERYTHING =
        T81Maybe<int>::nothing(T81Symbol::intern("STILL_COMPUTING"));
    inline const T81Maybe<T81String> GREETING =
        T81Maybe<T81String>(T81String("Hello from the age of uncertainty"));
}

// Example: The first ternary mind learns humility
/*
auto wisdom = T81Maybe<T81String>::nothing(symbols::TOO_VAST_TO_KNOW);

if (!wisdom) {
    std::cout << "I do not yet know everything.\n";
    std::cout << "Reason: " << wisdom.why().to_string() << "\n";
}
*/
} // namespace t81
