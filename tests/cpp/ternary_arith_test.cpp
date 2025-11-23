#include <cassert>
#include <iostream>
#include <vector>
#include "t81/ternary.hpp"
#include "t81/ternary/arith.hpp"

using t81::Trit;

static int64_t v(const std::vector<Trit>& ds) {
  // helper: interpret balanced-ternary (LSB-first) into int64
  return t81::ternary::decode_i64(ds);
}

static std::vector<Trit> enc(int64_t x) {
  return t81::ternary::encode_i64(x);
}

int main() {
  using t81::ternary::add;

  // encode/decode roundtrips
  for (int64_t x : {int64_t(-27), -9, -1, 0, 1, 2, 3, 9, 27, 12345}) {
    auto d = enc(x);
    auto y = v(d);
    assert(x == y);
  }

  // simple additions: (-5) + 7 = 2,  9 + 9 = 18
  {
    auto a = enc(-5);
    auto b = enc(7);
    auto s = add(a, b);
    assert(v(s) == 2);
  }
  {
    auto a = enc(9);
    auto b = enc(9);
    auto s = add(a, b);
    assert(v(s) == 18);
  }

  // normalization keeps at least one digit
  {
    auto z = enc(0);
    assert(z.size() == 1 && z[0] == Trit::Zero);
  }

  std::cout << "ternary_arith ok\n";
  return 0;
}
