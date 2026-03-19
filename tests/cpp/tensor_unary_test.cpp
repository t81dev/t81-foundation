#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/unary.hpp"

[[maybe_unused]] static bool approx(float a, float b, float eps = 1e-6f) {
  return std::fabs(a - b) <= eps * (1.0f + std::fabs(a) + std::fabs(b));
}

int main() {
  using namespace t81;

  // Base tensor: [-1, 0, 1, 2, 4]
  T729DynamicTensor x({5});
  x.data() = {-1.f, 0.f, 1.f, 2.f, 4.f};

  // relu
  {
    [[maybe_unused]] auto y = t81::ops::relu(x);
    [[maybe_unused]] const auto& d = y.data();
    assert((d == std::vector<float>{0.f, 0.f, 1.f, 2.f, 4.f}));
  }

  // tanh
  {
    [[maybe_unused]] auto y = t81::ops::tanh(x);
    [[maybe_unused]] const auto& d = y.data();
    assert(approx(d[0], std::tanh(-1.f)));
    assert(approx(d[1], std::tanh(0.f)));
    assert(approx(d[2], std::tanh(1.f)));
    assert(approx(d[3], std::tanh(2.f)));
    assert(approx(d[4], std::tanh(4.f)));
  }

  // exp
  {
    [[maybe_unused]] auto y = t81::ops::exp(x);
    [[maybe_unused]] const auto& d = y.data();
    assert(y.canonical_fixed_authoritative());
    assert(approx(d[0], std::exp(-1.f)));
    assert(approx(d[1], std::exp(0.f)));
    assert(approx(d[2], std::exp(1.f)));
    assert(approx(d[3], std::exp(2.f)));
    assert(approx(d[4], std::exp(4.f)));
  }

  // exp exact-trit fast path
  {
    T729DynamicTensor trits({3}, {-1.f, 0.f, 1.f});
    trits.set_numeric_class(TensorNumericClass::ExactTrit);
    [[maybe_unused]] auto y = t81::ops::exp(trits);
    [[maybe_unused]] const auto& d = y.data();
    assert(y.canonical_fixed_authoritative());
    assert(approx(d[0], std::exp(-1.f)));
    assert(approx(d[1], 1.f));
    assert(approx(d[2], std::exp(1.f)));
  }

  // sqrt
  {
    T729DynamicTensor p({3});
    p.data() = {0.25f, 1.f, 4.f};
    [[maybe_unused]] auto y = t81::ops::sqrt(p);
    [[maybe_unused]] const auto& d = y.data();
    assert(y.canonical_fixed_authoritative());
    assert(approx(d[0], 0.5f));
    assert(approx(d[1], 1.f));
    assert(approx(d[2], 2.f));

    [[maybe_unused]] bool threw = false;
    try {
      (void)t81::ops::sqrt(x);  // contains -1 -> should throw
    } catch (const std::domain_error&) {
      threw = true;
    }
    assert(threw);
  }

  // log (only positive entries survive; ensure throw on non-positive)
  {
    T729DynamicTensor p({3});
    p.data() = {0.5f, 1.f, 10.f};
    [[maybe_unused]] auto y = t81::ops::log(p);
    [[maybe_unused]] const auto& d = y.data();
    assert(y.canonical_fixed_authoritative());
    assert(y.numeric_class() == TensorNumericClass::ExactInt);
    assert(y.strict_core_eligible());
    assert(approx(d[0], std::log(0.5f)));
    assert(approx(d[1], std::log(1.f)));
    assert(approx(d[2], std::log(10.f)));

    [[maybe_unused]] bool threw = false;
    try {
      (void)t81::ops::log(x);  // contains -1 and 0 -> should throw
    } catch (const std::domain_error&) {
      threw = true;
    }
    assert(threw);
  }

  // Finite canonical-authoritative inputs should stay authoritative through generic unary ops.
  {
    const T729DynamicTensor finite({3}, {0.5f, -1.25f, 2.0f});
    assert(finite.canonical_fixed_authoritative());
    [[maybe_unused]] auto y = t81::ops::tanh(finite);
    assert(y.canonical_fixed_authoritative());
  }

  std::cout << "tensor_unary ok\n";
  return 0;
}
