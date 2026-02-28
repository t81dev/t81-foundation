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

  // 1. Construction determinism - same inputs should produce identical outputs
  Vec3 v1_a(1.0, 2.0, 3.0);
  Vec3 v1_b(1.0, 2.0, 3.0);
  TEST_CHECK(v1_a[0] == v1_b[0]);
  TEST_CHECK(v1_a[1] == v1_b[1]);
  TEST_CHECK(v1_a[2] == v1_b[2]);

  // 2. Arithmetic determinism - same operations should produce identical results
  Vec3 v2_a(4.0, 5.0, 6.0);
  Vec3 v2_b(4.0, 5.0, 6.0);

  Vec3 sum_a = v1_a + v2_a;
  Vec3 sum_b = v1_b + v2_b;
  TEST_CHECK(sum_a[0] == sum_b[0]);
  TEST_CHECK(sum_a[1] == sum_b[1]);
  TEST_CHECK(sum_a[2] == sum_b[2]);

  // 3. Dot product determinism
  Float dot_a = v1_a.dot(v2_a);
  Float dot_b = v1_b.dot(v2_b);
  TEST_CHECK(dot_a == dot_b);

  // 4. Cross product determinism
  Vec3 up_a(0.0, 1.0, 0.0);
  Vec3 up_b(0.0, 1.0, 0.0);
  Vec3 forward_a(0.0, 0.0, 1.0);
  Vec3 forward_b(0.0, 0.0, 1.0);

  Vec3 right_a = up_a.cross(forward_a);
  Vec3 right_b = up_b.cross(forward_b);
  TEST_CHECK(right_a[0] == right_b[0]);
  TEST_CHECK(right_a[1] == right_b[1]);
  TEST_CHECK(right_a[2] == right_b[2]);

  // 5. Normalization determinism
  Vec3 v3_a(3.0, 4.0, 0.0);
  Vec3 v3_b(3.0, 4.0, 0.0);
  Vec3 norm_a = v3_a.normalized();
  Vec3 norm_b = v3_b.normalized();
  TEST_CHECK(norm_a[0] == norm_b[0]);
  TEST_CHECK(norm_a[1] == norm_b[1]);
  TEST_CHECK(norm_a[2] == norm_b[2]);
}

int main() {
  test_vector_determinism();
  std::cout << "All vector determinism tests passed." << std::endl;
  return 0;
}
