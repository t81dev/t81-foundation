#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>
#include "t81/types/T81Quaternion.hpp"
#include "t81/types/T81Vector.hpp"

// Minimal test runner macro
#define TEST_CHECK(cond)                                                                      \
  do {                                                                                        \
    if (!(cond)) {                                                                            \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1);                                                                           \
    }                                                                                         \
  } while (0)

using namespace t81::v1;
using namespace t81;

void test_vector_determinism() {
  std::cout << "Testing T81Vector determinism..." << std::endl;

  using Vec3 = T81Vector<3, T81Float<72, 9>>;
  using Float = T81Float<72, 9>;

  // 1. Construction & Access
  Vec3 v1(1.0, 2.0, 3.0);
  TEST_CHECK(v1[0].to_double() == 1.0);
  TEST_CHECK(v1[1].to_double() == 2.0);
  TEST_CHECK(v1[2].to_double() == 3.0);

  // 2. Arithmetic
  Vec3 v2(4.0, 5.0, 6.0);
  Vec3 sum = v1 + v2;
  TEST_CHECK(sum[0].to_double() == 5.0);
  TEST_CHECK(sum[1].to_double() == 7.0);
  TEST_CHECK(sum[2].to_double() == 9.0);

  // 3. Dot Product
  Float dot = v1.dot(v2);
  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
  TEST_CHECK(std::abs(dot.to_double() - 32.0) < 1e-5);

  // 4. Cross Product
  Vec3 up(0.0, 1.0, 0.0);
  Vec3 forward(0.0, 0.0, 1.0);
  Vec3 right = up.cross(forward);  // (1, 0, 0)
  TEST_CHECK(std::abs(right[0].to_double() - 1.0) < 1e-5);
  TEST_CHECK(std::abs(right[1].to_double()) < 1e-5);
  TEST_CHECK(std::abs(right[2].to_double()) < 1e-5);

  // 5. Normalization
  Vec3 v3(3.0, 4.0, 0.0);
  Vec3 norm = v3.normalized();
  TEST_CHECK(std::abs(norm[0].to_double() - 0.6) < 1e-5);
  TEST_CHECK(std::abs(norm[1].to_double() - 0.8) < 1e-5);
}

int main() {
  test_vector_determinism();
  std::cout << "All vector determinism tests passed." << std::endl;
  return 0;
}
