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
#include <format>

namespace t81 {

// ======================================================================
// T81Vector<N, Scalar = T81Float<72,9>> – N-dimensional vector
// ======================================================================
template <std::size_t N, typename Scalar = T81Float<72,9>>
class T81Vector {
    static_assert(N >= 1 && N <= 81, "T81Vector dimension must be 1–81");

    alignas(64) Scalar components_[N];

    // Helper: construct Scalar from double when available, otherwise via cast
    static Scalar scalar_from_double(double v) noexcept {
        if constexpr (requires { Scalar::from_double(v); }) {
            return Scalar::from_double(v);
        } else {
            return static_cast<Scalar>(v);
        }
    }

    // Bridge arbitrary component/scalar types to Scalar
    template <typename C>
    static constexpr Scalar component_to_scalar(const C& c) noexcept {
        using Decayed = std::decay_t<C>;
        if constexpr (std::same_as<Decayed, Scalar>) {
            // Exact same type: copy
            return c;
        } else if constexpr (std::convertible_to<C, Scalar>) {
            // Implicitly convertible (e.g., T81Float<18,9> → T81Float<72,9> via widening ctor)
            return static_cast<Scalar>(c);
        } else if constexpr (requires (const C& x) { { x.to_double() } -> std::convertible_to<double>; }) {
            // Has to_double(): bridge via double → Scalar
            return scalar_from_double(c.to_double());
        } else {
            static_assert(std::same_as<C, void>,
                          "T81Vector component type is not convertible or bridgeable to Scalar");
        }
    }

    template <typename C>
    concept VectorComponent =
        std::same_as<std::decay_t<C>, Scalar> ||
        std::convertible_to<C, Scalar> ||
        requires (const C& x) { { x.to_double() } -> std::convertible_to<double>; };

public:
    using value_type = Scalar;
    static constexpr std::size_t dimension = N;

    //===================================================================
    // Construction
    //===================================================================
    constexpr T81Vector() noexcept : components_{} {}

    explicit constexpr T81Vector(Scalar fill) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            components_[i] = fill;
        }
    }

    // Variadic constructor – accepts N components that can be bridged to Scalar
    template <VectorComponent... Components>
        requires (sizeof...(Components) == N)
    constexpr T81Vector(Components... comps) noexcept
        : components_{ component_to_scalar(comps)... } {}

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
        return T81Vector(scalar_from_double(0.0));
    }

    [[nodiscard]] static constexpr T81Vector one() noexcept {
        return unit_vector<0>();
    }

    template <std::size_t I>
    [[nodiscard]] static constexpr T81Vector unit_vector() noexcept {
        static_assert(I < N, "Unit vector index out of range");
        T81Vector v = zero();
        v[I] = scalar_from_double(1.0);
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

    // Right scalar multiply: v * s (s can be Scalar, T81Float<18,9>, double, etc.)
    template <VectorComponent S>
    [[nodiscard]] constexpr T81Vector operator*(const S& s) const noexcept {
        const Scalar ss = component_to_scalar(s);
        T81Vector r;
        for (std::size_t i = 0; i < N; ++i) r[i] = components_[i] * ss;
        return r;
    }

    // Left scalar multiply: s * v
    template <VectorComponent S>
    friend constexpr T81Vector operator*(const S& s, const T81Vector& v) noexcept {
        const Scalar ss = component_to_scalar(s);
        return v * ss;
    }

    //===================================================================
    // Geometric operations
    //===================================================================
    [[nodiscard]] constexpr Scalar dot(const T81Vector& o) const noexcept {
        Scalar sum = scalar_from_double(0.0);
        for (std::size_t i = 0; i < N; ++i) sum += components_[i] * o[i];
        return sum;
    }

    [[nodiscard]] constexpr Scalar length2() const noexcept { return dot(*this); }

    [[nodiscard]] Scalar length() const noexcept {
        if constexpr (requires (const Scalar& s) { { s.to_double() } -> std::convertible_to<double>; }) {
            const double l2 = length2().to_double();
            if (l2 <= 0.0) {
                return scalar_from_double(0.0);
            }
            return scalar_from_double(std::sqrt(l2));
        } else {
            // Fallback: no bridge available, return squared length
            return length2();
        }
    }

    [[nodiscard]] T81Vector normalized() const noexcept {
        if constexpr (requires (const Scalar& s) { { s.to_double() } -> std::convertible_to<double>; }) {
            const double l2 = length2().to_double();
            if (l2 <= 0.0) {
                return *this;
            }
            const double len    = std::sqrt(l2);
            const double inv_d  = 1.0 / len;
            const Scalar inv_sc = scalar_from_double(inv_d);
            return *this * inv_sc;
        } else {
            return *this;
        }
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
        if constexpr (requires (const Scalar& s) { { s.to_double() } -> std::convertible_to<double>; }) {
            const double l1 = length().to_double();
            const double l2 = o.length().to_double();
            double denom = l1 * l2;
            if (denom <= 0.0) {
                return scalar_from_double(0.0);
            }
            double c = dot(o).to_double() / denom;
            if (c < -1.0) c = -1.0;
            if (c >  1.0) c =  1.0;
            return scalar_from_double(std::acos(c));
        } else {
            return scalar_from_double(0.0);
        }
    }

    //===================================================================
    // Rotation (using T81Quaternion)
    //===================================================================
    template <std::size_t M = N>
    [[nodiscard]] std::enable_if_t<M == 3, T81Vector> rotated(const T81Quaternion& q) const noexcept {
        using QScalar = T81Quaternion::Scalar;

        // Bridge vector components into quaternion scalar space
        const QScalar qx = QScalar::from_double(components_[0].to_double());
        const QScalar qy = QScalar::from_double(components_[1].to_double());
        const QScalar qz = QScalar::from_double(components_[2].to_double());

        T81Quaternion vq(
            QScalar::from_double(0.0),
            qx, qy, qz
        );
        T81Quaternion result = q * vq * q.conj();

        // Bridge back into this vector's scalar space
        const Scalar rx = scalar_from_double(result.x().to_double());
        const Scalar ry = scalar_from_double(result.y().to_double());
        const Scalar rz = scalar_from_double(result.z().to_double());

        return T81Vector(rx, ry, rz);
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
// using Vec81 = T81Vector<81>;

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
    T81Quaternion::from_axis_angle(
        T81Quaternion::Scalar::from_double(0.0),
        T81Quaternion::Scalar::from_double(1.0),
        T81Quaternion::Scalar::from_double(0.0),
        T81Quaternion::Scalar::from_double(3.14159 / 2)
    );
constexpr auto left = right.rotated(rotation);  // (0,0,-1) approximately
*/

} // namespace t81
