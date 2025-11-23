#include <cassert>
#include <sstream>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/io/tensor_loader.hpp"

int main() {
  using namespace t81;

  // Build a 2x3 tensor
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // Save to text via stringstream
  std::stringstream ss;
  t81::io::save_tensor_txt(ss, m);

  // Load back
  std::stringstream in(ss.str());
  auto got = t81::io::load_tensor_txt(in);

  // Validate shape and data
  assert(got.rank() == 2);
  assert(got.shape()[0] == 2 && got.shape()[1] == 3);
  const auto& d = got.data();
  assert(d.size() == 6);
  for (size_t i = 0; i < d.size(); ++i) assert(d[i] == m.data()[i]);

  // Also test parsing with comments/whitespace and multiline data
  std::stringstream ss2;
  ss2 << "# tensor header\n"
      << "2 2 2\n"
      << "1 2\n"
      << "3 4\n";
  auto t2 = t81::io::load_tensor_txt(ss2);
  assert(t2.rank() == 2 && t2.shape()[0] == 2 && t2.shape()[1] == 2);
  const auto& d2 = t2.data();
  assert(d2.size() == 4 && d2[0] == 1 && d2[1] == 2 && d2[2] == 3 && d2[3] == 4);

  std::cout << "tensor_loader ok\n";
  return 0;
}
