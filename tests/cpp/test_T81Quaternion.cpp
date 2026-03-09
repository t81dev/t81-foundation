#undef NDEBUG
#include <cassert>
#include <cmath>
#include <iostream>
#include "t81/types/T81Quaternion.hpp"

using namespace t81;

void test_basic() {
  T81Quaternion id = T81Quaternion::identity();
  assert(id.w().to_double() > 0.9 && id.w().to_double() < 1.1);
  assert(id.x().is_zero());

  // Check equality
  T81Quaternion id2 = T81Quaternion::identity();
  assert(id == id2);

  std::cout << "test_basic PASSED\n";
}

void test_rotate_vector() {
  using Scalar = T81Quaternion::Scalar;
  // Rotate vector (1, 0, 0) by 90 degrees around Z axis -> (0, 1, 0)

  auto axis_z = Scalar::from_double(1.0);
  auto zero = Scalar::from_double(0.0);
  // 90 deg = PI/2
  double pi_half = 3.14159265359 / 2.0;
  auto angle = Scalar::from_double(pi_half);

  T81Quaternion q = T81Quaternion::from_axis_angle(zero, zero, axis_z, angle);

  std::cout << "q: " << q.w().to_double() << ", " << q.x().to_double() << ", " << q.y().to_double()
            << ", " << q.z().to_double() << std::endl;

  // Vector (1, 0, 0)
  auto vx = Scalar::from_double(1.0);
  auto vy = Scalar::from_double(0.0);
  auto vz = Scalar::from_double(0.0);

  auto rotated = q.rotate_vector(vx, vy, vz);

  // Should be approx (0, 1, 0)
  // Rotated vector is in x, y, z components of the result quaternion
  double rx = rotated.x().to_double();
  double ry = rotated.y().to_double();
  double rz = rotated.z().to_double();

  std::cout << "Rotated: " << rx << ", " << ry << ", " << rz << std::endl;

  assert(std::abs(rx) < 0.01);
  assert(std::abs(ry - 1.0) < 0.01);
  assert(std::abs(rz) < 0.01);

  std::cout << "test_rotate_vector PASSED\n";
}

void test_slerp() {
  using Scalar = T81Quaternion::Scalar;
  T81Quaternion q1 = T81Quaternion::identity();

  // q2 is 90 deg rotation around X
  double pi_half = 3.14159265359 / 2.0;
  T81Quaternion q2 =
      T81Quaternion::from_axis_angle(Scalar::from_double(1.0), Scalar::from_double(0.0),
                                     Scalar::from_double(0.0), Scalar::from_double(pi_half));

  // Slerp 0.5 -> 45 deg rotation
  auto half = Scalar::from_double(0.5);
  T81Quaternion mid = slerp(q1, q2, half);

  // Check angle of mid
  // w = cos(theta/2)
  // theta should be 45 deg = PI/4
  // theta/2 = PI/8
  // cos(PI/8) = cos(22.5 deg) = 0.92388

  double w = mid.w().to_double();
  // std::cout << "Slerp mid w: " << w << std::endl;
  assert(std::abs(w - 0.92388) < 0.01);

  std::cout << "test_slerp PASSED\n";
}

int main() {
  test_basic();
  test_rotate_vector();
  test_slerp();
  std::cout << "All T81Quaternion tests PASSED!\n";
  return 0;
}
