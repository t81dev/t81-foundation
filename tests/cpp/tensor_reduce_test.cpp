#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/reduce.hpp"

int main() {
  using namespace t81;

  // 2x3:
  // [1 2 3
  //  4 5 6]
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // axis=0 → per-column sums => {2,3} -> {1+4, 2+5, 3+6} = {5,7,9}
  auto s0 = t81::ops::reduce_sum_2d(m, 0);
  assert(s0.rank() == 1 && s0.shape()[0] == 3);
  const auto& d0 = s0.data();
  assert(d0.size() == 3);
  assert(d0[0] == 5);
  assert(d0[1] == 7);
  assert(d0[2] == 9);

  // axis=1 → per-row sums => {2} -> {1+2+3, 4+5+6} = {6,15}
  auto s1 = t81::ops::reduce_sum_2d(m, 1);
  assert(s1.rank() == 1 && s1.shape()[0] == 2);
  const auto& d1 = s1.data();
  assert(d1.size() == 2);
  assert(d1[0] == 6);
  assert(d1[1] == 15);

  // bad axis
  bool threw = false;
  try { (void)t81::ops::reduce_sum_2d(m, 2); } catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  std::cout << "tensor_reduce ok\n";
  return 0;
}
