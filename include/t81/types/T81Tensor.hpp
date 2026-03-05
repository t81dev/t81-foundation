/**
 * @file T81Tensor.hpp
 * @brief Defines the T81Tensor class, a multi-dimensional array for high performance.
 *
 * This file provides the `T81Tensor<Element, Rank, Dims...>` class, a versatile
 * multi-dimensional array designed for high-performance numerical computing.
 * It is templatized by the element type, rank, and dimensions, and its memory
 * layout is contiguous and 64-byte aligned to be friendly to tensor cores and
 * other hardware accelerators. It supports essential tensor operations like
 * element-wise arithmetic, reshaping, broadcasting, and transposition.
 */
#pragma once

#include "t81/types/T81Complex.hpp"
#include "t81/types/T81Fixed.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81Symbol.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <cstddef>
#include <cstring>
#include <format>
#include <numeric>
#include <span>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace t81 {

// ======================================================================
// Concept: any type that fits in one tryte (81 trits) or is void (for views)
// ======================================================================
template <typename T>
concept T81Element =
    sizeof(T) <= 32 &&                                                       // ≤ 81 trits
    (std::same_as<T, T81Int<81>> || std::same_as<T, T81Float<72, 9>> ||      // 81-trit float
     std::same_as<T, T81Fixed<72, 9>> || std::same_as<T, T81Complex<40>> ||  // 80 trits → padded
     std::same_as<T, T81Symbol>);

// ======================================================================
// Metaprogramming Helpers
// ======================================================================
namespace detail {
template <size_t... Is>
struct Seq {};

template <typename S1, typename S2>
struct Concat;
template <size_t... I1, size_t... I2>
struct Concat<Seq<I1...>, Seq<I2...>> {
  using type = Seq<I1..., I2...>;
};

template <size_t... Is>
struct Reverse;
template <>
struct Reverse<> {
  using type = Seq<>;
};
template <size_t I, size_t... Is>
struct Reverse<I, Is...> {
  using type = typename Concat<typename Reverse<Is...>::type, Seq<I>>::type;
};
}  // namespace detail

// ======================================================================
// T81Tensor<Element, Rank, Dims...> – the ultimate N-D array
// ======================================================================
template <typename Element, size_t Rank, size_t... Dims>
  requires T81Element<Element> && (Rank == sizeof...(Dims)) && (Rank > 0)
class T81Tensor {
public:
  using value_type = Element;

  static constexpr size_t rank() noexcept { return Rank; }
  static constexpr size_t size() noexcept { return (Dims * ...); }
  static constexpr std::array<size_t, Rank> shape() noexcept { return {Dims...}; }

private:
  // P1: Stack vs Heap storage decision
  static constexpr size_t kTotalSize = (Dims * ...);
  // Threshold: 4KB stack limit
  static constexpr bool kUseHeap = (kTotalSize * sizeof(Element) > 4096);

  using Storage = std::conditional_t<kUseHeap, std::vector<Element>, Element[kTotalSize]>;

public:
  // Raw storage — always 64-byte aligned for tensor core friendly
  // Note: For large tensors, this is a std::vector. Direct pointer access (e.g. &data)
  // works for C-array (small) but for vector use data.data().
  alignas(64) Storage data;

  //===================================================================
  // Construction
  //===================================================================
  constexpr T81Tensor() noexcept(!kUseHeap) {
    if constexpr (kUseHeap) {
      data.resize(kTotalSize);
    }
  }

  explicit constexpr T81Tensor(Element fill) noexcept(!kUseHeap) {
    if constexpr (kUseHeap) {
      data.resize(kTotalSize);
    }
    std::fill(std::begin(data), std::end(data), fill);
  }

  // Zero-initialized tensor
  static constexpr T81Tensor zeros() noexcept { return T81Tensor(Element{}); }

  //===================================================================
  // Indexing — variadic, constexpr, bounds-checked in debug only
  //===================================================================
  template <typename... Indices>
    requires(sizeof...(Indices) == Rank) && (std::convertible_to<Indices, size_t> && ...)
  [[nodiscard]] constexpr Element& operator()(Indices... indices) noexcept {
    return data[linear_index(indices...)];
  }

  template <typename... Indices>
    requires(sizeof...(Indices) == Rank) && (std::convertible_to<Indices, size_t> && ...)
  [[nodiscard]] constexpr const Element& operator()(Indices... indices) const noexcept {
    return data[linear_index(indices...)];
  }

  //===================================================================
  // Views & reshaping — zero-cost, zero-copy
  //===================================================================
  [[nodiscard]] constexpr std::span<Element> span() noexcept {
    if constexpr (kUseHeap)
      return {data.data(), size()};
    else
      return {data, size()};
  }
  [[nodiscard]] constexpr std::span<const Element> span() const noexcept {
    if constexpr (kUseHeap)
      return {data.data(), size()};
    else
      return {data, size()};
  }

  template <size_t... NewDims>
    requires((sizeof...(NewDims) == Rank) && (size() == (NewDims * ...)))
  [[nodiscard]] constexpr auto reshape() const noexcept -> T81Tensor<Element, Rank, NewDims...> {
    T81Tensor<Element, Rank, NewDims...> out;
    if (std::is_constant_evaluated()) {
      std::copy(span().begin(), span().end(), out.span().begin());
    } else {
      std::memcpy(out.span().data(), span().data(), size() * sizeof(Element));
    }
    return out;
  }

  //===================================================================
  // Broadcasting — compile-time shape propagation (Axion does this in HW)
  //===================================================================
  /**
   * @brief Broadcasts the tensor to a new shape by tiling.
   * @tparam NewDims The target dimensions.
   * @return A new tensor with the specified dimensions.
   */
  template <size_t... NewDims>
    requires(sizeof...(NewDims) == Rank)
  [[nodiscard]] constexpr auto broadcast_to() const noexcept
      -> T81Tensor<Element, Rank, NewDims...> {
    T81Tensor<Element, Rank, NewDims...> out;
    std::array<size_t, Rank> old_shape = shape();
    std::array<size_t, Rank> new_shape = {NewDims...};

    // Simple tiling broadcast logic
    for (size_t i = 0; i < out.size(); ++i) {
      std::array<size_t, Rank> coords;
      size_t temp_i = i;
      for (int r = Rank - 1; r >= 0; --r) {
        coords[r] = (temp_i % new_shape[r]) % old_shape[r];
        temp_i /= new_shape[r];
      }

      size_t old_flat = 0;
      size_t stride = 1;
      for (int r = Rank - 1; r >= 0; --r) {
        old_flat += coords[r] * stride;
        stride *= old_shape[r];
      }
      out.span()[i] = span()[old_flat];
    }
    return out;
  }

  //===================================================================
  // Element-wise arithmetic — fused into single ternary instruction stream
  //===================================================================
  [[nodiscard]] constexpr T81Tensor operator+(const T81Tensor& o) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] + o.data[i];
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator-(const T81Tensor& o) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] - o.data[i];
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator*(const T81Tensor& o) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] * o.data[i];
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator/(const T81Tensor& o) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] / o.data[i];
    return r;
  }

  // Scalar Broadcasting
  [[nodiscard]] constexpr T81Tensor operator+(Element s) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] + s;
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator-(Element s) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] - s;
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator*(Element s) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] * s;
    return r;
  }

  [[nodiscard]] constexpr T81Tensor operator/(Element s) const noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = data[i] / s;
    return r;
  }

  // Friend Scalar Ops (Scalar + Tensor)
  friend constexpr T81Tensor operator+(Element s, const T81Tensor& t) noexcept { return t + s; }
  friend constexpr T81Tensor operator*(Element s, const T81Tensor& t) noexcept { return t * s; }
  friend constexpr T81Tensor operator-(Element s, const T81Tensor& t) noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = s - t.data[i];
    return r;
  }
  friend constexpr T81Tensor operator/(Element s, const T81Tensor& t) noexcept {
    T81Tensor r;
    for (size_t i = 0; i < size(); ++i) r.data[i] = s / t.data[i];
    return r;
  }

  //===================================================================
  // Comparison
  //===================================================================
  [[nodiscard]] constexpr auto operator<=>(const T81Tensor&) const noexcept = default;
  [[nodiscard]] constexpr bool operator==(const T81Tensor&) const noexcept = default;

  // P2: Canonical serialization
  [[nodiscard]] std::string serialize_canonical() const {
    std::stringstream ss;
    ss << "shape: [";
    bool first = true;
    for (auto d : shape()) {
      if (!first) ss << ", ";
      ss << d;
      first = false;
    }
    ss << "], data: [";
    first = true;
    for (const auto& e : span()) {
      if (!first) ss << ", ";
      if constexpr (requires { e.to_canonical_string(); }) {
        ss << e.to_canonical_string();
      } else if constexpr (requires { e.serialize_canonical(); }) {
        ss << e.serialize_canonical();
      } else {
        ss << e;
      }
      first = false;
    }
    ss << "]";
    return ss.str();
  }

private:
  // Row-major linear index
  template <typename... Indices>
  [[nodiscard]] constexpr size_t linear_index(Indices... indices) const noexcept {
    size_t idx[Rank] = {static_cast<size_t>(indices)...};
    size_t flat = 0;
    size_t stride = 1;
    for (int i = static_cast<int>(Rank) - 1; i >= 0; --i) {
      flat += idx[i] * stride;
      stride *= shape()[static_cast<size_t>(i)];
    }
    return flat;
  }
};

// ======================================================================
// The canonical tensor types of the new world
// ======================================================================
using float81 = T81Float<72, 9>;

using Vec81 = T81Tensor<float81, 1, 81>;                           // 81-dim embedding
using Vec4K = T81Tensor<float81, 1, 4096>;                         // transformer hidden state
using Mat81x81 = T81Tensor<float81, 2, 81, 81>;                    // attention matrix
using Mat4Kx4K = T81Tensor<float81, 2, 4096, 4096>;                // weight matrix
using TokenBatch = T81Tensor<float81, 2, 128, 4096>;               // batch, seq, dim
using KVCache = T81Tensor<T81Fixed<72, 9>, 4, 128, 128, 128, 64>;  // layers, heads, seq, dim

// Symbolic tensor — exact HRR binding
using SymbolTensor = T81Tensor<T81Symbol, 1, 81>;

//===================================================================
// Slicing (Axis 0)
//===================================================================
namespace detail {
template <size_t NewFirst, typename TensorType>
struct ReplaceFirstDimHelper;

template <size_t NewFirst, typename E, size_t Rank, size_t D0, size_t... Rest>
struct ReplaceFirstDimHelper<NewFirst, T81Tensor<E, Rank, D0, Rest...>> {
  using type = T81Tensor<E, Rank, NewFirst, Rest...>;
};
}  // namespace detail

/**
 * @brief Slices the tensor along the first dimension (axis 0).
 * @tparam Start The starting index (inclusive).
 * @tparam End The ending index (exclusive).
 */
template <size_t Start, size_t End, typename E, size_t Rank, size_t... Dims>
  requires(Start < End) && (End <= std::get<0>(std::array{Dims...}))
[[nodiscard]] constexpr auto slice(const T81Tensor<E, Rank, Dims...>& t) noexcept ->
    typename detail::ReplaceFirstDimHelper<End - Start, T81Tensor<E, Rank, Dims...>>::type {
  using OutTensor =
      typename detail::ReplaceFirstDimHelper<End - Start, T81Tensor<E, Rank, Dims...>>::type;

  // Calculate size of one slice in dimension 0
  // Total size / dim0
  constexpr size_t dim0 = std::get<0>(std::array{Dims...});
  constexpr size_t stride_0 = T81Tensor<E, Rank, Dims...>::size() / dim0;

  OutTensor out;
  const size_t copy_len = (End - Start) * stride_0;
  const size_t offset = Start * stride_0;

  if (std::is_constant_evaluated()) {
    for (size_t i = 0; i < copy_len; ++i) out.data[i] = t.data[offset + i];
  } else {
    std::memcpy(out.span().data(), t.span().data() + offset, copy_len * sizeof(E));
  }
  return out;
}

//===================================================================
// Normalization and Embedding
//===================================================================

/**
 * @brief Layer Normalization over the last dimension.
 * Output = (Input - Mean) / sqrt(Variance + eps)
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto layer_norm(const T81Tensor<E, Rank, Dims...>& t,
                                        E eps = E(1e-5)) noexcept -> T81Tensor<E, Rank, Dims...> {
  T81Tensor<E, Rank, Dims...> out;
  constexpr size_t total_size = T81Tensor<E, Rank, Dims...>::size();
  constexpr std::array<size_t, Rank> shape = {Dims...};
  constexpr size_t last_dim = shape[Rank - 1];

  for (size_t base = 0; base < total_size; base += last_dim) {
    // 1. Mean
    E sum{};
    for (size_t i = 0; i < last_dim; ++i) sum = sum + t.data[base + i];
    E mean = sum / E(static_cast<long long>(last_dim));

    // 2. Variance
    E sum_sq_diff{};
    for (size_t i = 0; i < last_dim; ++i) {
      E diff = t.data[base + i] - mean;
      sum_sq_diff = sum_sq_diff + diff * diff;
    }
    E var = sum_sq_diff / E(static_cast<long long>(last_dim));

    // 3. Normalize
    // Requires E to support sqrt(). T81Float does.
    E inv_std = E(1) / (var + eps).sqrt();

    for (size_t i = 0; i < last_dim; ++i) {
      out.data[base + i] = (t.data[base + i] - mean) * inv_std;
    }
  }
  return out;
}

/**
 * @brief Embedding lookup.
 * Input: (N) indices
 * Weight: (V, D) embedding table
 * Output: (N, D)
 */
template <typename IndexType, size_t N, typename E, size_t V, size_t D>
[[nodiscard]] constexpr auto embedding(const T81Tensor<IndexType, 1, N>& indices,
                                       const T81Tensor<E, 2, V, D>& weights) noexcept
    -> T81Tensor<E, 2, N, D> {
  T81Tensor<E, 2, N, D> out;
  for (size_t i = 0; i < N; ++i) {
    size_t idx = 0;
    if constexpr (std::is_integral_v<IndexType>) {
      idx = static_cast<size_t>(indices(i));
    } else {
      // Assume .to_int64() exists (e.g. T81Int)
      idx = static_cast<size_t>(indices(i).to_int64());
    }

    if (idx >= V) {
      for (size_t d = 0; d < D; ++d) out(i, d) = E{};
    } else {
      for (size_t d = 0; d < D; ++d) {
        out(i, d) = weights(idx, d);
      }
    }
  }
  return out;
}

/**
 * @brief Embedding lookup for Batch of sequences.
 * Input: (B, T) indices
 * Weight: (V, D) embedding table
 * Output: (B, T, D)
 */
template <typename IndexType, size_t B, size_t T, typename E, size_t V, size_t D>
[[nodiscard]] constexpr auto embedding(const T81Tensor<IndexType, 2, B, T>& indices,
                                       const T81Tensor<E, 2, V, D>& weights) noexcept
    -> T81Tensor<E, 3, B, T, D> {
  T81Tensor<E, 3, B, T, D> out;
  for (size_t b = 0; b < B; ++b) {
    for (size_t t = 0; t < T; ++t) {
      size_t idx = 0;
      if constexpr (std::is_integral_v<IndexType>) {
        idx = static_cast<size_t>(indices(b, t));
      } else {
        idx = static_cast<size_t>(indices(b, t).to_int64());
      }

      if (idx >= V) {
        for (size_t d = 0; d < D; ++d) out(b, t, d) = E{};
      } else {
        for (size_t d = 0; d < D; ++d) {
          out(b, t, d) = weights(idx, d);
        }
      }
    }
  }
  return out;
}

/**
 * @brief Softmax activation along the last dimension.
 * softmax(x)_i = exp(x_i) / sum(exp(x_j))
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto softmax(const T81Tensor<E, Rank, Dims...>& t) noexcept
    -> T81Tensor<E, Rank, Dims...> {
  T81Tensor<E, Rank, Dims...> out;
  constexpr size_t total_size = T81Tensor<E, Rank, Dims...>::size();
  constexpr std::array<size_t, Rank> shape = {Dims...};
  constexpr size_t last_dim = shape[Rank - 1];

  // Process chunks of size `last_dim`
  // Since tensor is row-major, the last dimension is contiguous.
  for (size_t base = 0; base < total_size; base += last_dim) {
    // 1. Find max for numerical stability
    E max_val = t.data[base];
    for (size_t i = 1; i < last_dim; ++i) {
      if (t.data[base + i] > max_val) max_val = t.data[base + i];
    }

    // 2. Compute exponentials and sum
    E sum_exp{};  // Default construct to 0
    for (size_t i = 0; i < last_dim; ++i) {
      E val = t.data[base + i] - max_val;
      // Assuming E has exp()
      E e = val.exp();
      out.data[base + i] = e;  // Store in output temporarily
      sum_exp = sum_exp + e;
    }

    // 3. Normalize
    for (size_t i = 0; i < last_dim; ++i) {
      out.data[base + i] = out.data[base + i] / sum_exp;
    }
  }
  return out;
}

//===================================================================
// Concatenation (Axis 0)
//===================================================================
namespace detail {
template <typename T1, typename T2>
struct ConcatFirstDimHelper;

template <typename E, size_t Rank, size_t D0_1, size_t... Rest, size_t D0_2>
struct ConcatFirstDimHelper<T81Tensor<E, Rank, D0_1, Rest...>, T81Tensor<E, Rank, D0_2, Rest...>> {
  using type = T81Tensor<E, Rank, D0_1 + D0_2, Rest...>;
};
}  // namespace detail

/**
 * @brief Concatenates two tensors along the first dimension (axis 0).
 */
template <typename E, size_t Rank, size_t D0_1, size_t... Rest, size_t D0_2>
[[nodiscard]] constexpr auto concat(const T81Tensor<E, Rank, D0_1, Rest...>& t1,
                                    const T81Tensor<E, Rank, D0_2, Rest...>& t2) noexcept ->
    typename detail::ConcatFirstDimHelper<T81Tensor<E, Rank, D0_1, Rest...>,
                                          T81Tensor<E, Rank, D0_2, Rest...>>::type {
  using OutTensor = typename detail::ConcatFirstDimHelper<T81Tensor<E, Rank, D0_1, Rest...>,
                                                          T81Tensor<E, Rank, D0_2, Rest...>>::type;

  OutTensor out;
  const size_t size1 = t1.size();
  const size_t size2 = t2.size();

  if (std::is_constant_evaluated()) {
    for (size_t i = 0; i < size1; ++i) out.data[i] = t1.data[i];
    for (size_t i = 0; i < size2; ++i) out.data[size1 + i] = t2.data[i];
  } else {
    std::memcpy(out.span().data(), t1.span().data(), size1 * sizeof(E));
    std::memcpy(out.span().data() + size1, t2.span().data(), size2 * sizeof(E));
  }
  return out;
}

//===================================================================
// Permute Implementation
//===================================================================

namespace detail {
// Helper to extract the I-th element from a pack
template <size_t I, typename... Ts>
struct GetTypeAt;
template <size_t I, typename Head, typename... Tail>
struct GetTypeAt<I, Head, Tail...> {
  using type = typename GetTypeAt<I - 1, Tail...>::type;
};
template <typename Head, typename... Tail>
struct GetTypeAt<0, Head, Tail...> {
  using type = Head;
};

// Helper to extract the I-th size_t from a pack
template <size_t I, size_t... Dims>
struct GetDimAt;
template <size_t I, size_t Head, size_t... Tail>
struct GetDimAt<I, Head, Tail...> {
  static constexpr size_t value = GetDimAt<I - 1, Tail...>::value;
};
template <size_t Head, size_t... Tail>
struct GetDimAt<0, Head, Tail...> {
  static constexpr size_t value = Head;
};

template <typename E, size_t Rank, typename PermSeq, typename DimsSeq>
struct PermuteHelper;

template <typename E, size_t Rank, size_t... Perms, size_t... Dims>
struct PermuteHelper<E, Rank, Seq<Perms...>, Seq<Dims...>> {
  using type = T81Tensor<E, Rank, GetDimAt<Perms, Dims...>::value...>;
};
}  // namespace detail

/**
 * @brief Permutes the dimensions of the tensor.
 * @tparam Perms The new order of dimensions.
 */
template <size_t... Perms, typename E, size_t Rank, size_t... Dims>
  requires(sizeof...(Perms) == Rank)
[[nodiscard]] constexpr auto permute(const T81Tensor<E, Rank, Dims...>& t) noexcept ->
    typename detail::PermuteHelper<E, Rank, detail::Seq<Perms...>, detail::Seq<Dims...>>::type {
  using OutTensor =
      typename detail::PermuteHelper<E, Rank, detail::Seq<Perms...>, detail::Seq<Dims...>>::type;
  OutTensor out;

  constexpr std::array<size_t, Rank> in_shape = {Dims...};
  constexpr std::array<size_t, Rank> out_shape = OutTensor::shape();
  constexpr std::array<size_t, Rank> perms = {Perms...};

  // Precompute strides
  std::array<size_t, Rank> in_strides{};
  std::array<size_t, Rank> out_strides{};

  size_t s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    in_strides[i] = s;
    s *= in_shape[i];
  }
  s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    out_strides[i] = s;
    s *= out_shape[i];
  }

  const size_t size = out.size();
  for (size_t i = 0; i < size; ++i) {
    // Decode flat out index to out_coords
    std::array<size_t, Rank> out_coords{};
    size_t temp = i;
    for (int d = 0; d < (int)Rank; ++d) {
      // Using precomputed strides for decoding requires careful iteration order
      // Standard row-major: flat = c0*s0 + c1*s1 ...
      // So we decode from dim 0
      size_t stride = out_strides[d];
      out_coords[d] = temp / stride;
      temp %= stride;
    }

    // Map to in_coords
    // in_coords[perms[k]] = out_coords[k]
    std::array<size_t, Rank> in_coords{};
    for (size_t k = 0; k < Rank; ++k) {
      in_coords[perms[k]] = out_coords[k];
    }

    // Encode in_coords to in_flat
    size_t in_flat = 0;
    for (size_t k = 0; k < Rank; ++k) {
      in_flat += in_coords[k] * in_strides[k];
    }

    out.data[i] = t.data[in_flat];
  }

  return out;
}

//===================================================================
// Padding Implementation
//===================================================================

namespace detail {
template <typename E, size_t Rank, typename PadSeq, typename DimsSeq, typename IsSeq>
struct PadHelper;

template <typename E, size_t Rank, size_t... Pads, size_t... Dims, size_t... Is>
struct PadHelper<E, Rank, Seq<Pads...>, Seq<Dims...>, std::index_sequence<Is...>> {
  using type = T81Tensor<E, Rank,
                         (GetDimAt<Is, Dims...>::value + GetDimAt<2 * Is, Pads...>::value +
                          GetDimAt<2 * Is + 1, Pads...>::value)...>;
};
}  // namespace detail

/**
 * @brief Pads the tensor with a constant value.
 * @tparam Pads... Pairs of (pad_before, pad_after) for each dimension. Total 2*Rank arguments.
 */
template <size_t... Pads, typename E, size_t Rank, size_t... Dims>
  requires(sizeof...(Pads) == 2 * Rank)
[[nodiscard]] constexpr auto pad(const T81Tensor<E, Rank, Dims...>& t, E value) noexcept ->
    typename detail::PadHelper<E, Rank, detail::Seq<Pads...>, detail::Seq<Dims...>,
                               std::make_index_sequence<Rank>>::type {
  using OutTensor = typename detail::PadHelper<E, Rank, detail::Seq<Pads...>, detail::Seq<Dims...>,
                                               std::make_index_sequence<Rank>>::type;
  OutTensor out(value);  // Fill with pad value

  constexpr std::array<size_t, 2 * Rank> pads = {Pads...};
  constexpr std::array<size_t, Rank> in_shape = {Dims...};
  constexpr std::array<size_t, Rank> out_shape = OutTensor::shape();

  // Precompute strides
  std::array<size_t, Rank> in_strides{};
  std::array<size_t, Rank> out_strides{};

  size_t s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    in_strides[i] = s;
    s *= in_shape[i];
  }
  s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    out_strides[i] = s;
    s *= out_shape[i];
  }

  const size_t size = t.size();
  for (size_t i = 0; i < size; ++i) {
    // Decode in_flat -> in_coords
    std::array<size_t, Rank> in_coords{};
    size_t temp = i;
    for (int d = 0; d < (int)Rank; ++d) {
      size_t stride = in_strides[d];
      in_coords[d] = temp / stride;
      temp %= stride;
    }

    // Map to out_coords: out_coords[d] = in_coords[d] + pad_before[d]
    // pad_before[d] is pads[2*d]
    std::array<size_t, Rank> out_coords{};
    for (size_t d = 0; d < Rank; ++d) {
      out_coords[d] = in_coords[d] + pads[2 * d];
    }

    // Encode out_coords -> out_flat
    size_t out_flat = 0;
    for (size_t d = 0; d < Rank; ++d) {
      out_flat += out_coords[d] * out_strides[d];
    }

    out.data[out_flat] = t.data[i];
  }

  return out;
}

//===================================================================
// Activation Functions
//===================================================================

/**
 * @brief Rectified Linear Unit activation.
 * ReLU(x) = max(0, x)
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto relu(const T81Tensor<E, Rank, Dims...>& t) noexcept
    -> T81Tensor<E, Rank, Dims...> {
  T81Tensor<E, Rank, Dims...> out;
  E zero{};  // Default construct to 0
  for (size_t i = 0; i < t.size(); ++i) {
    if (t.data[i] > zero)
      out.data[i] = t.data[i];
    else
      out.data[i] = zero;
  }
  return out;
}

/**
 * @brief Gaussian Error Linear Unit activation.
 * GELU(x) ≈ 0.5x(1 + tanh(sqrt(2/π)(x + 0.044715x³)))
 * Note: Requires Element type to support arithmetic and exp().
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto gelu(const T81Tensor<E, Rank, Dims...>& t) noexcept
    -> T81Tensor<E, Rank, Dims...> {
  T81Tensor<E, Rank, Dims...> out;
  // Constants
  E half(0.5);
  E one(1.0);
  // sqrt(2/pi) approx 0.7978845608
  E c1(0.7978845608);
  // 0.044715
  E c2(0.044715);

  for (size_t i = 0; i < t.size(); ++i) {
    E x = t.data[i];
    E x3 = x * x * x;
    E inner = c1 * (x + c2 * x3);

    // tanh(u) = (exp(2u) - 1) / (exp(2u) + 1)
    E two_inner = inner + inner;
    E exp_val = two_inner.exp();  // Assuming E has exp() member function (like T81Float)

    E tanh_val = (exp_val - one) / (exp_val + one);

    out.data[i] = half * x * (one + tanh_val);
  }
  return out;
}

//===================================================================
// Generic Transpose Implementation
//===================================================================

namespace detail {
// Helper to construct T81Tensor from a reversed sequence of dims
template <typename E, size_t Rank, typename DimsSeq>
struct TransposeHelper;

template <typename E, size_t Rank, size_t... Dims>
struct TransposeHelper<E, Rank, Seq<Dims...>> {
  using type = T81Tensor<E, Rank, Dims...>;
};

template <typename E, size_t Rank, size_t... Dims>
using TransposedType = typename TransposeHelper<E, Rank, typename Reverse<Dims...>::type>::type;
}  // namespace detail

/**
 * @brief Generic transpose: reverses the dimensions of any rank tensor.
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto transpose(const T81Tensor<E, Rank, Dims...>& t) noexcept
    -> detail::TransposedType<E, Rank, Dims...> {
  using OutTensor = detail::TransposedType<E, Rank, Dims...>;
  OutTensor out{};

  // We need to iterate over all elements of the input tensor, calculate their multi-index,
  // reverse that index, and assign to the output.
  // Since we don't have a dynamic multi-index iterator, we iterate flat and decode.

  constexpr std::array<size_t, Rank> in_shape = {Dims...};
  // Precompute input strides
  std::array<size_t, Rank> in_strides{};
  size_t s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    in_strides[i] = s;
    s *= in_shape[i];
  }

  // Precompute output strides (reversed shape)
  constexpr std::array<size_t, Rank> out_shape = OutTensor::shape();
  std::array<size_t, Rank> out_strides{};
  s = 1;
  for (int i = (int)Rank - 1; i >= 0; --i) {
    out_strides[i] = s;
    s *= out_shape[i];
  }

  for (size_t i = 0; i < t.size(); ++i) {
    // Decode linear index i to in_coords
    std::array<size_t, Rank> coords{};
    size_t temp = i;
    for (int d = 0; d < (int)Rank; ++d) {
      coords[d] = temp / in_strides[d];
      temp %= in_strides[d];
    }

    // Calculate out_index by reversing coords
    size_t out_idx = 0;
    for (int d = 0; d < (int)Rank; ++d) {
      // New dim 0 corresponds to old dim (Rank-1)
      // out coords[d] = in coords[Rank-1-d]
      out_idx += coords[Rank - 1 - d] * out_strides[d];
    }

    out.data[out_idx] = t.data[i];
  }

  return out;
}

//===================================================================
// Matrix Multiplication
//===================================================================
/**
 * @brief Matrix multiplication for Rank 2 tensors.
 * (M, K) x (K, N) -> (M, N)
 */
template <typename E, size_t M, size_t K, size_t K_check, size_t N>
  requires(K == K_check)
[[nodiscard]] constexpr auto matmul(const T81Tensor<E, 2, M, K>& a,
                                    const T81Tensor<E, 2, K_check, N>& b) noexcept
    -> T81Tensor<E, 2, M, N> {
  T81Tensor<E, 2, M, N> out;  // zero init
  // Naive O(M*N*K)
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      E sum{};
      for (size_t k = 0; k < K; ++k) {
        sum = sum + a(i, k) * b(k, j);
      }
      out(i, j) = sum;
    }
  }
  return out;
}

/**
 * @brief Matrix-Vector multiplication.
 * (M, K) x (K) -> (M)
 */
template <typename E, size_t M, size_t K, size_t K_check>
  requires(K == K_check)
[[nodiscard]] constexpr auto matmul(const T81Tensor<E, 2, M, K>& a,
                                    const T81Tensor<E, 1, K_check>& b) noexcept
    -> T81Tensor<E, 1, M> {
  T81Tensor<E, 1, M> out;
  for (size_t i = 0; i < M; ++i) {
    E sum{};
    for (size_t k = 0; k < K; ++k) {
      sum = sum + a(i, k) * b(k);
    }
    out(i) = sum;
  }
  return out;
}

//===================================================================
// Reductions
//===================================================================
/**
 * @brief Computes the sum of all elements in the tensor.
 */
template <typename E, size_t... Dims>
[[nodiscard]] constexpr E reduce_sum(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
  E sum{};
  for (const auto& x : t.span()) sum = sum + x;
  return sum;
}

/**
 * @brief Computes the product of all elements in the tensor.
 */
template <typename E, size_t... Dims>
[[nodiscard]] constexpr E reduce_prod(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
  E prod{1};  // Assuming 1 construction
  for (const auto& x : t.span()) prod = prod * x;
  return prod;
}

/**
 * @brief Computes the mean of all elements in the tensor.
 */
template <typename E, size_t... Dims>
[[nodiscard]] constexpr E reduce_mean(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
  using TensorType = T81Tensor<E, sizeof...(Dims), Dims...>;
  if constexpr (TensorType::size() == 0) return E{};
  E sum = reduce_sum(t);
  return sum / E(static_cast<long long>(TensorType::size()));
}

/**
 * @brief Computes the max of all elements in the tensor.
 */
template <typename E, size_t... Dims>
[[nodiscard]] constexpr E reduce_max(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
  using TensorType = T81Tensor<E, sizeof...(Dims), Dims...>;
  if constexpr (TensorType::size() == 0) return E{};
  E m = t.data[0];
  for (size_t i = 1; i < t.size(); ++i) {
    if (t.data[i] > m) m = t.data[i];
  }
  return m;
}

/**
 * @brief Computes the min of all elements in the tensor.
 */
template <typename E, size_t... Dims>
[[nodiscard]] constexpr E reduce_min(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
  using TensorType = T81Tensor<E, sizeof...(Dims), Dims...>;
  if constexpr (TensorType::size() == 0) return E{};
  E m = t.data[0];
  for (size_t i = 1; i < t.size(); ++i) {
    if (t.data[i] < m) m = t.data[i];
  }
  return m;
}

/**
 * @brief Computes the dot product of two Rank 1 tensors.
 */
template <typename E, size_t N>
[[nodiscard]] constexpr E contract(const T81Tensor<E, 1, N>& a,
                                   const T81Tensor<E, 1, N>& b) noexcept {
  E res{};
  for (size_t i = 0; i < N; ++i) res = res + a(i) * b(i);
  return res;
}

/**
 * @brief Returns the upper triangular part of a matrix (Rank 2).
 * Elements below the diagonal are set to zero.
 * @tparam k The diagonal index (0 is main diagonal, >0 is above, <0 is below).
 */
template <int k = 0, typename E, size_t R, size_t C>
[[nodiscard]] constexpr auto triu(const T81Tensor<E, 2, R, C>& t) noexcept
    -> T81Tensor<E, 2, R, C> {
  T81Tensor<E, 2, R, C> out;  // zero init
  E zero{};
  for (size_t r = 0; r < R; ++r) {
    for (size_t c = 0; c < C; ++c) {
      long long diff = static_cast<long long>(c) - static_cast<long long>(r);
      if (diff >= k) {
        out(r, c) = t(r, c);
      } else {
        out(r, c) = zero;
      }
    }
  }
  return out;
}

/**
 * @brief Returns the lower triangular part of a matrix (Rank 2).
 * Elements above the diagonal are set to zero.
 */
template <int k = 0, typename E, size_t R, size_t C>
[[nodiscard]] constexpr auto tril(const T81Tensor<E, 2, R, C>& t) noexcept
    -> T81Tensor<E, 2, R, C> {
  T81Tensor<E, 2, R, C> out;
  E zero{};
  for (size_t r = 0; r < R; ++r) {
    for (size_t c = 0; c < C; ++c) {
      long long diff = static_cast<long long>(c) - static_cast<long long>(r);
      if (diff <= k) {
        out(r, c) = t(r, c);
      } else {
        out(r, c) = zero;
      }
    }
  }
  return out;
}

/**
 * @brief Extracts the diagonal from a matrix (Rank 2).
 * @return Rank 1 tensor.
 */
template <int k = 0, typename E, size_t R, size_t C>
[[nodiscard]] constexpr auto diag(const T81Tensor<E, 2, R, C>& t) noexcept {
  constexpr long long start_col = (k >= 0) ? k : 0;
  constexpr long long start_row = (k >= 0) ? 0 : -k;

  constexpr long long raw_size =
      std::min(static_cast<long long>(R) - start_row, static_cast<long long>(C) - start_col);
  constexpr size_t N = (raw_size > 0) ? static_cast<size_t>(raw_size) : 0;

  T81Tensor<E, 1, N> out;

  if constexpr (N > 0) {
    for (size_t i = 0; i < N; ++i) {
      out(i) = t(static_cast<size_t>(start_row + i), static_cast<size_t>(start_col + i));
    }
  }
  return out;
}

/**
 * @brief Constructs a diagonal matrix from a vector (Rank 1).
 * @return Rank 2 tensor (square).
 */
template <typename E, size_t N>
[[nodiscard]] constexpr auto diag(const T81Tensor<E, 1, N>& v) noexcept -> T81Tensor<E, 2, N, N> {
  T81Tensor<E, 2, N, N> out;  // zero init
  for (size_t i = 0; i < N; ++i) {
    out(i, i) = v(i);
  }
  return out;
}

/**
 * @brief Clamps all elements in the tensor to [min, max].
 */
template <typename E, size_t Rank, size_t... Dims>
[[nodiscard]] constexpr auto clamp(const T81Tensor<E, Rank, Dims...>& t, E min, E max) noexcept
    -> T81Tensor<E, Rank, Dims...> {
  T81Tensor<E, Rank, Dims...> out;
  for (size_t i = 0; i < t.size(); ++i) {
    E val = t.data[i];
    if (val < min)
      val = min;
    else if (val > max)
      val = max;
    out.data[i] = val;
  }
  return out;
}

//===================================================================
// Convolution and Pooling
//===================================================================

/**
 * @brief 2D Convolution.
 * Input: (N, C, H, W)
 * Weight: (O, C, KH, KW)
 * Output: (N, O, OH, OW)
 */
template <size_t StrideH = 1, size_t StrideW = 1, size_t PadH = 0, size_t PadW = 0, typename E,
          size_t N, size_t C, size_t H, size_t W, size_t O, size_t KH, size_t KW>
[[nodiscard]] constexpr auto conv2d(const T81Tensor<E, 4, N, C, H, W>& input,
                                    const T81Tensor<E, 4, O, C, KH, KW>& weight) noexcept
    -> T81Tensor<E, 4, N, O, (H + 2 * PadH - KH) / StrideH + 1, (W + 2 * PadW - KW) / StrideW + 1> {
  constexpr size_t OH = (H + 2 * PadH - KH) / StrideH + 1;
  constexpr size_t OW = (W + 2 * PadW - KW) / StrideW + 1;
  T81Tensor<E, 4, N, O, OH, OW> out;  // zero initialized

  for (size_t n = 0; n < N; ++n) {
    for (size_t o = 0; o < O; ++o) {
      for (size_t oh = 0; oh < OH; ++oh) {
        for (size_t ow = 0; ow < OW; ++ow) {
          E sum{};
          // Input start coordinates
          const int ih_start = static_cast<int>(oh * StrideH) - static_cast<int>(PadH);
          const int iw_start = static_cast<int>(ow * StrideW) - static_cast<int>(PadW);

          for (size_t c = 0; c < C; ++c) {
            for (size_t kh = 0; kh < KH; ++kh) {
              for (size_t kw = 0; kw < KW; ++kw) {
                int ih = ih_start + static_cast<int>(kh);
                int iw = iw_start + static_cast<int>(kw);

                if (ih >= 0 && ih < static_cast<int>(H) && iw >= 0 && iw < static_cast<int>(W)) {
                  sum = sum + input(n, c, static_cast<size_t>(ih), static_cast<size_t>(iw)) *
                                  weight(o, c, kh, kw);
                }
              }
            }
          }
          out(n, o, oh, ow) = sum;
        }
      }
    }
  }
  return out;
}

/**
 * @brief 2D Max Pooling.
 * Input: (N, C, H, W)
 * Output: (N, C, OH, OW)
 */
template <size_t KH, size_t KW, size_t StrideH = KH, size_t StrideW = KW, size_t PadH = 0,
          size_t PadW = 0, typename E, size_t N, size_t C, size_t H, size_t W>
[[nodiscard]] constexpr auto max_pool2d(const T81Tensor<E, 4, N, C, H, W>& input) noexcept
    -> T81Tensor<E, 4, N, C, (H + 2 * PadH - KH) / StrideH + 1, (W + 2 * PadW - KW) / StrideW + 1> {
  constexpr size_t OH = (H + 2 * PadH - KH) / StrideH + 1;
  constexpr size_t OW = (W + 2 * PadW - KW) / StrideW + 1;
  T81Tensor<E, 4, N, C, OH, OW> out;

  for (size_t n = 0; n < N; ++n) {
    for (size_t c = 0; c < C; ++c) {
      for (size_t oh = 0; oh < OH; ++oh) {
        for (size_t ow = 0; ow < OW; ++ow) {
          // Input start coordinates
          const int ih_start = static_cast<int>(oh * StrideH) - static_cast<int>(PadH);
          const int iw_start = static_cast<int>(ow * StrideW) - static_cast<int>(PadW);

          E max_val{};
          bool first = true;

          for (size_t kh = 0; kh < KH; ++kh) {
            for (size_t kw = 0; kw < KW; ++kw) {
              int ih = ih_start + static_cast<int>(kh);
              int iw = iw_start + static_cast<int>(kw);

              if (ih >= 0 && ih < static_cast<int>(H) && iw >= 0 && iw < static_cast<int>(W)) {
                E val = input(n, c, static_cast<size_t>(ih), static_cast<size_t>(iw));
                if (first) {
                  max_val = val;
                  first = false;
                } else {
                  if (val > max_val) max_val = val;
                }
              }
            }
          }
          out(n, c, oh, ow) = max_val;
        }
      }
    }
  }
  return out;
}

}  // namespace t81
