// tests/cpp/test_T81Vector.cpp

#include "t81/core/T81Vector.hpp"
#include "t81/core/T81Float.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81;

// Canonical 3D vector over float81
using Vec3   = T81Vector<3>;
using Scalar = float81;

// Helpers: convert between double and Scalar for testing
static Scalar s(double x) {
    return Scalar::from_double(x);
}

static double d(const Scalar& v) {
    return v.to_double();
}

int main() {
    std::cout << "Running T81Vector tests...\n";

    // 1) Default construction initializes to "zero-ish" values
    {
        Vec3 v{};
        for (std::size_t i = 0; i < 3; ++i) {
            double vi = d(v[i]);
            assert(std::isfinite(vi));
            assert(std::fabs(vi) < 1e-6);
        }
    }

    // 2) Fill constructor
    {
        Vec3 v(s(1.5));
        for (std::size_t i = 0; i < 3; ++i) {
            double vi = d(v[i]);
            assert(std::fabs(vi - 1.5) < 1e-3);
        }
    }

    // 3) Component-wise constructor and indexing
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        assert(std::fabs(d(v[0]) - 1.0) < 1e-3);
        assert(std::fabs(d(v[1]) - 2.0) < 1e-3);
        assert(std::fabs(d(v[2]) - 3.0) < 1e-3);
    }

    // 4) Addition and subtraction
    {
        Vec3 v1(s(1.0), s(2.0), s(3.0));
        Vec3 v2(s(4.0), s(5.0), s(6.0));

        Vec3 sum  = v1 + v2;
        Vec3 diff = v2 - v1;

        assert(std::fabs(d(sum[0])  - 5.0) < 1e-3);
        assert(std::fabs(d(sum[1])  - 7.0) < 1e-3);
        assert(std::fabs(d(sum[2])  - 9.0) < 1e-3);

        assert(std::fabs(d(diff[0]) - 3.0) < 1e-3);
        assert(std::fabs(d(diff[1]) - 3.0) < 1e-3);
        assert(std::fabs(d(diff[2]) - 3.0) < 1e-3);
    }

    // 5) Scalar multiplication by Scalar (float81), both v * s and s * v
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        Scalar two = s(2.0);

        Vec3 scaled  = v * two;   // member operator*(Scalar)
        Vec3 scaled2 = two * v;   // friend Scalar * Vec3

        assert(std::fabs(d(scaled[0]) - 2.0) < 1e-3);
        assert(std::fabs(d(scaled[1]) - 4.0) < 1e-3);
        assert(std::fabs(d(scaled[2]) - 6.0) < 1e-3);

        // scalar multiplication should be commutative here
        for (std::size_t i = 0; i < 3; ++i) {
            assert(std::fabs(d(scaled[i]) - d(scaled2[i])) < 1e-6);
        }
    }

    std::cout << "All T81Vector tests PASSED!\n";
    return 0;
}
