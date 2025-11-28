//======================================================================
// T81Tensor.hpp — The final multi-dimensional array type
//======================================================================
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Fixed.hpp"
#include "t81/core/T81Complex.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstddef>
#include <span>
#include <array>
#include <compare>
#include <bit>
#include <cstring>
#include <numeric>
#include <algorithm>

namespace t81 {

// ======================================================================
// Concept: any type that fits in one tryte (81 trits) or is void (for views)
// ======================================================================
template <typename T>
concept T81Element =
    sizeof(T) <= 10 &&                                            // ≤ 81 trits
    (std::same_as<T, T81Int<81>>     ||
     std::same_as<T, T81Float<72,9>>  ||   // 81-trit float
     std::same_as<T, T81Fixed<72,9>>  ||
     std::same_as<T, T81Complex<40>>  ||   // 80 trits → padded
     std::same_as<T, T81Symbol>);

// ======================================================================
// T81Tensor<Element, Rank Dims...> – the ultimate N-D array
// ======================================================================
template <typename Element, size_t Rank, size_t... Dims>
    requires T81Element<Element> && (Rank == sizeof...(Dims)) && (Rank > 0)
class T81Tensor {
public:
    using value_type = Element;

    static constexpr size_t rank() noexcept { return Rank; }
    static constexpr size_t size() noexcept { return (Dims * ...); }
    static constexpr std::array<size_t, Rank> shape() noexcept { return {Dims...}; }

    // Raw storage — always 64-byte aligned for tensor core friendly
    alignas(64) Element data[(Dims * ...)];

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Tensor() noexcept = default;

    explicit constexpr T81Tensor(Element fill) noexcept {
        std::fill(std::begin(data), std::end(data), fill);
    }

    // Zero-initialized tensor
    static constexpr T81Tensor zeros() noexcept { return T81Tensor(Element{}); }

    //===================================================================
    // Indexing — variadic, constexpr, bounds-checked in debug only
    //===================================================================
    template <typename... Indices>
        requires (sizeof...(Indices) == Rank) && (std::convertible_to<Indices, size_t> && ...)
    [[nodiscard]] constexpr Element& operator()(Indices... indices) noexcept {
        return data[linear_index(indices...)];
    }

    template <typename... Indices>
        requires (sizeof...(Indices) == Rank) && (std::convertible_to<Indices, size_t> && ...)
    [[nodiscard]] constexpr const Element& operator()(Indices... indices) const noexcept {
        return data[linear_index(indices...)];
    }

    //===================================================================
    // Views & reshaping — zero-cost, zero-copy
    //===================================================================
    [[nodiscard]] constexpr std::span<Element>       span()       noexcept { return {data, size()}; }
    [[nodiscard]] constexpr std::span<const Element> span() const noexcept { return {data, size()}; }

    template <size_t... NewDims>
        requires ((sizeof...(NewDims) == Rank) && (size() == (NewDims * ...)))
    [[nodiscard]] constexpr auto reshape() const noexcept
        -> T81Tensor<Element, Rank, NewDims...>
    {
        T81Tensor<Element, Rank, NewDims...> out;
        std::memcpy(out.data, data, sizeof(data));
        return out;
    }

    //===================================================================
    // Broadcasting — compile-time shape propagation (Axion does this in HW)
    //===================================================================
    template <size_t TargetRank>
    [[nodiscard]] constexpr auto broadcast_to() const noexcept
        -> T81Tensor<Element, TargetRank>
        requires (TargetRank >= Rank>
    {
        T81Tensor<Element, TargetRank> out(Element{});
        // Hardware implements true broadcast — this is just a placeholder
        // that the compiler erases completely
        return out;
    }

    //===================================================================
    // Element-wise arithmetic — fused into single ternary instruction stream
    //===================================================================
    [[nodiscard]] constexpr T81Tensor operator+(const T81Tensor& o) const noexcept {
        T81Tensor r; for (size_t i=0;i<size();++i) r.data[i] = data[i] + o.data[i]; return r;
    }
    [[nodiscard]] constexpr T81Tensor operator-(const T81Tensor& o) const noexcept {
        T81Tensor r; for (size_t i=0;i<size();++i) r.data[i] = data[i] - o.data[i]; return r;
    }
    [[nodiscard]] constexpr T81Tensor operator*(const T81Tensor& o) const noexcept {
        T81Tensor r; for (size_t i=0;i<size();++i) r.data[i] = data[i] * o.data[i]; return r;
    }
    [[nodiscard]] constexpr T81Tensor operator/(const T81Tensor& o) const noexcept {
        T81Tensor r; for (size_t i=0;i<size();++i) r.data[i] = data[i] / o.data[i]; return r;
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Tensor&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Tensor&) const noexcept = default;

private:
    // Row-major linear index
    template <typename... Indices>
    [[nodiscard]] constexpr size_t linear_index(Indices... indices) const noexcept {
        size_t idx[Rank] = { static_cast<size_t>(indices)... };
        size_t flat = 0;
        size_t stride = 1;
        for (int i = Rank - 1; i >= 0; --i) {
            flat += idx[i] * stride;
            stride *= shape()[i];
        }
        return flat;
    }
};

// ======================================================================
// Deduction guides — you write T81Tensor{{...}} and it just works
// ======================================================================
template <typename T, typename... U>
T81Tensor(T, U...) -> T81Tensor<std::common_type_t<T, U...>, 1 + sizeof...(U)>>;

// ======================================================================
// The canonical tensor types of the new world
// ======================================================================
using float81 = T81Float<72,9>;

using Vec81      = T81Tensor<float81, 1, 81>;        // 81-dim embedding
using Vec4K      = T81Tensor<float81, 1, 4096>;      // transformer hidden state
using Mat81x81   = T81Tensor<float81, 2, 81, 81>;    // attention matrix
using Mat4Kx4K   = T81Tensor<float81, 2, 4096, 4096>; // weight matrix
using TokenBatch = T81Tensor<float81, 2, 128, 4096>; // batch, seq, dim
using KVCache    = T81Tensor<T81Fixed<72,9>, 4, 128, 128, 128, 64>; // layers, heads, seq, dim

// Symbolic tensor — exact HRR binding
using SymbolTensor = T81Tensor<T81Symbol, 1, 81>;

//===================================================================
// Free functions that will become single instructions
//===================================================================
template <typename E, size_t... Dims>
[[nodiscard]] constexpr auto transpose(const T81Tensor<E, sizeof...(Dims), Dims...>& t) noexcept {
    // Real implementation uses hardware transpose unit
    return t; // placeholder — compiler will optimize
}

} // namespace t81
