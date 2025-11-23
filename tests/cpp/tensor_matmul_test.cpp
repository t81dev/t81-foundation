#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/matmul.hpp"

int main() {
  using namespace t81;

  // A: 2x3
  // [1 2 3
  //  4 5 6]
  T729Tensor A({2,3});
  A.data() = {1,2,3, 4,5,6};

  // B: 3x2
  // [7  8
  //  9 10
  // 11 12]
  T729Tensor B({3,2});
  B.data() = {7,8, 9,10, 11,12};

  // C = A·B : 2x2
  // [ 58,  64
  //  139, 154]
  auto C = t81::ops::matmul(A, B);

  assert(C.rank() == 2);
  assert(C.shape()[0] == 2);
  assert(C.shape()[1] == 2);

  const auto& d = C.data();
  assert(d.size() == 4);
  assert(d[0] == 58);
  assert(d[1] == 64);
  assert(d[2] == 139);
  assert(d[3] == 154);

  std::cout << "tensor_matmul ok\n";
  return 0;
}
