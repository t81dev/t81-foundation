#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/transpose.hpp"

int main() {
  using namespace t81;

  // 2x3:
  // [1 2 3
  //  4 5 6]
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  auto t = t81::ops::transpose(m);

  // Should be 3x2:
  assert(t.rank() == 2);
  assert(t.shape()[0] == 3);
  assert(t.shape()[1] == 2);

  const auto& d = t.data();
  // Transposed:
  // [1 4
  //  2 5
  //  3 6]
  assert(d.size() == 6);
  assert(d[0] == 1);
  assert(d[1] == 4);
  assert(d[2] == 2);
  assert(d[3] == 5);
  assert(d[4] == 3);
  assert(d[5] == 6);

  std::cout << "tensor_transpose ok\n";
  return 0;
}
