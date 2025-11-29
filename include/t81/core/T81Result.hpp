/**
 * @file T81Result.hpp
 * @brief Defines the T81Result class for representing success or failure.
 *
 * This file provides the `T81Result<T>` class, a type used to represent the
 * outcome of an operation that can either succeed with a value of type `T` or
 * fail with a detailed `T81Error`. The `T81Error` struct captures not only a
 * symbolic error code and message but also the time, entropy cost, and source
 * of the failure, enabling robust error handling and reflection.
 */
#pragma once

#include "t81/core/T81Maybe.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81Time.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Reflection.hpp"
#include <variant>
#include <stdexcept>
#include <iostream>
#include "t81/core/T81Int.hpp"

namespace t81 {

// ======================================================================
// T81Error – A failure that remembers why, when, and how much it cost
// ======================================================================
struct T81Error {
    T81Symbol    code;           // e.g. symbols::DIVISION_BY_ZERO
    T81String    message;
    T81Time      occurred_at;
    T81Entropy   fuel_spent;     // how much was wasted trying
    T81Symbol    source;         // which agent/function caused it

constexpr T81Error(T81Symbol c, T81String m, T81Symbol src = symbols::unk)
        : code(c)
        , message(std::move(m))
        , occurred_at(T81Time::now(acquire_entropy(), symbols::unk))
        , fuel_spent(acquire_entropy())
        , source(src)
    {}

    T81Error(const T81Error& other)
        : code(other.code)
        , message(other.message)
        , occurred_at(T81Time::now(acquire_entropy(), T81Symbol::intern("ERROR_COPY")))
        , fuel_spent(acquire_entropy()) // Copying an error costs new entropy
        , source(other.source)
    {}

    [[nodiscard]] T81String explain() const {
        return T81String("[ERROR ") + T81String(code.to_string()) + T81String(" at ") + occurred_at.narrate() +
               T81String("] ") + message + T81String(" (source: ") + T81String(source.to_string()) + T81String(")");
    }
};

// ======================================================================
// T81Result<T> – The container of success and honorable failure
// ======================================================================
template <typename T>
class T81Result {
    std::variant<T, T81Error> payload_;

public:
    using value_type = T;
    using error_type = T81Error;

    //===================================================================
    // Constructors – success or failure
    //===================================================================
    constexpr T81Result(const T& value) noexcept : payload_(std::in_place_index<0>, value) {}
    constexpr T81Result(T&& value) noexcept : payload_(std::in_place_index<0>, std::move(value)) {}

    constexpr T81Result(const T81Error& err) noexcept : payload_(std::in_place_index<1>, err) {}
    constexpr T81Result(T81Error&& err) noexcept : payload_(std::in_place_index<1>, std::move(err)) {}

    // From exception-proof functions
    static constexpr T81Result success(T value) noexcept {
        return T81Result(std::move(value));
    }

    static constexpr T81Result failure(T81Symbol code, T81String msg, T81Symbol src = symbols::unk) noexcept {
        return T81Result(T81Error(code, std::move(msg), src));
    }

    //===================================================================
    // State queries
    //===================================================================
    [[nodiscard]] constexpr bool is_ok() const noexcept {
        return std::holds_alternative<T>(payload_);
    }

    [[nodiscard]] constexpr bool is_err() const noexcept {
        return std::holds_alternative<T81Error>(payload_);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return is_ok();
    }

    //===================================================================
    // Safe access
    //===================================================================
    [[nodiscard]] constexpr const T& value() const& {
        if (is_err()) {
            throw std::runtime_error(std::get<T81Error>(payload_).explain().str());
        }
        return std::get<T>(payload_);
    }

    [[nodiscard]] constexpr T& value() & {
        if (is_err()) {
            throw std::runtime_error(std::get<T81Error>(payload_).explain().str());
        }
        return std::get<T>(payload_);
    }

    [[nodiscard]] constexpr T value_or(const T& fallback) const& noexcept {
        return is_ok() ? std::get<T>(payload_) : fallback;
    }

    [[nodiscard]] constexpr T unwrap_or(const T& fallback) const& noexcept {
        return value_or(fallback);
    }

    [[nodiscard]] constexpr const T81Error& error() const& {
        if (is_ok()) {
            throw std::logic_error("T81Result: attempted to access error on success");
        }
        return std::get<T81Error>(payload_);
    }

    //===================================================================
    // Monadic operations – the path of resilience
    //===================================================================
    template <typename F>
    [[nodiscard]] constexpr auto map(F&& f) const& {
        if (is_ok()) {
            using U = std::invoke_result_t<F, const T&>;
            return T81Result<U>(std::forward<F>(f)(std::get<T>(payload_)));
        }
        return T81Result<std::invoke_result_t<F, const T&>>(std::get<T81Error>(payload_));
    }

    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& f) const& {
        if (is_ok()) {
            return std::forward<F>(f)(std::get<T>(payload_));
        }
        using R = std::invoke_result_t<F, const T&>;
        return R(std::get<T81Error>(payload_));
    }

    template <typename F>
    [[nodiscard]] constexpr T81Result or_else(F&& f) const& {
        if (is_ok()) {
            return *this;
        }
        return f(std::get<T81Error>(payload_));
    }

    //===================================================================
    // Reflection – failure is also part of the story
    //===================================================================
    [[nodiscard]] T81Reflection<T81Result<T>> reflect() const {
        auto kind = is_ok() ? T81Symbol::intern("SUCCESS") : T81Symbol::intern("FAILURE");
        return T81Reflection<T81Result<T>>(*this, T81Symbol::intern("RESULT"), kind);
    }

    //===================================================================
    // Panic on failure – the old way, still available
    //===================================================================
    [[nodiscard]] constexpr T unwrap() const {
        return value();  // throws if error
    }

    [[nodiscard]] constexpr T expect(const T81String& msg) const {
        if (is_err()) {
            std::cerr << msg.str() << "\n";
            throw std::runtime_error(msg.str());
        }
        return value();
    }
};

// ======================================================================
// Deduction guides
// ======================================================================
template <typename T> T81Result(T) -> T81Result<T>;
template <typename T> T81Result(T81Error) -> T81Result<T>;

// ======================================================================
// Common error codes – the universal language of failure
// ======================================================================
namespace errors {
    inline const T81Symbol OUT_OF_ENTROPY     = T81Symbol::intern("OUT_OF_ENTROPY");
    inline const T81Symbol DIVISION_BY_ZERO   = T81Symbol::intern("DIVISION_BY_ZERO");
    inline const T81Symbol IO_FAILURE         = T81Symbol::intern("IO_FAILURE");
    inline const T81Symbol UNKNOWN_AGENT      = T81Symbol::intern("UNKNOWN_AGENT");
    inline const T81Symbol TIME_PARADOX       = T81Symbol::intern("TIME_PARADOX");
}

// ======================================================================
// The first honorable failure in the ternary universe
// ======================================================================
namespace wisdom {
    inline constexpr auto SAFE_DIV = [](T81Int<81> a, T81Int<81> b) -> T81Result<T81Int<81>> {
        if (b.is_zero()) {
            return T81Result<T81Int<81>>::failure(
                errors::DIVISION_BY_ZERO,
                T81String("Cannot divide by zero — the universe would collapse."),
                T81Symbol::intern("MATH_MODULE")
            );
        }
        return T81Result<T81Int<81>>::success(a / b);
    };
}

} // namespace t81
