#include <cassert>
#include <iostream>
#include "t81/tensor.hpp"
#include "t81/tensor/elementwise.hpp"
#include "t81/tensor/broadcast.hpp"
#include "t81/tensor/shape.hpp"

int main() {
  using namespace t81;

  // Same-shape elementwise ops
  {
    T729Tensor A({2,3}); A.data() = {1,2,3, 4,5,6};
    T729Tensor B({2,3}); B.data() = {6,5,4, 3,2,1};

    [[maybe_unused]] auto S = t81::ops::add(A, B);
    [[maybe_unused]] auto D = t81::ops::sub(A, B);
    [[maybe_unused]] auto M = t81::ops::mul(A, B);

    assert((S.data() == std::vector<float>{7,7,7, 7,7,7}));
    assert((D.data() == std::vector<float>{-5,-3,-1, 1,3,5}));
    assert((M.data() == std::vector<float>{6,10,12, 12,10,6}));
  }

  // Broadcasting: vector {3} + matrix {2,3} → {2,3}
  {
    T729Tensor row({3});   row.data() = {10,20,30};
    T729Tensor A({2,3});   A.data()   = {1,2,3, 4,5,6};

    auto S = t81::ops::add(A, row);
    assert(S.rank() == 2 && S.shape()[0] == 2 && S.shape()[1] == 3);
    const auto& sd = S.data();
    assert((sd == std::vector<float>{11,22,33, 14,25,36}));
  }

  // Broadcasting: row {1,3} * matrix {2,3} → {2,3}
  {
    T729Tensor row({1,3}); row.data() = {2,3,4};
    T729Tensor A({2,3});   A.data()   = {1,2,3, 4,5,6};

    auto P = t81::ops::mul(A, row);
    const auto& pd = P.data();
    assert((pd == std::vector<float>{2,6,12, 8,15,24}));
  }

  // Division by zero should throw
  {
    T729Tensor A({3}); A.data() = {1,2,3};
    T729Tensor Z({3}); Z.data() = {1,0,1};
    bool threw = false;
    try { (void)t81::ops::div(A, Z); } catch (const std::domain_error&) { threw = true; }
    assert(threw);
  }

  std::cout << "tensor_elementwise ok\n";
  return 0;
}
