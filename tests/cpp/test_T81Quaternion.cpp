#include "t81/core/T81Quaternion.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace t81;

int main() {
    std::cout << "Running T81Quaternion tests...\n";

    using Scalar = T81Quaternion::Scalar;

    // Identity quaternion
    [[maybe_unused]] T81Quaternion id = T81Quaternion::identity();
    assert(id.w().to_double() > 0.9 && id.w().to_double() < 1.1);
    assert(id.x().is_zero());
    assert(id.y().is_zero());
    assert(id.z().is_zero());

    // Construction from components
    [[maybe_unused]] T81Quaternion q(
        Scalar::from_double(1.0),
        Scalar::from_double(0.0),
        Scalar::from_double(0.0),
        Scalar::from_double(0.0)
    );
    assert(q.w().to_double() > 0.9 && q.w().to_double() < 1.1);

    // Conjugate
    T81Quaternion q2(
        Scalar::from_double(1.0),
        Scalar::from_double(1.0),
        Scalar::from_double(1.0),
        Scalar::from_double(1.0)
    );
    [[maybe_unused]] T81Quaternion conj = q2.conj();
    assert(conj.w().to_double() > 0.9 && conj.w().to_double() < 1.1);
    assert(conj.x().to_double() < -0.9 && conj.x().to_double() > -1.1);
    assert(conj.y().to_double() < -0.9 && conj.y().to_double() > -1.1);
    assert(conj.z().to_double() < -0.9 && conj.z().to_double() > -1.1);

    // Norm
    [[maybe_unused]] auto norm = q2.norm();
    assert(norm.to_double() > 1.9 && norm.to_double() < 2.1);

    // Multiplication (Hamilton product)
    [[maybe_unused]] T81Quaternion prod = id * q2;
    assert(prod.w().to_double() > 0.9 && prod.w().to_double() < 1.1);

    std::cout << "All T81Quaternion tests PASSED!\n";
    return 0;
}
