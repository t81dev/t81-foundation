#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/reduce.hpp"

int main() {
  using namespace t81;

  // 2x3 matrix:
  // [1 2 3
  //  4 5 6]
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // Sum axis 0 (per-column) => {3}
  auto s0 = t81::ops::reduce_sum_2d(m, 0);
  assert(s0.rank() == 1 && s0.shape()[0] == 3);
  assert((s0.data() == std::vector<float>{1+4, 2+5, 3+6}));

  // Sum axis 1 (per-row) => {2}
  auto s1 = t81::ops::reduce_sum_2d(m, 1);
  assert(s1.rank() == 1 && s1.shape()[0] == 2);
  assert((s1.data() == std::vector<float>{1+2+3, 4+5+6}));

  // Max axis 0 => {3}
  auto mx0 = t81::ops::reduce_max_2d(m, 0);
  assert(mx0.rank() == 1 && mx0.shape()[0] == 3);
  assert((mx0.data() == std::vector<float>{4,5,6}));

  // Max axis 1 => {2}
  auto mx1 = t81::ops::reduce_max_2d(m, 1);
  assert(mx1.rank() == 1 && mx1.shape()[0] == 2);
  assert((mx1.data() == std::vector<float>{3,6}));

  // Bad axis should throw
  bool threw = false;
  try { (void)t81::ops::reduce_sum_2d(m, 2); } catch (const std::invalid_argument&) { threw = true; }
  assert(threw); threw = false;
  try { (void)t81::ops::reduce_max_2d(m, -1); } catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  std::cout << "tensor_reduce ok\n";
  return 0;
}
