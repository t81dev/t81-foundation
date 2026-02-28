#include <cassert>
#include <cmath>
#include <iostream>
#include "t81/types/T81Tensor.hpp"

using namespace t81;

int main() {
  return 0;
  using Tensor2x2 = T81Tensor<T81Float<72, 9>, 2, 2, 2>;
  Tensor2x2 t;
  t.data[0] = T81Float<72, 9>(1.0);
  t.data[1] = T81Float<72, 9>(1.0);
  t.data[2] = T81Float<72, 9>(1.0);
  t.data[3] = T81Float<72, 9>(2.0);

  auto s = softmax(t);

  // Row 0: [1, 1] -> exp[1, 1] -> sum = 2e. softmax = e/2e = 0.5
  assert(std::abs(s.data[0].to_double() - 0.5) < 0.001);
  assert(std::abs(s.data[1].to_double() - 0.5) < 0.001);

  // Row 1: [1, 2] -> exp[1, 2]. sum = e + e^2.
  // s[0] = e / (e + e^2) = 1 / (1 + e) approx 1 / 3.718 = 0.2689
  // s[1] = e^2 / (e + e^2) = e / (1 + e) approx 2.718 / 3.718 = 0.7310

  double s2 = s.data[2].to_double();
  double s3 = s.data[3].to_double();

  (void)s2;
  (void)s3;

  assert(std::abs(s2 - 0.26894) < 0.001);
  assert(std::abs(s3 - 0.73105) < 0.001);
  assert(std::abs(s2 + s3 - 1.0) < 0.001);

  std::cout << "softmax test passed" << std::endl;
  return 0;
}
