#include <cassert>
#include <sstream>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/io/tensor_loader.hpp"

int main() {
  using namespace t81;

  // Build a simple 2x3 tensor with values 1..6
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  // Save to text (memory stream)
  std::stringstream ss_out;
  t81::io::save_tensor_txt(ss_out, m);

  // Load back from text
  std::stringstream ss_in(ss_out.str());
  T729Tensor r = t81::io::load_tensor_txt(ss_in);

  // Validate shape & contents
  assert(r.rank() == 2);
  assert(r.shape()[0] == 2);
  assert(r.shape()[1] == 3);
  const auto& rd = r.data();
  const auto& md = m.data();
  assert(rd.size() == md.size());
  for (size_t i = 0; i < rd.size(); ++i) assert(rd[i] == md[i]);

  std::cout << "tensor_loader ok\n";
  return 0;
}
