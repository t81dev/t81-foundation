#include <iostream>
#include "t81/t81.hpp"

int main() {
  using namespace t81;

  // ---- BigInt demo ---------------------------------------------------------
  auto a = T243BigInt::from_i64(12345);
  auto b = T243BigInt::from_i64(6789);
  auto sum = T243BigInt::add(a, b);
  auto prod = T243BigInt::mul(a, b);

  std::cout << "[BigInt]\n";
  std::cout << "a      = " << a.to_string() << "\n";
  std::cout << "b      = " << b.to_string() << "\n";
  std::cout << "a + b  = " << sum.to_string() << "\n";
  std::cout << "a * b  = " << prod.to_string() << "\n\n";

  // ---- Fraction demo -------------------------------------------------------
  auto f1 = T81Fraction::from_int(2);
  auto f2 = T81Fraction::from_int(3);
  auto fsum = T81Fraction::add(f1, f2); // 5/1
  auto fdiv = T81Fraction::div(f1, f2); // 2/3

  std::cout << "[Fraction]\n";
  std::cout << "2/1 + 3/1 = " << fsum.to_string() << "\n";
  std::cout << "2/1 / 3/1 = " << fdiv.to_string() << "\n\n";

  // ---- Tensor demo ---------------------------------------------------------
  // Vectors for dot: [1,2,3] · [4,5,6] = 32
  T729Tensor v1({3}); v1.data() = {1,2,3};
  T729Tensor v2({3}); v2.data() = {4,5,6};
  auto dot = T729Tensor::contract_dot(v1, v2);

  // 2x3 matrix ops
  T729Tensor m({2,3});
  m.data() = {1,2,3, 4,5,6};

  auto mt = t81::ops::transpose(m);
  auto s  = t81::ops::slice2d(m, 1, 2, 0, 2);
  auto r  = t81::ops::reshape(m, {3,2});
  auto c  = t81::ops::matmul(m, t81::ops::transpose(m)); // (2x3)·(3x2) -> (2x2)

  std::cout << "[Tensor]\n";
  std::cout << "dot([1,2,3],[4,5,6]) = " << dot.data()[0] << "\n";

  std::cout << "transpose(2x3) -> " << mt.shape()[0] << "x" << mt.shape()[1] << "\n";
  std::cout << "slice( rows[1:2), cols[0:2) ) -> "
            << s.shape()[0] << "x" << s.shape()[1] << " : {";
  for (size_t i = 0; i < s.data().size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << s.data()[i];
  }
  std::cout << "}\n";

  std::cout << "reshape(2x3 -> 3x2) -> " << r.shape()[0] << "x" << r.shape()[1] << "\n";
  std::cout << "matmul(2x3, 3x2) -> " << c.shape()[0] << "x" << c.shape()[1] << " : {";
  for (size_t i = 0; i < c.data().size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << c.data()[i];
  }
  std::cout << "}\n";

  // ---- CanonFS / Hash stubs -----------------------------------------------
  std::string payload = "hello-t81";
  auto h = t81::hash::make_canonhash81_base81stub(payload);
  t81::CanonRef ref = t81::CanonRef::make(h, CANON_PERM_READ | CANON_PERM_WRITE, 0);

  uint8_t buf[t81::canonfs_io::kWireSize];
  t81::canonfs_io::encode_ref(ref, buf);
  auto ref2 = t81::canonfs_io::decode_ref(buf);

  std::cout << "\n[CanonFS]\n";
  std::cout << "canon hash (stub) = " << ref2.target.to_string() << "\n";

  return 0;
}
