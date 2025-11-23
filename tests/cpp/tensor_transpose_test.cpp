#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/transpose.hpp"

int main() {
  using namespace t81;

  // 2x3 matrix:
  // [ 1 2 3
  //   4 5 6 ]
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  auto mt = t81::ops::transpose(m);

  // Expect 3x2:
  assert(mt.rank() == 2);
  assert(mt.shape()[0] == 3);
  assert(mt.shape()[1] == 2);

  // Transposed:
  // [ 1 4
  //   2 5
  //   3 6 ]
  const auto& d = mt.data();
  assert(d.size() == 6);
  assert(d[0] == 1); // (0,0)
  assert(d[1] == 4); // (0,1)
  assert(d[2] == 2); // (1,0)
  assert(d[3] == 5); // (1,1)
  assert(d[4] == 3); // (2,0)
  assert(d[5] == 6); // (2,1)

  std::cout << "tensor_transpose ok\n";
  return 0;
}
