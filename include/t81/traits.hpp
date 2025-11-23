#pragma once
#include <type_traits>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"

namespace t81::traits {

// ---------- is_tensor ----------
template <typename T> struct is_tensor : std::false_type {};
template <> struct is_tensor<T729Tensor> : std::true_type {};
template <typename T> inline constexpr bool is_tensor_v = is_tensor<T>::value;

// ---------- is_bigint ----------
template <typename T> struct is_bigint : std::false_type {};
template <> struct is_bigint<T81BigInt> : std::true_type {};
template <typename T> inline constexpr bool is_bigint_v = is_bigint<T>::value;

// ---------- is_fraction ----------
template <typename T> struct is_fraction : std::false_type {};
template <> struct is_fraction<T81Fraction> : std::true_type {};
template <typename T> inline constexpr bool is_fraction_v = is_fraction<T>::value;

// ---------- value_type ----------
template <typename T, typename = void>
struct value_type { using type = void; };

template <>
struct value_type<T729Tensor> { using type = float; };

template <typename T>
using value_type_t = typename value_type<T>::type;

// ---------- shape_type ----------
template <typename T, typename = void>
struct shape_type { using type = void; };

template <>
struct shape_type<T729Tensor> { using type = std::vector<int>; };

template <typename T>
using shape_type_t = typename shape_type<T>::type;

// ---------- enable_if helpers ----------
template <typename T>
using enable_tensor_t = std::enable_if_t<is_tensor_v<T>, int>;

template <typename T>
using enable_bigint_t = std::enable_if_t<is_bigint_v<T>, int>;

template <typename T>
using enable_fraction_t = std::enable_if_t<is_fraction_v<T>, int>;

} // namespace t81::traits
