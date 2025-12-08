#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/ternary.hpp"
#include "t81/ternary/arith.hpp"

using namespace t81;

int main() {
  using t81::ternary::encode_i64;
  using t81::ternary::decode_i64;
  using t81::ternary::add;
  using t81::Trit;

  // --- encode/decode roundtrips ---
  {
    std::vector<int64_t> vals = {0, 1, -1, 2, -2, 3, -3, 7, -7, 42, -42,
                                 123456789, -123456789, INT64_C(1)<<40, -(INT64_C(1)<<40)};
    for (auto v : vals) {
      auto ds = encode_i64(v);
      [[maybe_unused]] auto r  = decode_i64(ds);
      assert(r == v);
    }
  }

  // --- addition correctness on small cases ---
  {
    // 5 + 7 = 12
    auto a = encode_i64(5);
    auto b = encode_i64(7);
    auto c = add(a, b);
    assert(decode_i64(c) == 12);

    // (-9) + 4 = -5
    auto m9 = encode_i64(-9);
    auto p4 = encode_i64(4);
    auto sum = add(m9, p4);
    assert(decode_i64(sum) == -5);

    // Biggish numbers
    auto x = encode_i64(1234567);
    auto y = encode_i64(-987654);
    auto z = add(x, y);
    assert(decode_i64(z) == (1234567 - 987654));
  }

  // --- normalization: no redundant leading zeros (MS trits) ---
  {
    auto d0 = encode_i64(0);
    assert(d0.size() == 1 && d0[0] == Trit::Zero);

    auto d1 = encode_i64(1);
    // Create c = d1 + encode_i64(-1) -> 0, should normalize to single zero
    auto c = add(d1, encode_i64(-1));
    assert(c.size() == 1 && c[0] == Trit::Zero);
  }

  // --- randomized-ish sweep (deterministic) ---
  {
    int64_t acc = 0;
    for (int i = -500; i <= 500; ++i) {
      auto ds = encode_i64(i);
      acc += decode_i64(ds);
    }
    // sum_{i=-500}^{500} i = 0
    assert(acc == 0);
  }

  std::cout << "ternary_arith ok\n";
  return 0;
}
