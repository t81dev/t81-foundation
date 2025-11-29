#include "t81/core/T81Vector.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81;

int main() {
    std::cout << "Running T81Vector tests...\n";

    using Vec3 = T81Vector<3>;

    // Construction
    Vec3 zero = Vec3::zero();
    assert(zero[0].is_zero());
    assert(zero[1].is_zero());
    assert(zero[2].is_zero());

    Vec3 v1(1.0, 2.0, 3.0);
    assert(v1[0].to_double() > 0.9 && v1[0].to_double() < 1.1);
    assert(v1[1].to_double() > 1.9 && v1[1].to_double() < 2.1);
    assert(v1[2].to_double() > 2.9 && v1[2].to_double() < 3.1);

    // Unit vector
    Vec3 unit = Vec3::unit_vector<0>();
    assert(unit[0].to_double() > 0.9 && unit[0].to_double() < 1.1);
    assert(unit[1].is_zero());
    assert(unit[2].is_zero());

    // Arithmetic
    Vec3 v2(4.0, 5.0, 6.0);
    Vec3 sum = v1 + v2;
    assert(sum[0].to_double() > 4.9 && sum[0].to_double() < 5.1);
    assert(sum[1].to_double() > 6.9 && sum[1].to_double() < 7.1);
    assert(sum[2].to_double() > 8.9 && sum[2].to_double() < 9.1);

    Vec3 diff = v2 - v1;
    assert(diff[0].to_double() > 2.9 && diff[0].to_double() < 3.1);

    // Scalar multiplication
    Vec3 scaled = v1 * 2.0;
    assert(scaled[0].to_double() > 1.9 && scaled[0].to_double() < 2.1);

    // Dot product
    auto dot = v1.dot(v2);
    double expected_dot = 1.0*4.0 + 2.0*5.0 + 3.0*6.0;  // 32
    assert(dot.to_double() > expected_dot - 0.1 && dot.to_double() < expected_dot + 0.1);

    // Magnitude
    auto mag = v1.magnitude();
    double expected_mag = std::sqrt(1.0*1.0 + 2.0*2.0 + 3.0*3.0);
    assert(mag.to_double() > expected_mag - 0.1 && mag.to_double() < expected_mag + 0.1);

    std::cout << "All T81Vector tests PASSED!\n";
    return 0;
}

