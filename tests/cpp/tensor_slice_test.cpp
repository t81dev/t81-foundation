#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/slice.hpp"

int main() {
  using namespace t81;

  // 3x4 matrix:
  // [ 1  2  3  4
  //   5  6  7  8
  //   9 10 11 12 ]
  T729Tensor m({3,4});
  m.data() = {
    1, 2, 3, 4,
    5, 6, 7, 8,
    9,10,11,12
  };

  // Slice rows[0:2), cols[1:3) => 2x2:
  auto s1 = t81::ops::slice2d(m, 0, 2, 1, 3);
  assert(s1.rank() == 2 && s1.shape()[0] == 2 && s1.shape()[1] == 2);
  const auto& d1 = s1.data();
  // Expect:
  // [2 3
  //  6 7]
  assert(d1.size() == 4);
  assert(d1[0] == 2 && d1[1] == 3 && d1[2] == 6 && d1[3] == 7);

  // Slice a single row: rows[1:2), cols[0:4) => 1x4
  auto s2 = t81::ops::slice2d(m, 1, 2, 0, 4);
  assert(s2.rank() == 2 && s2.shape()[0] == 1 && s2.shape()[1] == 4);
  const auto& d2 = s2.data();
  assert((d2 == std::vector<float>{5,6,7,8}));

  // Slice a single column: rows[0:3), cols[2:3) => 3x1
  auto s3 = t81::ops::slice2d(m, 0, 3, 2, 3);
  assert(s3.rank() == 2 && s3.shape()[0] == 3 && s3.shape()[1] == 1);
  const auto& d3 = s3.data();
  assert((d3 == std::vector<float>{3,7,11}));

  // Bad ranges should throw
  bool bad = false;
  try { (void)t81::ops::slice2d(m, -1, 2, 0, 1); } catch (const std::invalid_argument&) { bad = true; }
  assert(bad); bad = false;
  try { (void)t81::ops::slice2d(m, 0, 4, 0, 1); } catch (const std::invalid_argument&) { bad = true; }
  assert(bad); bad = false;
  try { (void)t81::ops::slice2d(m, 0, 1, 3, 2); } catch (const std::invalid_argument&) { bad = true; }
  assert(bad);

  std::cout << "tensor_slice ok\n";
  return 0;
}
