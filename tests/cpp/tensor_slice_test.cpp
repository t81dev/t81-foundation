#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/slice.hpp"

int main() {
  using namespace t81;

  // 4x5 matrix with values 1..20 laid out row-major
  T729Tensor m({4,5});
  m.data() = {
     1,  2,  3,  4,  5,
     6,  7,  8,  9, 10,
    11, 12, 13, 14, 15,
    16, 17, 18, 19, 20
  };

  // Slice rows [1,3) and cols [2,5) -> 2x3:
  // Source rows 1..2, cols 2..4:
  // [ 8,  9, 10
  //  13, 14, 15 ]
  auto s = t81::ops::slice2d(m, /*r0=*/1, /*r1=*/3, /*c0=*/2, /*c1=*/5);

  assert(s.rank() == 2);
  assert(s.shape()[0] == 2);
  assert(s.shape()[1] == 3);

  const auto& d = s.data();
  assert(d.size() == 6);
  assert(d[0] ==  8);
  assert(d[1] ==  9);
  assert(d[2] == 10);
  assert(d[3] == 13);
  assert(d[4] == 14);
  assert(d[5] == 15);

  // Edge case: full-row slice [0,4) x [0,5) should equal original
  auto s_full = t81::ops::slice2d(m, 0, 4, 0, 5);
  assert(s_full.rank() == 2);
  assert(s_full.shape()[0] == 4);
  assert(s_full.shape()[1] == 5);
  const auto& df = s_full.data();
  assert(df.size() == m.data().size());
  for (size_t i = 0; i < df.size(); ++i) assert(df[i] == m.data()[i]);

  std::cout << "tensor_slice ok\n";
  return 0;
}
