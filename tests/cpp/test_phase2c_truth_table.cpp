#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>
#include "t81/experimental/packed_trit_vector.hpp"

using namespace t81::experimental;
using namespace t81;

// Helper to format trit
std::string fmt_trit(int8_t t) {
  if (t == 0) return "0";
  if (t == 1) return "1";
  if (t == -1) return "-1";
  return "?";
}

void test_phase2c_truth_table_exhaustive() {
  std::cout << "[Phase 1] Exhaustive Truth-Table Validation (9 cases)..." << std::endl;

  std::vector<int8_t> domain = {-1, 0, 1};
  int failures = 0;

  for (int8_t a : domain) {
    // Unary TNot
    {
      int8_t ref = PackedTritVector::scalar_not(a);

      auto v_a = ComputeTritVector::from_trits({a}).value();

      auto lut_res = v_a.t_not_lut().value().to_trits().value()[0];
      auto swar_res = t81::swar::t_not_swar(v_a).value().to_trits().value()[0];

      if (ref != lut_res || ref != swar_res) {
        std::cerr << "FAIL: TNot(" << fmt_trit(a) << ") -> Ref=" << fmt_trit(ref)
                  << " LUT=" << fmt_trit(lut_res) << " SWAR=" << fmt_trit(swar_res) << std::endl;
        failures++;
      }
    }

    for (int8_t b : domain) {
      auto v_a = ComputeTritVector::from_trits({a}).value();
      auto v_b = ComputeTritVector::from_trits({b}).value();

      // TAnd
      {
        int8_t ref = PackedTritVector::scalar_and(a, b);
        auto lut_res = v_a.t_and_lut(v_b).value().to_trits().value()[0];
        auto swar_res = t81::swar::t_and_swar(v_a, v_b).value().to_trits().value()[0];

        if (ref != lut_res || ref != swar_res) {
          std::cerr << "FAIL: TAnd(" << fmt_trit(a) << ", " << fmt_trit(b)
                    << ") -> Ref=" << fmt_trit(ref) << " LUT=" << fmt_trit(lut_res)
                    << " SWAR=" << fmt_trit(swar_res) << std::endl;
          failures++;
        }
      }

      // TOr
      {
        int8_t ref = PackedTritVector::scalar_or(a, b);
        auto lut_res = v_a.t_or_lut(v_b).value().to_trits().value()[0];
        auto swar_res = t81::swar::t_or_swar(v_a, v_b).value().to_trits().value()[0];

        if (ref != lut_res || ref != swar_res) {
          std::cerr << "FAIL: TOr(" << fmt_trit(a) << ", " << fmt_trit(b)
                    << ") -> Ref=" << fmt_trit(ref) << " LUT=" << fmt_trit(lut_res)
                    << " SWAR=" << fmt_trit(swar_res) << std::endl;
          failures++;
        }
      }

      // TXor (Check LUT path matches Ref)
      {
        int8_t ref = PackedTritVector::scalar_xor(a, b);
        // Note: t_xor uses lut internally in current impl, but we check specific method if exposed,
        // or just t_xor() vs scalar.
        auto res = v_a.t_xor(v_b).value().to_trits().value()[0];

        if (ref != res) {
          std::cerr << "FAIL: TXor(" << fmt_trit(a) << ", " << fmt_trit(b)
                    << ") -> Ref=" << fmt_trit(ref) << " Impl=" << fmt_trit(res) << std::endl;
          failures++;
        }
      }
    }
  }

  if (failures == 0) {
    std::cout << "PASS: All 9x3 truth table cases verified." << std::endl;
  } else {
    std::cerr << "FAIL: " << failures << " mismatches found." << std::endl;
    std::exit(1);
  }
}

void test_txor_commutativity_check() {
  std::cout << "[Phase 7] TXor Commutativity Validation..." << std::endl;
  // TXor is defined as (a - b).
  // Check if there exists (a, b) such that a - b != b - a (wrapped).
  // -1 - 0 = -1.
  // 0 - -1 = 1.
  // -1 != 1. So it is non-commutative.

  int8_t a = -1;
  int8_t b = 0;

  int8_t ab = PackedTritVector::scalar_xor(a, b);
  int8_t ba = PackedTritVector::scalar_xor(b, a);

  std::cout << "TXor(" << fmt_trit(a) << ", " << fmt_trit(b) << ") = " << fmt_trit(ab) << std::endl;
  std::cout << "TXor(" << fmt_trit(b) << ", " << fmt_trit(a) << ") = " << fmt_trit(ba) << std::endl;

  if (ab != ba) {
    std::cout << "PASS: TXor confirmed non-commutative as per spec (Difference)." << std::endl;
  } else {
    // If for some reason the scalar impl changed to be commutative, we must know.
    // But the memory says it is non-commutative.
    std::cerr << "WARNING: TXor appears commutative for this case?" << std::endl;
    // Iterate to find any non-commutative case
    bool found = false;
    for (int8_t x : {-1, 0, 1}) {
      for (int8_t y : {-1, 0, 1}) {
        if (PackedTritVector::scalar_xor(x, y) != PackedTritVector::scalar_xor(y, x)) {
          found = true;
          break;
        }
      }
    }
    if (!found) {
      std::cerr
          << "FAIL: TXor is commutative for all inputs. This contradicts the spec 'Difference'."
          << std::endl;
      std::exit(1);
    }
  }
}

int main() {
  test_phase2c_truth_table_exhaustive();
  test_txor_commutativity_check();
  return 0;
}
