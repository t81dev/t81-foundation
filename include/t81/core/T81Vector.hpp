/**
 * @file T81Vector.hpp
 * @brief Defines the T81Vector class, a mathematical vector with physical semantics.
 *
 * This file provides the `T81Vector<N, Scalar>` class, a fixed-size N-dimensional
 * vector designed as a geometric object rather than a general-purpose container.
 * It supports a rich set of constexpr-friendly mathematical and geometric
 * operations, including dot and cross products, normalization, and rotations
 * via `T81Quaternion`.
 */
#pragma once

#include "t81/core/T81Tensor.hpp"
#include "t81/core/T81Quaternion.hpp"
#include "t81/core/T81Float.hpp"

#include <cstddef>
#include <compare>
#include <cmath>
#include <concepts>
#include <type_traits>
#include <cstring>

namespace t81 {

// ======================================================================
// T81Vector<N, Scalar = T81Float<72,9>> – N-dimensional vector
// ======================================================================
template <std::size_t N, typename Scalar = T81Float<72,9>>
class T81Vector {
    static_assert(N >= 1 && N <= 81, "T81Vector dimension must be 1–81");

    alignas(64) Scalar components_[N];

public:
    using value_type = Scalar;
    static constexpr std::size_t dimension = N;

    //===================================================================
    // Construction
    // (all constexpr, zero-overhead)
    //===================================================================
    constexpr T81Vector() noexcept = default;

    explicit constexpr T81Vector(Scalar fill) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            components_[i] = fill;
        }
    }

    // Variadic constructor – the natural way
    template <typename... Components>
        requires (sizeof...(Components) == N) && (std::convertible_to<Components, Scalar> && ...)
    constexpr T81Vector(Components... comps) noexcept
        : components_{ static_cast<Scalar>(comps)... } {}

    // From raw array
    static constexpr T81Vector from_array(const Scalar* data) noexcept {
        T81Vector v;
        for (std::size_t i = 0; i < N; ++i) v[i] = data[i];
        return v;
    }

    //===================================================================
    // Access
    //===================================================================
    [[nodiscard]] constexpr Scalar&       operator[](std::size_t i)       noexcept { return components_[i]; }
    [[nodiscard]] constexpr const Scalar& operator[](std::size_t i) const noexcept { return components_[i]; }

    [[nodiscard]] constexpr Scalar*       data()       noexcept { return components_; }
    [[nodiscard]] constexpr const Scalar* data() const noexcept { return components_; }

    //===================================================================
    // Common vectors
    //===================================================================
    [[nodiscard]] static constexpr T81Vector zero() noexcept {
        return T81Vector(Scalar(0));
    }

    [[nodiscard]] static constexpr T81Vector one() noexcept {
        return unit_vector<0>();
    }

    template <std::size_t I>
    [[nodiscard]] static constexpr T81Vector unit_vector() noexcept {
        static_assert(I < N, "Unit vector index out of range");
        T81Vector v = zero();
        v[I] = Scalar(1);
        return v;
    }

    //===================================================================
    // Arithmetic
    //===================================================================
    [[nodiscard]] constexpr T81Vector operator+(const T81Vector& o) const noexcept {
        T81Vector r;
        for (std::size_t i = 0; i < N; ++i) r[i] = components_[i] + o[i];
        return r;
    }

    [[nodiscard]] constexpr T81Vector operator-(const T81Vector& o) const noexcept {
        T81Vector r;
        for (std::size_t i = 0; i < N; ++i) r[i] = components_[i] - o[i];
        return r;
    }

    [[nodiscard]] constexpr T81Vector operator-() const noexcept {
        T81Vector r;
        for (std::size_t i = 0; i < N; ++i) r[i] = -components_[i];
        return r;
    }

    [[nodiscard]] constexpr T81Vector operator*(Scalar s) const noexcept {
        T81Vector r;
        for (std::size_t i = 0; i < N; ++i) r[i] = components_[i] * s;
        return r;
    }

    friend constexpr T81Vector operator*(Scalar s, const T81Vector& v) noexcept {
        return v * s;
    }

    //===================================================================
    // Geometric operations
    //===================================================================
    [[nodiscard]] constexpr Scalar dot(const T81Vector& o) const noexcept {
        Scalar sum(0);
        for (std::size_t i = 0; i < N; ++i) sum += components_[i] * o[i];
        return sum;
    }

    [[nodiscard]] constexpr Scalar length2() const noexcept { return dot(*this); }

    [[nodiscard]] Scalar length() const noexcept { return length2().sqrt(); }

    [[nodiscard]] T81Vector normalized() const noexcept {
        const Scalar len = length();
        return len.is_zero() ? *this : *this * (Scalar(1) / len);
    }

    // Cross product – only for 3D
    template <std::size_t M = N>
    [[nodiscard]] std::enable_if_t<M == 3, T81Vector> cross(const T81Vector& o) const noexcept {
        return T81Vector(
            components_[1] * o[2] - components_[2] * o[1],
            components_[2] * o[0] - components_[0] * o[2],
            components_[0] * o[1] - components_[1] * o[0]
        );
    }

    // Angle between vectors
    [[nodiscard]] Scalar angle(const T81Vector& o) const noexcept {
        return (dot(o) / (length() * o.length())).acos();
    }

    // Projection
    [[nodiscard]] T81Vector project_onto(const T81Vector& o) const noexcept {
        return o * (dot(o) / o.length2());
    }

    //===================================================================
    // Rotation (using T81Quaternion)
    //===================================================================
    template <std::size_t M = N>
    [[nodiscard]] std::enable_if_t<M == 3, T81Vector> rotated(const T81Quaternion& q) const noexcept {
        T81Quaternion vq(Scalar(0), components_[0], components_[1], components_[2]);
        T81Quaternion result = q * vq * q.conj();
        return T81Vector(result.x(), result.y(), result.z());
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Vector&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Vector&) const noexcept = default;

    //===================================================================
    // Conversion to tensor (interoperability)
    //===================================================================
    [[nodiscard]] operator T81Tensor<Scalar, 1, N>() const noexcept {
        T81Tensor<Scalar, 1, N> t{};
        std::memcpy(t.data, components_, sizeof(components_));
        return t;
    }
};

// ======================================================================
// Common vector types used in the ternary civilization
// ======================================================================
using Vec2  = T81Vector<2>;
using Vec3  = T81Vector<3>;
using Vec4  = T81Vector<4>;
using Vec81 = T81Vector<81>;

using Vec2f = T81Vector<2, T81Float<72,9>>;
using Vec3f = T81Vector<3, T81Float<72,9>>;
using Vec4f = T81Vector<4, T81Float<72,9>>;

// ======================================================================
// Example: This is how the future moves through space
// ======================================================================
/*
constexpr Vec3 up(0, 1, 0);
constexpr Vec3 forward(0, 0, 1);
constexpr auto right = up.cross(forward);  // (1,0,0)

constexpr auto rotation =
    T81Quaternion::from_axis_angle(0, 1, 0, T81Float<72,9>(3.14159 / 2));
constexpr auto left = right.rotated(rotation);  // (0,0,-1) exactly
*/

} // namespace t81
