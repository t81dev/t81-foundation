#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/ops.hpp"

int main() {
  using namespace t81;

  // Build a 2x3 tensor with values 1..6
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // Transpose to 3x2
  auto mt = t81::ops::transpose(m);

  // Slice rows [1,2) and cols [0,2) -> 1x2 => {4,5}
  auto s = t81::ops::slice2d(m, 1, 2, 0, 2);

  // Reshape to 3x2 (same data order)
  auto r = t81::ops::reshape(m, {3,2});

  std::cout << "orig  shape: [" << m.shape()[0] << "," << m.shape()[1] << "]\n";
  std::cout << "trans shape: [" << mt.shape()[0] << "," << mt.shape()[1] << "]\n";
  std::cout << "slice shape: [" << s.shape()[0] << "," << s.shape()[1] << "]\n";
  std::cout << "rshp  shape: [" << r.shape()[0] << "," << r.shape()[1] << "]\n";

  std::cout << "transpose data: ";
  for (float v : mt.data()) std::cout << v << " ";
  std::cout << "\nslice data: ";
  for (float v : s.data()) std::cout << v << " ";
  std::cout << "\nreshape data: ";
  for (float v : r.data()) std::cout << v << " ";
  std::cout << "\n";
  return 0;
}
