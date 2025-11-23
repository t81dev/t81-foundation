#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/tensor/transpose.hpp"

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

  // C = A·B -> 2x2
  auto C = t81::ops::matmul(A, B);
  assert(C.rank() == 2 && C.shape()[0] == 2 && C.shape()[1] == 2);
  const auto& cd = C.data();
  // Expected:
  // [ 58  64
  //  139 154]
  assert(cd.size() == 4);
  assert(cd[0] == 58 && cd[1] == 64 && cd[2] == 139 && cd[3] == 154);

  // Sanity: A·A^T -> 2x2
  auto AT = t81::ops::transpose(A);
  auto G = t81::ops::matmul(A, AT);
  const auto& gd = G.data();
  // [1 2 3]·[1 2 3] = 14 ; [4 5 6]·[4 5 6] = 77 ; off-diag = 32
  assert((gd == std::vector<float>{14, 32, 32, 77}));

  // Mismatch should throw
  bool threw = false;
  try {
    (void)t81::ops::matmul(A, A); // 2x3 · 2x3 invalid
  } catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  std::cout << "tensor_matmul ok\n";
  return 0;
}
