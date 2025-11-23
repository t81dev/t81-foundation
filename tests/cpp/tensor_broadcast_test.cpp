#include <cassert>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/broadcast.hpp"

int main() {
  using namespace t81;

  // Broadcast a vector {3} -> {2,3}
  T729Tensor v({3});
  v.data() = {1, 2, 3};
  auto M = t81::ops::broadcast_to(v, {2,3});
  assert(M.rank() == 2);
  assert(M.shape()[0] == 2 && M.shape()[1] == 3);
  const auto& md = M.data();
  assert(md.size() == 6);
  // Two identical rows
  assert(md[0] == 1 && md[1] == 2 && md[2] == 3);
  assert(md[3] == 1 && md[4] == 2 && md[5] == 3);

  // Broadcast a row {1,3} -> {4,3}
  T729Tensor row({1,3});
  row.data() = {10, 20, 30};
  auto R = t81::ops::broadcast_to(row, {4,3});
  assert(R.rank() == 2);
  assert(R.shape()[0] == 4 && R.shape()[1] == 3);
  const auto& rd = R.data();
  for (int i = 0; i < 4; ++i) {
    size_t base = static_cast<size_t>(i) * 3;
    assert(rd[base+0] == 10);
    assert(rd[base+1] == 20);
    assert(rd[base+2] == 30);
  }

  // Incompatible shapes should throw
  bool threw = false;
  try {
    (void)t81::ops::broadcast_to(v, {4,4}); // {3} -> {4,4} incompatible
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);

  std::cout << "tensor_broadcast ok\n";
  return 0;
}
