#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/unary.hpp"

static bool approx(float a, float b, float eps=1e-6f) {
  return std::fabs(a-b) <= eps * (1.0f + std::fabs(a) + std::fabs(b));
}

int main() {
  using namespace t81;

  // Base tensor: [-1, 0, 1, 2, 4]
  T729Tensor x({5});
  x.data() = {-1.f, 0.f, 1.f, 2.f, 4.f};

  // relu
  {
    auto y = t81::ops::relu(x);
    const auto& d = y.data();
    assert((d == std::vector<float>{0.f, 0.f, 1.f, 2.f, 4.f}));
  }

  // tanh
  {
    auto y = t81::ops::tanh(x);
    const auto& d = y.data();
    assert(approx(d[0], std::tanh(-1.f)));
    assert(approx(d[1], std::tanh(0.f)));
    assert(approx(d[2], std::tanh(1.f)));
    assert(approx(d[3], std::tanh(2.f)));
    assert(approx(d[4], std::tanh(4.f)));
  }

  // exp
  {
    auto y = t81::ops::exp(x);
    const auto& d = y.data();
    assert(approx(d[0], std::exp(-1.f)));
    assert(approx(d[1], std::exp(0.f)));
    assert(approx(d[2], std::exp(1.f)));
    assert(approx(d[3], std::exp(2.f)));
    assert(approx(d[4], std::exp(4.f)));
  }

  // log (only positive entries survive; ensure throw on non-positive)
  {
    T729Tensor p({3});
    p.data() = {0.5f, 1.f, 10.f};
    auto y = t81::ops::log(p);
    const auto& d = y.data();
    assert(approx(d[0], std::log(0.5f)));
    assert(approx(d[1], std::log(1.f)));
    assert(approx(d[2], std::log(10.f)));

    bool threw = false;
    try {
      (void)t81::ops::log(x); // contains -1 and 0 -> should throw
    } catch (const std::domain_error&) { threw = true; }
    assert(threw);
  }

  std::cout << "tensor_unary ok\n";
  return 0;
}
