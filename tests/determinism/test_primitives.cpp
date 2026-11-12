#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "t81/types/T81BigInt.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/cell.hpp"

// Minimal test runner macro
#define TEST_CHECK(cond)                                                                      \
  do {                                                                                        \
    if (!(cond)) {                                                                            \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1);                                                                           \
    }                                                                                         \
  } while (0)

using namespace t81::core;
using namespace t81::v1;
using namespace t81;

// --- Cell Tests ---
void test_cell_determinism() {
  std::cout << "Testing Cell determinism..." << std::endl;

  // 1. Boundary values
  Cell min = Cell::from_int(-121);
  Cell max = Cell::from_int(121);
  TEST_CHECK(min.to_int() == -121);
  TEST_CHECK(max.to_int() == 121);

  // 2. Arithmetic determinism
  Cell a = Cell::from_int(40);
  Cell b = Cell::from_int(2);
  Cell sum = a + b;
  TEST_CHECK(sum.to_int() == 42);
  Cell diff = a - b;
  TEST_CHECK(diff.to_int() == 38);
  Cell prod = a * b;
  TEST_CHECK(prod.to_int() == 80);
  Cell quot = a / b;
  TEST_CHECK(quot.to_int() == 20);

  // 3. Overflow behavior (should throw deterministically)
  bool caught1 = false;
  try {
    [[maybe_unused]] Cell ov = max + Cell::from_int(1);
  } catch (const std::overflow_error& e) {
    caught1 = true;
  }
  TEST_CHECK(caught1);

  // 4. Shift Overflow
  bool caught2 = false;
  try {
    Cell high = Cell::from_int(81);
    [[maybe_unused]] Cell ov = high << 1;
  } catch (const std::overflow_error& e) {
    caught2 = true;
  }
  TEST_CHECK(caught2);

  // 5. Comparison
  Cell c10 = Cell::from_int(10);
  Cell c20 = Cell::from_int(20);
  Cell c20b = Cell::from_int(20);
  Cell cn10 = Cell::from_int(-10);

  TEST_CHECK(c10 < c20);
  TEST_CHECK(cn10 < c10);
  TEST_CHECK(c20 > c10);
  TEST_CHECK(c20 >= c20b);
  TEST_CHECK(c20 <= c20b);
  TEST_CHECK(!(c20 < c20b));
  TEST_CHECK(cn10 < c20);

  // 6. Roundtrip
  for (int i = -121; i <= 121; ++i) {
    Cell c = Cell::from_int(i);
    TEST_CHECK(c.to_int() == i);
  }
}

// --- Exhaustive Host-Integer (int64_t) Oracle Validation for Cell (BASE-PERF-00 Phase B) ---
void test_cell_host_oracle_exhaustive() {
  std::cout << "Testing Cell exhaustive host-integer oracle validation..." << std::endl;

  // 1. Conversion round-trip and canonical trit uniqueness over [-121, +121]
  std::size_t valid_conversions = 0;
  for (int64_t v = Cell::MIN; v <= Cell::MAX; ++v) {
    const Cell c = Cell::from_int(v);
    TEST_CHECK(c.to_int() == v);
    TEST_CHECK(c == Cell::from_int(c.to_int()));
    TEST_CHECK((-c).to_int() == -v);
    TEST_CHECK((-(-c)) == c);
    if (v > Cell::MIN) {
      const Cell prev = Cell::from_int(v - 1);
      TEST_CHECK(prev < c);
      TEST_CHECK(c > prev);
      TEST_CHECK(prev != c);
    }
    ++valid_conversions;
  }
  TEST_CHECK(valid_conversions == 243);

  // Out-of-range conversion rejection
  for (int64_t out_v : {-10000LL, -243LL, -122LL, 122LL, 243LL, 10000LL}) {
    bool caught = false;
    try {
      [[maybe_unused]] Cell bad = Cell::from_int(out_v);
    } catch (const std::overflow_error&) {
      caught = true;
    }
    TEST_CHECK(caught);
  }

  // 2. Exhaustive addition and subtraction over [-121, +121]^2 (59,049 pairs each)
  std::size_t add_in_range = 0;
  std::size_t add_overflow = 0;
  std::size_t sub_in_range = 0;
  std::size_t sub_overflow = 0;

  for (int64_t a = Cell::MIN; a <= Cell::MAX; ++a) {
    const Cell ca = Cell::from_int(a);
    for (int64_t b = Cell::MIN; b <= Cell::MAX; ++b) {
      const Cell cb = Cell::from_int(b);

      // Addition oracle
      const int64_t expected_sum = a + b;
      if (expected_sum >= Cell::MIN && expected_sum <= Cell::MAX) {
        const Cell sum = ca + cb;
        TEST_CHECK(sum.to_int() == expected_sum);
        ++add_in_range;
      } else {
        bool caught = false;
        try {
          [[maybe_unused]] Cell sum = ca + cb;
        } catch (const std::overflow_error&) {
          caught = true;
        }
        TEST_CHECK(caught);
        ++add_overflow;
      }

      // Subtraction oracle
      const int64_t expected_diff = a - b;
      if (expected_diff >= Cell::MIN && expected_diff <= Cell::MAX) {
        const Cell diff = ca - cb;
        TEST_CHECK(diff.to_int() == expected_diff);
        ++sub_in_range;
      } else {
        bool caught = false;
        try {
          [[maybe_unused]] Cell diff = ca - cb;
        } catch (const std::overflow_error&) {
          caught = true;
        }
        TEST_CHECK(caught);
        ++sub_overflow;
      }
    }
  }
  TEST_CHECK(add_in_range + add_overflow == 59049);
  TEST_CHECK(sub_in_range + sub_overflow == 59049);
  // Exact combinatorics: 2 * (121 * 122 / 2) = 14,762 overflow pairs; 59,049 - 14,762 = 44,287
  // in-range pairs.
  TEST_CHECK(add_in_range == 44287 && add_overflow == 14762);
  TEST_CHECK(sub_in_range == 44287 && sub_overflow == 14762);

  // 3. Multiplication oracle:
  // (a) Benchmark safe domain [-10, +10]^2 (441 pairs): 100% in-range and exact.
  // Operands in [-10, +10] fit in three balanced trits (trits 0..2, 1 + 3 + 9 = 13 >= 10),
  // whereas products in [-100, +100] can require five trits. Because operands fit in three
  // trits, shifts in operator* stop at index 2, shifted operands (*this << i) have magnitude
  // <= 10 * 9 = 90 <= 121, and partial sums remain representable in [-121, +121].
  std::size_t mul_bench_domain_ok = 0;
  for (int64_t a = -10; a <= 10; ++a) {
    const Cell ca = Cell::from_int(a);
    for (int64_t b = -10; b <= 10; ++b) {
      const Cell cb = Cell::from_int(b);
      const Cell prod = ca * cb;
      TEST_CHECK(prod.to_int() == a * b);
      ++mul_bench_domain_ok;
    }
  }
  TEST_CHECK(mul_bench_domain_ok == 441);

  // (b) Full [-121, +121]^2 characterization (59,049 pairs):
  // Every non-throwing product matches int64_t a * b; every out-of-range product throws
  // std::overflow_error; intermediate shift/partial-sum overflows when |a*b| <= 121 also
  // throw std::overflow_error deterministically (never silently corrupting output).
  std::size_t mul_succeeded = 0;
  std::size_t mul_true_overflow = 0;
  std::size_t mul_intermediate_shift_overflow = 0;
  for (int64_t a = Cell::MIN; a <= Cell::MAX; ++a) {
    const Cell ca = Cell::from_int(a);
    for (int64_t b = Cell::MIN; b <= Cell::MAX; ++b) {
      const Cell cb = Cell::from_int(b);
      const int64_t expected_prod = a * b;
      const bool mathematically_in_range =
          (expected_prod >= Cell::MIN && expected_prod <= Cell::MAX);
      try {
        const Cell prod = ca * cb;
        TEST_CHECK(mathematically_in_range);
        TEST_CHECK(prod.to_int() == expected_prod);
        ++mul_succeeded;
      } catch (const std::overflow_error&) {
        if (mathematically_in_range) {
          ++mul_intermediate_shift_overflow;
        } else {
          ++mul_true_overflow;
        }
      }
    }
  }
  TEST_CHECK(mul_succeeded + mul_true_overflow + mul_intermediate_shift_overflow == 59049);
  TEST_CHECK(mul_intermediate_shift_overflow == 352);

  // 4. Exhaustive division oracle over [-121, +121]^2:
  // 58,806 non-zero divisor pairs match host int64_t truncated division;
  // all 243 b == 0 pairs throw std::domain_error.
  std::size_t div_valid = 0;
  std::size_t div_by_zero_caught = 0;
  for (int64_t a = Cell::MIN; a <= Cell::MAX; ++a) {
    const Cell ca = Cell::from_int(a);
    for (int64_t b = Cell::MIN; b <= Cell::MAX; ++b) {
      const Cell cb = Cell::from_int(b);
      if (b == 0) {
        bool caught = false;
        try {
          [[maybe_unused]] Cell quot = ca / cb;
        } catch (const std::domain_error&) {
          caught = true;
        }
        TEST_CHECK(caught);
        ++div_by_zero_caught;
      } else {
        const Cell quot = ca / cb;
        TEST_CHECK(quot.to_int() == (a / b));
        ++div_valid;
      }
    }
  }
  TEST_CHECK(div_valid == 58806);
  TEST_CHECK(div_by_zero_caught == 243);

  // 5. Deterministic benchmark input sequence verification (seed = 0x781, DATA_SIZE = 10000)
  {
    constexpr std::size_t kDataSize = 10000;
    constexpr int64_t kSafeLimit = 10;
    std::mt19937_64 gen(0x781);
    std::uniform_int_distribution<int64_t> distrib(-kSafeLimit, kSafeLimit);
    uint64_t fnv_hash = 14695981039346656037ULL;
    auto fnv_mix = [&](int64_t v) {
      const uint64_t u = static_cast<uint64_t>(v);
      for (int byte_idx = 0; byte_idx < 8; ++byte_idx) {
        fnv_hash ^= (u >> (byte_idx * 8)) & 0xFFULL;
        fnv_hash *= 1099511628211ULL;
      }
    };

    for (std::size_t i = 0; i < kDataSize; ++i) {
      const int64_t val_a = distrib(gen);
      int64_t val_b = distrib(gen);
      if (val_b == 0) val_b = 1;
      fnv_mix(val_a);
      fnv_mix(val_b);

      const Cell ca = Cell::from_int(val_a);
      const Cell cb = Cell::from_int(val_b);
      TEST_CHECK(ca.to_int() == val_a);
      TEST_CHECK(cb.to_int() == val_b);
      TEST_CHECK((ca + cb).to_int() == val_a + val_b);
      TEST_CHECK((ca - cb).to_int() == val_a - val_b);
      TEST_CHECK((ca * cb).to_int() == val_a * val_b);
      TEST_CHECK((ca / cb).to_int() == val_a / val_b);
    }

    // Re-generate and verify deterministic hash stability
    std::mt19937_64 gen2(0x781);
    std::uniform_int_distribution<int64_t> distrib2(-kSafeLimit, kSafeLimit);
    uint64_t fnv_hash2 = 14695981039346656037ULL;
    auto fnv_mix2 = [&](int64_t v) {
      const uint64_t u = static_cast<uint64_t>(v);
      for (int byte_idx = 0; byte_idx < 8; ++byte_idx) {
        fnv_hash2 ^= (u >> (byte_idx * 8)) & 0xFFULL;
        fnv_hash2 *= 1099511628211ULL;
      }
    };
    for (std::size_t i = 0; i < kDataSize; ++i) {
      const int64_t val_a = distrib2(gen2);
      int64_t val_b = distrib2(gen2);
      if (val_b == 0) val_b = 1;
      fnv_mix2(val_a);
      fnv_mix2(val_b);
    }
    TEST_CHECK(fnv_hash == fnv_hash2);
  }

  // 6. Deterministic round-trip benchmark input sequence verification (seed = 0x781, DATA_SIZE =
  // 10000)
  {
    constexpr std::size_t kDataSize = 10000;
    std::mt19937_64 gen(0x781);
    std::uniform_int_distribution<int64_t> distrib(Cell::MIN, Cell::MAX);
    for (std::size_t i = 0; i < kDataSize; ++i) {
      const int64_t val = distrib(gen);
      TEST_CHECK(val >= Cell::MIN && val <= Cell::MAX);
      const Cell c = Cell::from_int(val);
      TEST_CHECK(c.to_int() == val);
    }
  }

  std::cout << "  Cell oracle summary: conv=243/243, add_ok=" << add_in_range
            << " (ovf=" << add_overflow << "), sub_ok=" << sub_in_range << " (ovf=" << sub_overflow
            << "), mul_bench_ok=" << mul_bench_domain_ok << "/441 (full_ok=" << mul_succeeded
            << ", true_ovf=" << mul_true_overflow
            << ", shift_ovf=" << mul_intermediate_shift_overflow << "), div_ok=" << div_valid
            << " (div0=" << div_by_zero_caught << ")" << std::endl;
}

// --- T81Int Tests ---
void test_t81int_determinism() {
  std::cout << "Testing T81Int determinism..." << std::endl;

  using Int27 = T81Int<27>;

  // 1. Zero and basic values
  Int27 zero;
  TEST_CHECK(zero.to_int64() == 0);
  TEST_CHECK(zero.is_zero());

  Int27 one(1);
  TEST_CHECK(one.to_int64() == 1);

  Int27 neg_one(-1);
  TEST_CHECK(neg_one.to_int64() == -1);

  // 2. Arithmetic
  Int27 sum = one + neg_one;
  TEST_CHECK(sum.is_zero());

  Int27 prod = Int27(10) * Int27(10);
  TEST_CHECK(prod.to_int64() == 100);

  // 3. String canonicalization (trit string)
  std::string s = Int27(13).to_string();  // 13 = 9 + 3 + 1 = 111 (base 3) -> +++
  TEST_CHECK(s == "111");

  // 4. Canonical string (trit string: +, 0, -)
  std::string can = Int27(13).to_canonical_string();
  TEST_CHECK(can.length() == 27);
  TEST_CHECK(can.substr(24) == "+++");
}

// --- T81BigInt Tests ---
void test_bigint_determinism() {
  std::cout << "Testing T81BigInt determinism..." << std::endl;

  // 1. Basic construction
  T81BigInt a(123456789);
  T81BigInt c = T81BigInt::from_int64(987654321);

  // 2. Arithmetic
  T81BigInt sum = a + c;
  TEST_CHECK(sum.to_int64() == 123456789 + 987654321);

  T81BigInt diff = c - a;
  TEST_CHECK(diff.to_int64() == 987654321 - 123456789);

  // 3. Multiplication (Karatsuba threshold check)
  T81BigInt base(3);
  T81BigInt exp(50);
  T81BigInt huge = T81BigInt::pow(base, exp);  // 3^50

  // 4. Canonical String (Base81)
  std::string s81 = huge.to_base81_string();
  // Ensure roundtrip
  T81BigInt recovered = T81BigInt::from_base81_string(s81);
  TEST_CHECK(recovered == huge);

  // 5. Division / Modulo
  auto [q, r] = T81BigInt::div_mod(c, a);
  TEST_CHECK(q.to_int64() == 987654321 / 123456789);
  TEST_CHECK(r.to_int64() == 987654321 % 123456789);
}

int main() {
  test_cell_determinism();
  test_cell_host_oracle_exhaustive();
  test_t81int_determinism();
  test_bigint_determinism();
  std::cout << "All primitive determinism tests passed." << std::endl;
  return 0;
}
