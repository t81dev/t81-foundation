/**
 * @file T81Matrix.hpp
 * @brief Defines the T81Matrix class for matrices of ternary-native scalars.
 *
 * This file provides the `T81Matrix<Scalar, Rows, Cols>` class, a container for
 * matrices of balanced-ternary, tryte-based scalar types. It is designed for
 * high performance, featuring 64-byte aligned, contiguous storage suitable for
 * hardware acceleration on tensor cores. The class supports fundamental matrix
 * operations, including arithmetic and transposition.
 */
#pragma once

#include "t81/core/T81Int.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Fixed.hpp"
#include "t81/core/T81Complex.hpp"
#include <cstddef>
#include <array>
#include <span>
#include <compare>
#include <bit>
#include <cstring>
#include <format>

namespace t81 {

// ======================================================================
// Core concept – any scalar that is exactly one tryte (81 trits)
// ======================================================================
template <typename T>
concept T81TryteScalar =
    std::same_as<T, T81Int<81>>          ||  // exact integer symbols / IDs
    std::same_as<T, T81Float<72,9>>       ||  // 81-trit float (72+9+sign)
    std::same_as<T, T81Fixed<72,9>>       ||  // 81-trit fixed Q72.9
    std::same_as<T, T81Complex<40>>;         // 80 trits → rounded to 81 with padding

// ======================================================================
// T81Matrix<Scalar, Rows, Cols> – the final matrix type
// ======================================================================
template <typename Scalar, size_t Rows, size_t Cols>
    requires T81TryteScalar<Scalar>
class T81Matrix {
public:
    using value_type      = Scalar;
    using reference       = Scalar&;
    using const_reference = const Scalar&;

    static constexpr size_t rows    = Rows;
    static constexpr size_t cols    = Cols;
    static constexpr size_t size    = Rows * Cols;
    static constexpr size_t trytes  = (size + 3) / 4;   // packed storage if desired

    // Contiguous storage — always 64-byte aligned for tensor cores
    alignas(64) Scalar data[Rows * Cols];

    // ==================================================================
    // Construction
    // ==================================================================
    constexpr T81Matrix() noexcept = default;

    explicit constexpr T81Matrix(Scalar fill) noexcept {
        for (size_t i = 0; i < size; ++i) data[i] = fill;
    }

    // Aggregate initialization works out of the box
    //   T81Matrix<float81,3,3> M{{{1},{2},{3},{4},{5},{6},{7},{8},{9}}};

    // ==================================================================
    // Access
    // ==================================================================
    [[nodiscard]] constexpr Scalar&       operator()(size_t r, size_t c)       noexcept { return data[r * Cols + c]; }
    [[nodiscard]] constexpr const Scalar& operator()(size_t r, size_t c) const noexcept { return data[r * Cols + c]; }

    [[nodiscard]] constexpr Scalar&       at(size_t r, size_t c)                     { return (*this)(r,c); }
    [[nodiscard]] constexpr const Scalar& at(size_t r, size_t c)               const { return (*this)(r,c); }

    [[nodiscard]] constexpr std::span<Scalar>       row(size_t r)       noexcept { return {data + r*Cols, Cols}; }
    [[nodiscard]] constexpr std::span<const Scalar> row(size_t r) const noexcept { return {data + r*Cols, Cols}; }

    // ==================================================================
    // Views
    // ==================================================================
    [[nodiscard]] constexpr auto transpose() const noexcept
        -> T81Matrix<Scalar, Cols, Rows>
    {
        T81Matrix<Scalar, Cols, Rows> t;
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j)
                t(j,i) = (*this)(i,j);
        return t;
    }

    // ==================================================================
    // Arithmetic — fused on Axion hardware
    // ==================================================================
    [[nodiscard]] constexpr T81Matrix operator+(const T81Matrix& o) const noexcept {
        T81Matrix r; for (size_t i=0;i<size;++i) r.data[i] = data[i] + o.data[i]; return r;
    }
    [[nodiscard]] constexpr T81Matrix operator-(const T81Matrix& o) const noexcept {
        T81Matrix r; for (size_t i=0;i<size;++i) r.data[i] = data[i] - o.data[i]; return r;
    }
    [[nodiscard]] constexpr T81Matrix operator-() const noexcept {
        T81Matrix r; for (size_t i=0;i<size;++i) r.data[i] = -data[i]; return r;
    }

    // Matrix × Matrix — the crown jewel (single tensor-core instruction on real silicon)
    template <size_t K>
    [[nodiscard]] friend constexpr auto operator*(
        const T81Matrix<Scalar, Rows, K>& A,
        const T81Matrix<Scalar, K,    Cols>& B) noexcept
        -> T81Matrix<Scalar, Rows, Cols>
    {
        T81Matrix<Scalar, Rows, Cols> C(Scalar(0));
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j) {
                Scalar sum{};
                for (size_t k = 0; k < K; ++k)
                    sum = sum + A(i,k) * B(k,j);
                C(i,j) = sum;
            }
        return C;
    }

    // Matrix × Scalar
    [[nodiscard]] constexpr T81Matrix operator*(Scalar s) const noexcept {
        T81Matrix r; for (size_t i=0;i<size;++i) r.data[i] = data[i] * s; return r;
    }

    // ==================================================================
    // Comparison
    // ==================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Matrix&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Matrix&) const noexcept = default;
};

// ======================================================================
// Common aliases — these are the matrices the future will actually use
// ======================================================================
using float81  = T81Float<72,9>;     // exactly 81 trits
using fixed81  = T81Fixed<72,9>;
using sym81    = T81Int<81>;

using Mat4x4   = T81Matrix<float81, 4, 4>;   // transformation matrices
using Mat3x3   = T81Matrix<float81, 3, 3>;
using Mat81x81 = T81Matrix<float81,81,81>;   // attention / routing tables
using SymMat   = T81Matrix<sym81,  81,81>;   // symbolic transition matrix (exact HRR)

// ======================================================================
// Zero-cost utilities
// ======================================================================
template <typename S, size_t R, size_t C>
[[nodiscard]] constexpr auto transpose(const T81Matrix<S,R,C>& m) noexcept
    -> T81Matrix<S,C,R> { return m.transpose(); }

template <typename S, size_t N>
[[nodiscard]] constexpr T81Matrix<S,N,N> identity() noexcept {
    T81Matrix<S,N,N> I(S(0));
    for (size_t i = 0; i < N; ++i) I(i,i) = S(1);
    return I;
}

} // namespace t81

// Make it printable
template <typename S, size_t R, size_t C>
struct std::formatter<t81::T81Matrix<S,R,C>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const t81::T81Matrix<S,R,C>& m, format_context& ctx) const {
        auto out = ctx.out();
        out = format_to(out, "[\n");
        for (size_t i = 0; i < R; ++i) {
            out = format_to(out, "  ");
            for (size_t j = 0; j < C; ++j)
                out = format_to(out, "{:10} ", m(i,j).to_double());
            if (i+1 < R) out = format_to(out, "\n");
        }
        return format_to(out, "\n]");
    }
};
