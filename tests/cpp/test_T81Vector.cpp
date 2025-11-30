// tests/cpp/test_T81Vector.cpp
// CORRECTED — Vec3 uses Scalar = T81Float<18,9> without ambiguity

#include "t81/core/T81Vector.hpp"
#include "t81/core/T81Float.hpp"

#include <cassert>
#include <iostream>
#include <cmath>

// Do NOT use 'using namespace t81;' to avoid Vec3 ambiguity.
using Scalar = t81::T81Float<18, 9>;
using Vec3   = t81::T81Vector<3, Scalar>;

static Scalar s(double x) { return Scalar::from_double(x); }
static double d(const Scalar& v) { return v.to_double(); }

int main() {
    std::cout << "Running T81Vector tests...\n";

    // 1) Default construction → zero
    {
        Vec3 v{};
        for (int i = 0; i < 3; ++i)
            assert(std::fabs(d(v[i])) < 1e-10);
        std::cout << "  [OK] Default construction\n";
    }

    // 2) Fill constructor — was broken before trivial copyability
    {
        Vec3 v(s(1.5));
        for (int i = 0; i < 3; ++i)
            assert(std::fabs(d(v[i]) - 1.5) < 1e-8);
        std::cout << "  [OK] Fill constructor\n";
    }

    // 3) Component-wise construction
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        assert(std::fabs(d(v[0]) - 1.0) < 1e-8);
        assert(std::fabs(d(v[1]) - 2.0) < 1e-8);
        assert(std::fabs(d(v[2]) - 3.0) < 1e-8);
        std::cout << "  [OK] Component-wise construction\n";
    }

    // 4) Arithmetic
    {
        Vec3 a(s(1.0), s(2.0), s(3.0));
        Vec3 b(s(4.0), s(5.0), s(6.0));
        Vec3 sum  = a + b;
        Vec3 diff = b - a;

        assert(std::fabs(d(sum[0])  - 5.0) < 1e-8);
        assert(std::fabs(d(diff[2]) - 3.0) < 1e-8);
        std::cout << "  [OK] Addition & subtraction\n";
    }

    // 5) Scalar multiplication
    {
        Vec3 v(s(1.0), s(2.0), s(3.0));
        Scalar two = s(2.0);

        Vec3 v1 = v * two;
        Vec3 v2 = two * v;

        assert(std::fabs(d(v1[1]) - 4.0) < 1e-8);
        assert(std::fabs(d(v2[0]) - 2.0) < 1e-8);
        std::cout << "  [OK] Scalar multiplication\n";
    }

    // 6) Copy
    {
        Vec3 v(s(10.0), s(20.0), s(30.0));
        Vec3 c = v;
        assert(std::fabs(d(c[2]) - 30.0) < 1e-8);
        std::cout << "  [OK] Copy construction\n";
    }

    std::cout << "All T81Vector tests PASSED!\n";
    return 0;
}
