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
  T729DynamicTensor m({2, 3});
  m.data() = {1, 2, 3, 4, 5, 6};

  // Sum axis 0 (per-column) => {3}
  [[maybe_unused]] auto s0 = t81::ops::reduce_sum_2d(m, 0);
  assert(s0.rank() == 1 && s0.shape()[0] == 3);
  assert((s0.data() == std::vector<float>{1 + 4, 2 + 5, 3 + 6}));

  // Sum axis 1 (per-row) => {2}
  [[maybe_unused]] auto s1 = t81::ops::reduce_sum_2d(m, 1);
  assert(s1.rank() == 1 && s1.shape()[0] == 2);
  assert((s1.data() == std::vector<float>{1 + 2 + 3, 4 + 5 + 6}));

  // Max axis 0 => {3}
  [[maybe_unused]] auto mx0 = t81::ops::reduce_max_2d(m, 0);
  assert(mx0.rank() == 1 && mx0.shape()[0] == 3);
  assert((mx0.data() == std::vector<float>{4, 5, 6}));

  // Max axis 1 => {2}
  [[maybe_unused]] auto mx1 = t81::ops::reduce_max_2d(m, 1);
  assert(mx1.rank() == 1 && mx1.shape()[0] == 2);
  assert((mx1.data() == std::vector<float>{3, 6}));

  auto dot_exact = t81::ops::contract_dot(T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}),
                                          T729DynamicTensor({3}, {4.0f, 5.0f, 6.0f}));
  assert(dot_exact.data()[0] == 32.0f);
  assert(dot_exact.numeric_class() == TensorNumericClass::ExactInt);

  auto dot_float = t81::ops::contract_dot(T729DynamicTensor({2}, {1.0f, 2.0f}),
                                          T729DynamicTensor({2}, {0.5f, 1.5f}));
  assert(dot_float.numeric_class() == TensorNumericClass::HostFloat);

  // Bad axis should throw
  [[maybe_unused]] bool threw = false;
  try {
    (void)t81::ops::reduce_sum_2d(m, 2);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  threw = false;
  try {
    (void)t81::ops::reduce_max_2d(m, -1);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  std::cout << "tensor_reduce ok\n";
  return 0;
}
