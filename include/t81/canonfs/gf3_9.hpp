#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace t81::canonfs {

/**
 * @class GF3_9
 * @brief Finite field GF(3^9) arithmetic for Reed-Solomon parity.
 * Primitive polynomial: x^9 + 2x^3 + 2x^2 + 2x + 1
 */
class GF3_9 {
public:
  using value_type = uint16_t;
  static constexpr value_type kOrder = 19683;  // 3^9

  static value_type add(value_type a, value_type b) {
    value_type res = 0;
    value_type p3 = 1;
    for (int i = 0; i < 9; ++i) {
      int da = (a / p3) % 3;
      int db = (b / p3) % 3;
      int dr = (da + db) % 3;
      res += dr * p3;
      p3 *= 3;
    }
    return res;
  }

  static value_type sub(value_type a, value_type b) {
    value_type res = 0;
    value_type p3 = 1;
    for (int i = 0; i < 9; ++i) {
      int da = (a / p3) % 3;
      int db = (b / p3) % 3;
      int dr = (da - db + 3) % 3;
      res += dr * p3;
      p3 *= 3;
    }
    return res;
  }

  static value_type mul(value_type a, value_type b) {
    std::array<int, 18> product{};
    value_type p3a = 1;
    for (int i = 0; i < 9; ++i) {
      int da = (a / p3a) % 3;
      if (da == 0) {
        p3a *= 3;
        continue;
      }
      value_type p3b = 1;
      for (int j = 0; j < 9; ++j) {
        int db = (b / p3b) % 3;
        product[i + j] = (product[i + j] + da * db) % 3;
        p3b *= 3;
      }
      p3a *= 3;
    }

    // Reduce modulo x^9 + 2x^3 + 2x^2 + 2x + 1
    // x^9 = x^3 + x^2 + x + 2 (mod 3)
    for (int i = 17; i >= 9; --i) {
      int coeff = product[i];
      if (coeff == 0) continue;
      product[i - 9 + 3] = (product[i - 9 + 3] + coeff) % 3;
      product[i - 9 + 2] = (product[i - 9 + 2] + coeff) % 3;
      product[i - 9 + 1] = (product[i - 9 + 1] + coeff) % 3;
      product[i - 9 + 0] = (product[i - 9 + 0] + 2 * coeff) % 3;
      product[i] = 0;
    }

    value_type res = 0;
    value_type p3 = 1;
    for (int i = 0; i < 9; ++i) {
      res += product[i] * p3;
      p3 *= 3;
    }
    return res;
  }

  static value_type inv(value_type a) {
    if (a == 0) return 0;  // Should handle error
    // a^(3^9 - 2)
    value_type res = 1;
    value_type base = a;
    uint32_t exp = kOrder - 2;
    while (exp > 0) {
      if (exp % 2 == 1) res = mul(res, base);
      base = mul(base, base);
      exp /= 2;
    }
    return res;
  }
};

}  // namespace t81::canonfs
