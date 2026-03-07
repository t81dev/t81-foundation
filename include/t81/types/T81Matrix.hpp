/**
 * @file T81Matrix.hpp
 * @brief Defines the T81Matrix class for matrices of ternary-native scalars.
 */
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>
#include "t81/math/t81_soft_math/t81_soft_math.hpp"
#include "t81/types/T81Complex.hpp"
#include "t81/types/T81Fixed.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Int.hpp"

namespace t81 {

template <typename T>
concept T81TryteScalar = std::same_as<T, T81Int<81>> || std::same_as<T, T81Float<72, 9>> ||
                         std::same_as<T, T81Fixed<72, 9>> || std::same_as<T, T81Complex<40>>;

template <typename Scalar, size_t Rows, size_t Cols>
  requires T81TryteScalar<Scalar>
class T81Matrix {
public:
  using value_type = Scalar;
  using reference = Scalar&;
  using const_reference = const Scalar&;

  static constexpr size_t rows = Rows;
  static constexpr size_t cols = Cols;
  static constexpr size_t size = Rows * Cols;

  // alignas(64) removed to avoid potential stack alignment issues on some platforms
  Scalar data[Rows * Cols];

private:
  static constexpr auto get_magnitude(const Scalar& x) {
    if constexpr (requires { x.norm(); })
      return x.norm();
    else if constexpr (requires { x.abs(); })
      return x.abs();
    else if constexpr (requires { x.sign_trit(); }) {
      if (x.sign_trit() == Trit::N) return -x;
      return x;
    } else
      return x;
  }

  static constexpr bool is_effectively_zero(const Scalar& x) {
    if constexpr (std::is_floating_point_v<Scalar>) {
      // Standard float/double
      return std::abs(x) < 1e-12;
    } else if constexpr (requires {
                           x.to_double();
                           Scalar::kExponentTrits;
                         }) {
      // T81Float specific check: rely on double conversion + tolerance
      if (std::is_constant_evaluated()) {
        return x == Scalar(0);
      }
      return std::abs(x.to_double()) < 1e-12;
    } else if constexpr (requires {
                           x.real();
                           x.imag();
                         }) {
      // T81Complex check: verify both components
      if (std::is_constant_evaluated()) return x == Scalar(0);
      auto re = x.real();
      auto im = x.imag();
      // Only apply fuzzy check if components support to_double()
      if constexpr (requires { re.to_double(); }) {
        return std::abs(re.to_double()) < 1e-12 && std::abs(im.to_double()) < 1e-12;
      } else {
        return x == Scalar(0);
      }
    } else {
      // T81Fixed, T81Int, and others: exact zero check
      return x == Scalar(0);
    }
  }

public:
  constexpr T81Matrix() noexcept = default;

  explicit constexpr T81Matrix(Scalar fill) noexcept {
    for (size_t i = 0; i < size; ++i) data[i] = fill;
  }

  [[nodiscard]] constexpr Scalar& operator()(size_t r, size_t c) noexcept {
    return data[r * Cols + c];
  }
  [[nodiscard]] constexpr const Scalar& operator()(size_t r, size_t c) const noexcept {
    return data[r * Cols + c];
  }

  [[nodiscard]] constexpr auto transpose() const noexcept -> T81Matrix<Scalar, Cols, Rows> {
    T81Matrix<Scalar, Cols, Rows> t;
    for (size_t i = 0; i < Rows; ++i)
      for (size_t j = 0; j < Cols; ++j) t(j, i) = (*this)(i, j);
    return t;
  }

  [[nodiscard]] constexpr T81Matrix operator+(const T81Matrix& o) const noexcept {
    T81Matrix r;
    for (size_t i = 0; i < size; ++i) r.data[i] = data[i] + o.data[i];
    return r;
  }
  [[nodiscard]] constexpr T81Matrix operator-(const T81Matrix& o) const noexcept {
    T81Matrix r;
    for (size_t i = 0; i < size; ++i) r.data[i] = data[i] - o.data[i];
    return r;
  }
  [[nodiscard]] constexpr T81Matrix operator-() const noexcept {
    T81Matrix r;
    for (size_t i = 0; i < size; ++i) r.data[i] = -data[i];
    return r;
  }

  template <size_t K>
  [[nodiscard]] friend constexpr auto operator*(const T81Matrix<Scalar, Rows, K>& A,
                                                const T81Matrix<Scalar, K, Cols>& B) noexcept
      -> T81Matrix<Scalar, Rows, Cols> {
    T81Matrix<Scalar, Rows, Cols> C(Scalar(0));
    for (size_t i = 0; i < Rows; ++i)
      for (size_t j = 0; j < Cols; ++j) {
        Scalar sum(0);
        for (size_t k = 0; k < K; ++k) sum = sum + A(i, k) * B(k, j);
        C(i, j) = sum;
      }
    return C;
  }

  [[nodiscard]] constexpr T81Matrix operator*(Scalar s) const noexcept {
    T81Matrix r;
    for (size_t i = 0; i < size; ++i) r.data[i] = data[i] * s;
    return r;
  }

  [[nodiscard]] constexpr Scalar determinant() const noexcept
    requires(Rows == Cols)
  {
    if constexpr (Rows == 1) {
      return (*this)(0, 0);
    } else if constexpr (Rows == 2) {
      return (*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0);
    } else if constexpr (Rows == 3) {
      return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) -
             (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0)) +
             (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
    } else {
      // Generic Gaussian elimination for N >= 4
      T81Matrix temp = *this;
      Scalar det = Scalar(1);

      for (size_t i = 0; i < Rows; ++i) {
        size_t pivot = i;
        for (size_t k = i + 1; k < Rows; ++k) {
          if (get_magnitude(temp(k, i)) > get_magnitude(temp(pivot, i))) pivot = k;
        }

        if (pivot != i) {
          for (size_t j = 0; j < Cols; ++j) {
            using std::swap;
            swap(temp(i, j), temp(pivot, j));
          }
          det = -det;
        }

        if (is_effectively_zero(temp(i, i))) return Scalar(0);

        det = det * temp(i, i);
        Scalar div = temp(i, i);
        Scalar inv = Scalar(1) / div;

        for (size_t k = i + 1; k < Rows; ++k) {
          Scalar factor = temp(k, i) * inv;
          for (size_t j = i + 1; j < Cols; ++j) {
            temp(k, j) = temp(k, j) - factor * temp(i, j);
          }
        }
      }
      return det;
    }
  }

  [[nodiscard]] constexpr T81Matrix inverse() const noexcept
    requires(Rows == Cols)
  {
    T81Matrix res;
    // Note: For integer types, division performs floor/truncation.

    if constexpr (Rows == 1) {
      Scalar val = (*this)(0, 0);
      if (val == Scalar(0)) return T81Matrix();
      res(0, 0) = Scalar(1) / val;
    } else if constexpr (Rows == 2) {
      Scalar det = determinant();
      if (det == Scalar(0)) return T81Matrix();
      res(0, 0) = (*this)(1, 1) / det;
      res(0, 1) = -(*this)(0, 1) / det;
      res(1, 0) = -(*this)(1, 0) / det;
      res(1, 1) = (*this)(0, 0) / det;
    } else if constexpr (Rows == 3) {
      Scalar det = determinant();
      if (det == Scalar(0)) return T81Matrix();
      // Transpose of cofactor matrix divided by det
      res(0, 0) = ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) / det;
      res(0, 1) = ((*this)(0, 2) * (*this)(2, 1) - (*this)(0, 1) * (*this)(2, 2)) / det;
      res(0, 2) = ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)) / det;

      res(1, 0) = ((*this)(1, 2) * (*this)(2, 0) - (*this)(1, 0) * (*this)(2, 2)) / det;
      res(1, 1) = ((*this)(0, 0) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 0)) / det;
      res(1, 2) = ((*this)(1, 0) * (*this)(0, 2) - (*this)(0, 0) * (*this)(1, 2)) / det;

      res(2, 0) = ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)) / det;
      res(2, 1) = ((*this)(2, 0) * (*this)(0, 1) - (*this)(0, 0) * (*this)(2, 1)) / det;
      res(2, 2) = ((*this)(0, 0) * (*this)(1, 1) - (*this)(1, 0) * (*this)(0, 1)) / det;
    } else {
      // Gaussian elimination with augmented matrix [A|I]
      T81Matrix work = *this;
      T81Matrix inv(Scalar(0));
      for (size_t i = 0; i < Rows; ++i) inv(i, i) = Scalar(1);

      for (size_t i = 0; i < Rows; ++i) {
        size_t pivot = i;
        for (size_t k = i + 1; k < Rows; ++k) {
          if (get_magnitude(work(k, i)) > get_magnitude(work(pivot, i))) pivot = k;
        }

        if (is_effectively_zero(work(pivot, i))) return T81Matrix();  // Singular

        if (pivot != i) {
          for (size_t j = 0; j < Cols; ++j) {
            using std::swap;
            swap(work(i, j), work(pivot, j));
            swap(inv(i, j), inv(pivot, j));
          }
        }

        Scalar div = work(i, i);
        Scalar scale = Scalar(1) / div;

        for (size_t j = 0; j < Cols; ++j) {
          work(i, j) = work(i, j) * scale;
          inv(i, j) = inv(i, j) * scale;
        }

        for (size_t k = 0; k < Rows; ++k) {
          if (k != i) {
            Scalar f = work(k, i);
            for (size_t j = 0; j < Cols; ++j) {
              work(k, j) = work(k, j) - f * work(i, j);
              inv(k, j) = inv(k, j) - f * inv(i, j);
            }
          }
        }
      }
      return inv;
    }
    return res;
  }

  [[nodiscard]] constexpr auto operator<=>(const T81Matrix&) const noexcept = default;
  [[nodiscard]] constexpr bool operator==(const T81Matrix&) const noexcept = default;
};

using float81 = T81Float<72, 9>;
using fixed81 = T81Fixed<72, 9>;
using sym81 = T81Int<81>;

using Mat4x4 = T81Matrix<float81, 4, 4>;
using Mat3x3 = T81Matrix<float81, 3, 3>;
// Mat81x81 removed to avoid conflict with T81Tensor.hpp

template <typename S, size_t R, size_t C>
[[nodiscard]] constexpr auto transpose(const T81Matrix<S, R, C>& m) noexcept -> T81Matrix<S, C, R> {
  return m.transpose();
}

template <typename S, size_t N>
[[nodiscard]] constexpr T81Matrix<S, N, N> identity() noexcept {
  T81Matrix<S, N, N> I(S(0));
  for (size_t i = 0; i < N; ++i) I(i, i) = S(1);
  return I;
}

}  // namespace t81
