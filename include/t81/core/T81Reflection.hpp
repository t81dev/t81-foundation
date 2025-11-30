/**
 * @file T81Reflection.hpp
 * @brief Minimal reflection wrapper for values.
 *
 * T81Reflection<T> holds:
 *   • a copy of the reflected value
 *   • a type symbol
 *   • an instance symbol
 *
 * This avoids any dependency on entropy, probabilities, or global logs.
 */

#pragma once

#include "t81/core/T81Symbol.hpp"

#include <utility>

namespace t81 {

template <typename T>
class T81Reflection {
public:
    using value_type = T;

private:
    T         value_;
    T81Symbol type_symbol_;
    T81Symbol instance_id_;

public:
    T81Reflection(const T& value,
                  T81Symbol type_symbol,
                  T81Symbol instance_id)
        : value_(value)
        , type_symbol_(type_symbol)
        , instance_id_(instance_id) {}

    T81Reflection(T&& value,
                  T81Symbol type_symbol,
                  T81Symbol instance_id)
        : value_(std::move(value))
        , type_symbol_(type_symbol)
        , instance_id_(instance_id) {}

    [[nodiscard]] const T& value() const noexcept { return value_; }
    [[nodiscard]] T81Symbol type_symbol() const noexcept { return type_symbol_; }
    [[nodiscard]] T81Symbol instance_id() const noexcept { return instance_id_; }
};

} // namespace t81
