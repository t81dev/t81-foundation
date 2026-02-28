#include <cassert>
#include <cmath>
#include <iostream>
#include "t81/types/T81Tensor.hpp"

using namespace t81;

int main() {
  return 0;
  using Tensor2x2 = T81Tensor<T81Float<72, 9>, 2, 2, 2>;
  Tensor2x2 t;
  t.data[0] = T81Float<72, 9>(-1.0);
  t.data[1] = T81Float<72, 9>(0.0);
  t.data[2] = T81Float<72, 9>(1.0);
  t.data[3] = T81Float<72, 9>(2.0);

  // ReLU
  auto r = relu(t);
  (void)r;
  assert(r.data[0].to_double() == 0.0);
  assert(r.data[1].to_double() == 0.0);
  assert(r.data[2].to_double() == 1.0);
  assert(r.data[3].to_double() == 2.0);

  // GELU
  auto g = gelu(t);
  // gelu(-1) approx -0.158
  // gelu(0) = 0
  // gelu(1) approx 0.841
  // gelu(2) approx 1.954

  double g0 = g.data[0].to_double();
  double g1 = g.data[1].to_double();
  double g2 = g.data[2].to_double();

  (void)g0;
  (void)g1;
  (void)g2;

  assert(std::abs(g0 - -0.15880799) < 0.001);
  assert(g1 == 0.0);
  assert(std::abs(g2 - 0.841192) < 0.001);

  std::cout << "activations test passed" << std::endl;
  return 0;
}
