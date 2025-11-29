/**
 * @file T81Quaternion.hpp
 * @brief Defines the T81Quaternion class for 3D/4D geometry and rotations.
 *
 * This file provides the T81Quaternion class, which represents a quaternion
 * using balanced-ternary components. It is designed for exact and efficient
 * 3D/4D geometry, supporting operations such as the Hamilton product,
 * conjugation, normalization, and spherical linear interpolation (slerp).
 * It is also used in cognitive models for representing "intent rotation."
 */
#pragma once

#include "t81/core/T81Complex.hpp"
#include "t81/core/T81Float.hpp"

#include <array>
#include <cstddef>
#include <cmath>

namespace t81 {

// ======================================================================
// T81Quaternion – Exact quaternion using two T81Complex<27>
// ======================================================================
//
// Representation:
//   We encode the quaternion components (w, x, y, z) as two complex numbers:
//     real_imag = w + x·i
//     j_k       = y + z·i
//
//   This keeps the storage compact and naturally aligned with the
//   existing balanced-ternary complex type.
// ======================================================================
class T81Quaternion {
public:
    using Scalar  = T81Float<27, 9>;
    using Complex = T81Complex<27>;

private:
    // w + x·i
    Complex real_imag_{};
    // y + z·i
    Complex j_k_{};

    // Internal constructor from complex pairs
    inline T81Quaternion(const Complex& ri, const Complex& jk) noexcept
        : real_imag_(ri)
        , j_k_(jk) {}

public:
    //===================================================================
    // Construction
    //===================================================================
    inline T81Quaternion() noexcept = default;

    // From four scalar components
    inline T81Quaternion(
        const Scalar& w,
        const Scalar& x,
        const Scalar& y,
        const Scalar& z
    ) noexcept
        : real_imag_(w, x)
        , j_k_(y,   z) {}

    // From axis-angle (axis is expected to be normalized).
    // Exact when angle = k·π/3^n in the underlying ternary representation.
    static inline T81Quaternion from_axis_angle(
        const Scalar& axis_x,
        const Scalar& axis_y,
        const Scalar& axis_z,
        const Scalar& angle_radians
    ) noexcept {
        const Scalar half = angle_radians * Scalar(0.5);
        const Scalar s    = half.sin();
        const Scalar c    = half.cos();

        return T81Quaternion(
            c,
            axis_x * s,
            axis_y * s,
            axis_z * s
        );
    }

    // Identity quaternion
    static inline T81Quaternion identity() noexcept {
        return T81Quaternion(Scalar(1), Scalar(0), Scalar(0), Scalar(0));
    }

    //===================================================================
    // Component access
    //===================================================================
    [[nodiscard]] inline Scalar w() const noexcept { return real_imag_.real(); }
    [[nodiscard]] inline Scalar x() const noexcept { return real_imag_.imag(); }
    [[nodiscard]] inline Scalar y() const noexcept { return j_k_.real(); }
    [[nodiscard]] inline Scalar z() const noexcept { return j_k_.imag(); }

    //===================================================================
    // Algebraic operations (component-wise + Hamilton product)
    //===================================================================
    [[nodiscard]] inline T81Quaternion operator+(
        const T81Quaternion& o
    ) const noexcept {
        return T81Quaternion(real_imag_ + o.real_imag_, j_k_ + o.j_k_);
    }

    [[nodiscard]] inline T81Quaternion operator-() const noexcept {
        return T81Quaternion(-real_imag_, -j_k_);
    }

    [[nodiscard]] inline T81Quaternion operator-(
        const T81Quaternion& o
    ) const noexcept {
        return T81Quaternion(real_imag_ - o.real_imag_, j_k_ - o.j_k_);
    }

    // Hamilton product
    [[nodiscard]] friend inline T81Quaternion operator*(
        const T81Quaternion& a,
        const T81Quaternion& b
    ) noexcept {
        // Let:
        //   a = (aw + ax i) + (ay j + az k)
        //   b = (bw + bx i) + (by j + bz k)
        //
        // We use complex arithmetic plus conjugation tricks to encode the
        // Hamilton product in terms of Complex operations.
        const Complex ac    = a.real_imag_ * b.real_imag_;
        const Complex bd    = a.j_k_ * b.j_k_.conj();
        const Complex ab_cd = a.real_imag_ * b.j_k_;
        const Complex cd_ab = a.j_k_ * b.real_imag_;

        const Scalar w = ac.real() - bd.real();
        const Scalar x = ac.imag() + bd.imag();
        const Scalar y = ab_cd.real() + cd_ab.real();
        const Scalar z = ab_cd.imag() - cd_ab.imag();

        return T81Quaternion(w, x, y, z);
    }

    // Scalar multiplication (Quaternion * Scalar)
    [[nodiscard]] friend inline T81Quaternion operator*(
        const T81Quaternion& q,
        const Scalar& s
    ) noexcept {
        return T81Quaternion(
            q.w() * s,
            q.x() * s,
            q.y() * s,
            q.z() * s
        );
    }

    // Scalar multiplication (Scalar * Quaternion)
    [[nodiscard]] friend inline T81Quaternion operator*(
        const Scalar& s,
        const T81Quaternion& q
    ) noexcept {
        return q * s;
    }

    // Conjugate (inverse if |q| == 1)
    [[nodiscard]] inline T81Quaternion conj() const noexcept {
        return T81Quaternion(
            w(),
            -x(),
            -y(),
            -z()
        );
    }

    // Magnitude squared
    [[nodiscard]] inline Scalar mag2() const noexcept {
        return w() * w() + x() * x() + y() * y() + z() * z();
    }

    // Normalize (unit quaternion). If length is zero, returns identity().
    [[nodiscard]] inline T81Quaternion normalized() const noexcept {
        const Scalar m = mag2();
        if (m.is_zero()) {
            return identity();
        }
        const Scalar inv_len = Scalar(1) / m.sqrt();
        return *this * inv_len;
    }

    // Rotate a 3D vector.
    //
    // Interprets (vx, vy, vz) as a "pure" quaternion v = (0, vx, vy, vz),
    // then returns q * v * q.conj(). The returned quaternion has w ≈ 0,
    // and the rotated vector lives in {x, y, z}.
    [[nodiscard]] inline T81Quaternion rotate_vector(
        const Scalar& vx,
        const Scalar& vy,
        const Scalar& vz
    ) const noexcept {
        const T81Quaternion v(Scalar(0), vx, vy, vz);
        const T81Quaternion result = (*this) * this->conj() * v;
        return result;
    }

    //===================================================================
    // Comparison
    //===================================================================
    [[nodiscard]] inline bool operator==(
        const T81Quaternion& o
    ) const noexcept {
        return w() == o.w()
            && x() == o.x()
            && y() == o.y()
            && z() == o.z();
    }

    [[nodiscard]] inline bool operator!=(
        const T81Quaternion& o
    ) const noexcept {
        return !(*this == o);
    }

    //===================================================================
    // Conversion to double[4] for legacy interop
    //===================================================================
    [[nodiscard]] inline std::array<double, 4> to_array() const noexcept {
        return {
            w().to_double(),
            x().to_double(),
            y().to_double(),
            z().to_double()
        };
    }
};

// ======================================================================
// Free functions – geometric utilities
// ======================================================================

[[nodiscard]] inline T81Quaternion slerp(
    const T81Quaternion& a,
    const T81Quaternion& b,
    const T81Quaternion::Scalar& t
) noexcept {
    using Scalar = T81Quaternion::Scalar;

    // Cosine of the angle between them
    Scalar dot = a.w() * b.w()
               + a.x() * b.x()
               + a.y() * b.y()
               + a.z() * b.z();

    // If dot < 0, use the negated second quaternion to take the short arc
    T81Quaternion b_adj = b;
    if (dot < Scalar(0)) {
        dot   = -dot;
        b_adj = -b;
    }

    // If very close, fall back to normalized lerp
    if (dot > Scalar(0.9999)) {
        const T81Quaternion lerp =
            a + (b_adj - a) * t;
        return lerp.normalized();
    }

    const Scalar theta     = dot.acos();
    const Scalar sin_theta = theta.sin();
    const Scalar one       = Scalar(1);
    const Scalar a_factor  = (theta * (one - t)).sin() / sin_theta;
    const Scalar b_factor  = (theta * t).sin() / sin_theta;

    return (a * a_factor) + (b_adj * b_factor);
}

// ======================================================================
// Example usage
// ======================================================================
/*
inline constexpr auto q = T81Quaternion::from_axis_angle(
    T81Quaternion::Scalar(0),
    T81Quaternion::Scalar(1),
    T81Quaternion::Scalar(0),
    T81Quaternion::Scalar(3.14159) // ~180° around Y
);
inline const auto rotated = q.rotate_vector(
    T81Quaternion::Scalar(1),
    T81Quaternion::Scalar(0),
    T81Quaternion::Scalar(0)
);
// rotated.x() ≈ -1, rotated.y() ≈ 0, rotated.z() ≈ 0
*/

} // namespace t81
