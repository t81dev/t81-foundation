#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/reshape.hpp"

int main() {
  using namespace t81;

  // 2x3 tensor: [1 2 3; 4 5 6]
  T729DynamicTensor m({2, 3});
  m.data() = {1, 2, 3, 4, 5, 6};

  // Reshape 2x3 -> 3x2 (same data, row-major)
  [[maybe_unused]] auto r = t81::ops::reshape(m, {3, 2});
  assert(r.rank() == 2);
  assert(r.shape()[0] == 3 && r.shape()[1] == 2);
  // Expect: [1 2; 3 4; 5 6] in row-major
  const auto& d = r.data();
  (void)d;
  assert((d == std::vector<float>{1, 2, 3, 4, 5, 6}));
  assert(r.canonical_fixed_authoritative());

  // Reshape using -1 inference: 2x3 -> {-1} => {6}
  [[maybe_unused]] auto v = t81::ops::reshape(m, {-1});
  assert(v.rank() == 1 && v.shape()[0] == 6);
  assert((v.data() == std::vector<float>{1, 2, 3, 4, 5, 6}));
  assert(v.canonical_fixed_authoritative());

  // Infer middle dim: {2,3} -> {2,-1,1} -> {2,3,1}
  [[maybe_unused]] auto r3 = t81::ops::reshape(m, {2, -1, 1});
  assert(r3.rank() == 3);
  assert(r3.shape()[0] == 2 && r3.shape()[1] == 3 && r3.shape()[2] == 1);
  assert((r3.data() == std::vector<float>{1, 2, 3, 4, 5, 6}));

  // Multiple -1 should throw
  [[maybe_unused]] bool threw = false;
  try {
    (void)t81::ops::reshape(m, {-1, -1});
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);  // Mismatched element count should throw
  threw = false;
  try {
    (void)t81::ops::reshape(m, {4, 2});
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  std::cout << "tensor_reshape ok\n";
  return 0;
}
