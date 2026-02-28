/**
 * @file T81Set.hpp
 * @brief Defines the T81Set class, an immutable, ternary-native set.
 */
#pragma once

#include "t81/types/T81Entropy.hpp"
#include "t81/types/T81List.hpp"
#include "t81/types/T81Map.hpp"
#include "t81/types/T81String.hpp"
#include "t81/types/T81Symbol.hpp"

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>

namespace t81 {

template <typename T>
class T81Set {
  T81Map<T, std::monostate> elements_;

public:
  using value_type = T;
  using size_type = std::size_t;

  struct const_iterator {
    typename T81Map<T, std::monostate>::const_iterator impl;

    const_iterator(typename T81Map<T, std::monostate>::const_iterator it) : impl(it) {}
    const_iterator() = default;

    [[nodiscard]] bool operator==(const const_iterator& o) const noexcept = default;

    const_iterator& operator++() noexcept {
      ++impl;
      return *this;
    }

    const_iterator operator++(int) noexcept {
      const_iterator tmp = *this;
      ++impl;
      return tmp;
    }

    [[nodiscard]] const T& operator*() const noexcept { return impl.key(); }

    [[nodiscard]] const T* operator->() const noexcept { return &impl.key(); }
  };

  constexpr T81Set() noexcept = default;

  constexpr T81Set(std::initializer_list<T> init) {
    for (const auto& elem : init) {
      elements_[elem] = {};
    }
  }

  template <typename InputIt>
  constexpr T81Set(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      elements_[*first] = {};
    }
  }

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

  [[nodiscard]] constexpr bool contains(const T& value) const noexcept {
    return elements_.contains(value);
  }

  [[nodiscard]] constexpr size_type size() const noexcept { return elements_.size(); }

  [[nodiscard]] constexpr bool empty() const noexcept { return elements_.empty(); }

  [[nodiscard]] constexpr T81Set union_with(const T81Set& other) const {
    T81Set result = *this;
    for (const auto& elem : other) {
      result.elements_[elem] = {};
    }
    return result;
  }

  [[nodiscard]] constexpr T81Set intersection_with(const T81Set& other) const {
    T81Set result;
    for (const auto& elem : *this) {
      if (other.contains(elem)) {
        result.elements_[elem] = {};
      }
    }
    return result;
  }

  [[nodiscard]] constexpr T81Set difference_from(const T81Set& other) const {
    T81Set result = *this;
    for (const auto& elem : other) {
      result.elements_.erase(elem);
    }
    return result;
  }

  [[nodiscard]] constexpr T81Set symmetric_difference(const T81Set& other) const {
    return union_with(other).difference_from(intersection_with(other));
  }

  [[nodiscard]] constexpr bool subset_of(const T81Set& other) const {
    for (const auto& elem : *this) {
      if (!other.contains(elem)) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] constexpr bool superset_of(const T81Set& other) const {
    return other.subset_of(*this);
  }

  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return const_iterator{elements_.begin()};
  }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return const_iterator{elements_.end()};
  }


  [[nodiscard]] std::string serialize_canonical() const {
    std::ostringstream ss;
    ss << "Set{";
    bool first = true;
    auto items = elements_.iter_sorted();
    for (const auto& item : items) {
      if (!first) ss << ", ";
      if constexpr (requires { item.first.serialize_canonical(); }) {
        ss << item.first.serialize_canonical();
      } else {
        ss << item.first;
      }
      first = false;
    }
    ss << "}";
    return ss.str();
  }

  [[nodiscard]] constexpr T81List<T> to_list() const {
    T81List<T> list;
    for (const auto& elem : *this) {
      list.push_back(elem);
    }
    return list;
  }

  [[nodiscard]] constexpr auto operator<=>(const T81Set& o) const noexcept = default;
  [[nodiscard]] constexpr bool operator==(const T81Set&) const noexcept = default;

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

template <typename... Ts>
T81Set(Ts...) -> T81Set<std::common_type_t<Ts...>>;

template <typename T>
T81Set(std::initializer_list<T>) -> T81Set<T>;

using SymbolSet = T81Set<T81Symbol>;
using TokenSet = T81Set<std::uint32_t>;
using ConceptSet = T81Set<T81String>;

}  // namespace t81
