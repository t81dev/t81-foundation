#include <iostream>
#include "t81/t81.hpp"

int main(){
  using namespace t81;
  auto A = T243BigInt::from_ascii("hello");
  auto B = T243BigInt::from_ascii("world");
  auto S = T243BigInt::add(A,B);
  auto P = T243BigInt::mul(A,B);
  std::cout << S.to_string() << "\n" << P.to_string() << "\n";
  T729Tensor v({3}); v.data() = {1,2,3};
  T729Tensor u({3}); u.data() = {4,5,6};
  auto dot = T729Tensor::contract_dot(v,u);
  std::cout << dot.data()[0] << "\n";
}
