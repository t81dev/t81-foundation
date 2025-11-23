#include <cassert>
#include <iostream>
#include <stdexcept>
#include "t81/tensor.hpp"
#include "t81/tensor/reshape.hpp"

int main() {
  using namespace t81;

  // Build a simple 2x3 tensor with values 1..6
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // Reshape to 3x2 (no inference)
  auto r = t81::ops::reshape(m, {3,2});
  assert(r.rank() == 2);
  assert(r.shape()[0] == 3);
  assert(r.shape()[1] == 2);
  // Data pointer equality not guaranteed; values must match in row-major order
  const auto& rd = r.data();
  const auto& md = m.data();
  assert(rd.size() == md.size());
  for (size_t i = 0; i < rd.size(); ++i) assert(rd[i] == md[i]);

  // Reshape with -1 inference: {-1, 3} should infer 2 -> {2,3}
  auto r2 = t81::ops::reshape(m, {-1, 3});
  assert(r2.rank() == 2);
  assert(r2.shape()[0] == 2);
  assert(r2.shape()[1] == 3);
  const auto& r2d = r2.data();
  assert(r2d.size() == md.size());
  for (size_t i = 0; i < r2d.size(); ++i) assert(r2d[i] == md[i]);

  // Error cases
  bool threw = false;
  try { (void)t81::ops::reshape(m, {-1, -1}); }  // multiple -1s
  catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  threw = false;
  try { (void)t81::ops::reshape(m, {4, 4}); }    // size mismatch (needs 16)
  catch (const std::invalid_argument&) { threw = true; }
  assert(threw);

  std::cout << "tensor_reshape ok\n";
  return 0;
}
