/**
 * @file T81Set.hpp
 * @brief Defines the T81Set class, an immutable, ternary-native set.
 *
 * This file provides the `T81Set<T>` class, an immutable, hash-based set that
 * is implemented on top of the `T81Map`. It offers pure functional-style
 * operations (e.g., `insert`, `erase` return new sets) and is designed for
 * perfect membership testing, particularly when using `T81Symbol` as the key
 * type. It supports standard set-theoretic operations like union, intersection,
 * and difference.
 */
#pragma once

#include "t81/core/T81Symbol.hpp"
#include "t81/core/T81String.hpp"
#include "t81/core/T81Entropy.hpp"
#include "t81/core/T81Map.hpp"
#include "t81/core/T81List.hpp"

#include <cstddef>
#include <compare>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>

namespace t81 {

// ======================================================================
// T81Set<T> – Mathematical set with perfect membership testing
// ======================================================================
template <typename T>
class T81Set {
    // Internally: T81Map<T, std::monostate> — value is irrelevant
    T81Map<T, std::monostate> elements_;

public:
    using value_type     = T;
    using size_type      = std::size_t;
    using const_iterator = typename T81Map<T, std::monostate>::const_iterator;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Set() noexcept = default;

    // From initializer list
    constexpr T81Set(std::initializer_list<T> init) {
        for (const auto& elem : init) {
            elements_[elem] = {};
        }
    }

    // From any input range [first, last)
    template <typename InputIt>
    constexpr T81Set(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            elements_[*first] = {};
        }
    }

    //===================================================================
    // Modifiers – pure functional style (return new set)
    //===================================================================
    [[nodiscard]] constexpr T81Set insert(const T& value) const {
        T81Set copy = *this;
        copy.elements_[value] = {};
        return copy;
    }

    [[nodiscard]] constexpr T81Set insert(T&& value) const {
        T81Set copy = *this;
        copy.elements_[std::move(value)] = {};
        return copy;
    }

    template <typename InputIt>
    [[nodiscard]] constexpr T81Set insert(InputIt first, InputIt last) const {
        T81Set copy = *this;
        for (; first != last; ++first) {
            copy.elements_[*first] = {};
        }
        return copy;
    }

    [[nodiscard]] constexpr T81Set erase(const T& value) const {
        T81Set copy = *this;
        copy.elements_.erase(value);
        return copy;
    }

    //===================================================================
    // Queries – O(1) average, O(log₃ n) worst-case
    //===================================================================
    [[nodiscard]] constexpr bool contains(const T& value) const noexcept {
        return elements_.contains(value);
    }

    [[nodiscard]] constexpr size_type size() const noexcept {
        return elements_.size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return elements_.empty();
    }

    //===================================================================
    // Set operations – pure, lazy, exact
    //===================================================================
    [[nodiscard]] constexpr T81Set union_with(const T81Set& other) const {
        T81Set result = *this;
        for (const auto& [key, _] : other.elements_) {
            result.elements_[key] = {};
        }
        return result;
    }

    [[nodiscard]] constexpr T81Set intersection_with(const T81Set& other) const {
        T81Set result;
        for (const auto& [key, _] : elements_) {
            if (other.contains(key)) {
                result.elements_[key] = {};
            }
        }
        return result;
    }

    [[nodiscard]] constexpr T81Set difference_from(const T81Set& other) const {
        T81Set result = *this;
        for (const auto& [key, _] : other.elements_) {
            result.elements_.erase(key);
        }
        return result;
    }

    [[nodiscard]] constexpr T81Set symmetric_difference(const T81Set& other) const {
        return union_with(other).difference_from(intersection_with(other));
    }

    [[nodiscard]] constexpr bool subset_of(const T81Set& other) const {
        for (const auto& [key, _] : elements_) {
            if (!other.contains(key)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool superset_of(const T81Set& other) const {
        return other.subset_of(*this);
    }

    //===================================================================
    // Iteration
    //===================================================================
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return elements_.begin(); }
    [[nodiscard]] constexpr const_iterator end()   const noexcept { return elements_.end(); }

    //===================================================================
    // Conversion
    //===================================================================
    [[nodiscard]] constexpr T81List<T> to_list() const {
        T81List<T> list;
        for (const auto& [key, _] : elements_) {
            list.push_back(key);
        }
        return list;
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Set& o) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Set&) const noexcept = default;

    //===================================================================
    // Operators
    //===================================================================
    [[nodiscard]] friend constexpr T81Set operator|(const T81Set& a, const T81Set& b) noexcept {
        return a.union_with(b);
    }

    [[nodiscard]] friend constexpr T81Set operator&(const T81Set& a, const T81Set& b) noexcept {
        return a.intersection_with(b);
    }

    [[nodiscard]] friend constexpr T81Set operator-(const T81Set& a, const T81Set& b) noexcept {
        return a.difference_from(b);
    }
};

// ======================================================================
// Deduction guides
// ======================================================================
template <typename... Ts>
T81Set(Ts...) -> T81Set<std::common_type_t<Ts...>>;

template <typename T>
T81Set(std::initializer_list<T>) -> T81Set<T>;

// ======================================================================
// Common sets in the ternary world
// ======================================================================
using SymbolSet  = T81Set<T81Symbol>;
using TokenSet   = T81Set<std::uint32_t>;
using ConceptSet = T81Set<T81String>;

// ======================================================================
// Example: This is how the future reasons with pure sets
// ======================================================================
/*
constexpr auto mammals = T81Set{symbols::HUMAN, symbols::DOG, symbols::CAT};
constexpr auto mortals = T81Set{symbols::HUMAN, symbols::SOCRATES};

auto socrates_is_mortal  = mortals.contains(symbols::SOCRATES);
auto all_humans_mortal   = (mammals & mortals).size() == mammals.size();

static_assert(socrates_is_mortal);
*/

} // namespace t81
