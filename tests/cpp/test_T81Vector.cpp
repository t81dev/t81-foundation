// tests/cpp/test_T81Vector.cpp
// Fixed and verified to pass with trivially copyable T81Float<18,9>

#include "t81/core/T81Vector.hpp"
#include "t81/core/T81Float.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81;

// Canonical 3D vector over float81
using Vec3   = T81Vector<3>;
using Scalar = T81Float<18,9>;  // Explicit — no typedef surprises

// Helpers: convert between double and Scalar for testing
static Scalar s(double x) {
    return Scalar::from_double(x);
}

static double d(const Scalar& v) {
    return v.to_double();
}

int main() {
    std::cout << "Running T81Vector tests...\n";

    // 1) Default construction initializes to zero
    {
        Vec3 v{};
        for (std::size_t i = 0; i < 3; ++i) {
            double vi = d(v[i]);
            assert(std::isfinite(vi));
            assert(std::fabs(vi) < 1e-10);  // T81Float zeros perfectly
        }
        std::cout << "  [OK] Default construction\n";
    }

    // 2) Fill constructor — this was the one that failed before
    {
        Vec3 v(s(1.5));
        for (std::size_t i = 0; i < 3; ++i) {
            double vi = d(v[i]);
            assert(std::fabs(vi - 1.5) < 1e-8);  // High precision — T81Float is exact here
        }
        std::cout << "  [OK] Fill constructor (1.5, 1.5, 1.5)\n";
    }

    // 3) Component-wise constructor and indexing
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        assert(std::fabs(d(v[0]) - 1.0) < 1e-8);
        assert(std::fabs(d(v[1]) - 2.0) < 1e-8);
        assert(std::fabs(d(v[2]) - 3.0) < 1e-8);
        std::cout << "  [OK] Component-wise construction and indexing\n";
    }

    // 4) Addition and subtraction
    {
        Vec3 v1(s(1.0), s(2.0), s(3.0));
        Vec3 v2(s(4.0), s(5.0), s(6.0));

        Vec3 sum  = v1 + v2;
        Vec3 diff = v2 - v1;

        assert(std::fabs(d(sum[0])  - 5.0) < 1e-8);
        assert(std::fabs(d(sum[1])  - 7.0) < 1e-8);
        assert(std::fabs(d(sum[2])  - 9.0) < 1e-8);

        assert(std::fabs(d(diff[0]) - 3.0) < 1e-8);
        assert(std::fabs(d(diff[1]) - 3.0) < 1e-8);
        assert(std::fabs(d(diff[2]) - 3.0) < 1e-8);

        std::cout << "  [OK] Vector addition and subtraction\n";
    }

    // 5) Scalar multiplication — both orders
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        Scalar two = s(2.0);

        Vec3 scaled1  = v * two;
        Vec3 scaled2  = two * v;  // friend operator

        for (int i = 0; i < 3; ++i) {
            assert(std::fabs(d(scaled1[i]) - 2.0 * (i + 1)) < 1e-8);
            assert(std::fabs(d(scaled2[i]) - d(scaled1[i])) < 1e-12);
        }

        std::cout << "  [OK] Scalar multiplication (v*s and s*v)\n";
    }

    // 6) Copy construction and assignment
    {
        Vec3 v(s(10.0), s(20.0), s(30.0));
        Vec3 v_copy = v;           // copy constructor
        Vec3 v_assigned;
        v_assigned = v;            // copy assignment

        for (int i = 0; i < 3; ++i) {
            assert(std::fabs(d(v_copy[i])     - d(v[i])) < 1e-12);
            assert(std::fabs(d(v_assigned[i]) - d(v[i])) < 1e-12);
        }

        std::cout << "  [OK] Copy construction and assignment\n";
    }

    std::cout << "All T81Vector tests PASSED!\n";
    return 0;
}
