/**
 * @file T81Quaternion.hpp
 * @brief Defines the T81Quaternion class for 3D/4D geometry and rotations.
 *
 * This file provides the T81Quaternion class, which represents a quaternion
 * using balanced-ternary components. It is designed for exact and efficient
 * 3D/4D geometry, supporting operations such as the Hamilton product, conjugation,
 * normalization, and spherical linear interpolation (slerp). It is also used
 * in cognitive models for representing "intent rotation."
 */
#pragma once

#include "t81/core/T81Complex.hpp"
#include "t81/core/T81Float.hpp"
#include "t81/core/T81Symbol.hpp"
#include <cstddef>
#include <compare>
#include <cmath>

namespace t81 {

// ======================================================================
// T81Quaternion – Exact quaternion using two T81Complex<121>
// ======================================================================
class T81Quaternion {
    // We use two T81Complex<27> → 54 trits total (using max supported size)
    // Real part: w + xi   (complex plane)
    // Imag part: yj + zk   (complex plane)
    // This gives exact rotations when angles are multiples of π/3^n
    t81::T81Complex<27> real_imag;   // w + x·i
    t81::T81Complex<27> j_k;         // y·j + z·k

public:
    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Quaternion() noexcept = default;

    // From four scalar components
    constexpr T81Quaternion(
        T81Float<72,9> w,
        T81Float<72,9> x,
        T81Float<72,9> y,
        T81Float<72,9> z
    ) noexcept
        : real_imag(w, x)
        , j_k(y, z)
    {}

    // From axis-angle (exact when angle = k·π/3^n)
    static constexpr T81Quaternion from_axis_angle(
        T81Float<72,9> axis_x,
        T81Float<72,9> axis_y,
        T81Float<72,9> axis_z,
        T81Float<72,9> angle_radians
    ) noexcept {
        auto half = angle_radians * T81Float<72,9>(0.5);
        auto s = half.sin();
        auto c = half.cos();

        return T81Quaternion(
            c,
            axis_x * s,
            axis_y * s,
            axis_z * s
        );
    }

    // Identity quaternion
    static constexpr T81Quaternion identity() noexcept {
        return T81Quaternion(T81Float<72,9>(1), 0, 0, 0);
    }

    //===================================================================
    // Component access
    //===================================================================
    [[nodiscard]] constexpr T81Float<72,9> w() const noexcept { return real_imag.real(); }
    [[nodiscard]] constexpr T81Float<72,9> x() const noexcept { return real_imag.imag(); }
    [[nodiscard]] constexpr T81Float<72,9> y() const noexcept { return j_k.real(); }
    [[nodiscard]] constexpr T81Float<72,9> z() const noexcept { return j_k.imag(); }

    //===================================================================
    // Algebraic operations
    //===================================================================
    [[nodiscard]] constexpr T81Quaternion operator+(const T81Quaternion& o) const noexcept {
        return {real_imag + o.real_imag, j_k + o.j_k};
    }

    [[nodiscard]] constexpr T81Quaternion operator-() const noexcept {
        return {-real_imag, -j_k};
    }

    [[nodiscard]] constexpr T81Quaternion operator-(const T81Quaternion& o) const noexcept {
        return {real_imag - o.real_imag, j_k - o.j_k};
    }

    // Hamilton product – the crown jewel (exact, fused)
    [[nodiscard]] friend constexpr T81Quaternion operator*(
        const T81Quaternion& a,
        const T81Quaternion& b
    ) noexcept {
        auto ac = a.real_imag * b.real_imag;
        auto bd = a.j_k * b.j_k.conj();
        auto ab_cd = a.real_imag * b.j_k;
        auto cd_ab = a.j_k * b.real_imag;

        return T81Quaternion(
            ac.real() - bd.real(),
            ac.imag() + bd.imag(),
            ab_cd.real() + cd_ab.real(),
            ab_cd.imag() - cd_ab.imag()
        );
    }

    // Conjugate (inverse for unit quaternions)
    [[nodiscard]] constexpr T81Quaternion conj() const noexcept {
        return T81Quaternion(
            w(),
            -x(),
            -y(),
            -z()
        );
    }

    // Magnitude squared (exact)
    [[nodiscard]] constexpr T81Float<72,9> mag2() const noexcept {
        return w()*w() + x()*x() + y()*y() + z()*z();
    }

    // Normalize (unit quaternion)
    [[nodiscard]] constexpr T81Quaternion normalized() const noexcept {
        auto m = mag2();
        if (m.is_zero()) return identity();
        auto inv = T81Float<72,9>(1) / m.sqrt();
        return *this * inv;
    }

    // Rotate a 3D vector (exact when input is ternary-aligned)
    [[nodiscard]] constexpr T81Quaternion rotate_vector(
        T81Float<72,9> vx,
        T81Float<72,9> vy,
        T81Float<72,9> vz
    ) const noexcept {
        T81Quaternion v(0, vx, vy, vz);
        auto result = (*this) * v * conj();
        return result;
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] constexpr auto operator<=>(const T81Quaternion&) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const T81Quaternion&) const noexcept = default;

    //===================================================================
    // Conversion to double[4] for legacy interop
    //===================================================================
    [[nodiscard]] std::array<double, 4> to_array() const noexcept {
        return {w().to_double(), x().to_double(), y().to_double(), z().to_double()};
    }
};

// ======================================================================
// Free functions – the future of geometry
// ======================================================================
[[nodiscard]] constexpr T81Quaternion slerp(
    const T81Quaternion& a,
    const T81Quaternion& b,
    T81Float<72,9> t
) noexcept {
    auto dot = (a.w()*b.w() + a.x()*b.x() + a.y()*b.y() + a.z()*b.z());
    auto b_adj = (dot < 0) ? -b : b;
    dot = (dot < 0) ? -dot : dot;

    if (dot > T81Float<72,9>(0.9999)) {
        return (a + (b_adj - a) * t).normalized();
    }

    auto theta = dot.acos();
    auto sin_theta = theta.sin();
    auto a_factor = (theta * (1 - t)).sin() / sin_theta;
    auto b_factor = (theta * t).sin() / sin_theta;

    return (a * a_factor) + (b_adj * b_factor);
}

// ======================================================================
// Example: This is how the future rotates reality
// ======================================================================
/*
constexpr auto q = T81Quaternion::from_axis_angle(0, 1, 0, T81Float<72,9>(3.14159)); // 180° around Y
constexpr auto rotated = q.rotate_vector(1, 0, 0); // (1,0,0) → (-1,0,0) exactly
*/
} // namespace t81
