#include <cassert>
#include <stdexcept>
#include <t81/bigint.hpp>
#include <utility>
#include <vector>

using t81::DivModResult;
using t81::gcd;
using t81::T81BigInt;

static void test_divmod_basic_cases() {
  struct Case {
    long long a;
    long long b;
  };

  const std::vector<Case> cases = {
      {0, 1},    {0, -1}, {5, 2},   {5, -2},  {-5, 2},   {-5, -2}, {13, 5}, {13, -5}, {-13, 5},
      {-13, -5}, {42, 7}, {42, -7}, {-42, 7}, {-42, -7}, {1, 2},   {-1, 2}, {1, -2},  {-1, -2},
  };

  for (auto c : cases) {
    T81BigInt A(c.a);
    T81BigInt B(c.b);

    [[maybe_unused]] DivModResult dm = t81::divmod(A, B);

    // Invariant: a = b*q + r
    [[maybe_unused]] T81BigInt lhs = B * dm.q + dm.r;
    assert(lhs == A && "divmod invariant a = b*q + r failed");

    // Remainder constraints: 0 <= r < |b|
    assert(!dm.r.is_negative() && "divmod remainder must be non-negative");

    [[maybe_unused]] T81BigInt absB = B.is_negative() ? B.abs() : B;
    assert(dm.r < absB && "divmod remainder must satisfy r < |b|");
  }
}

static void test_gcd_basic_cases() {
  [[maybe_unused]] auto make = [](long long x) { return T81BigInt(x); };

  struct Case {
    long long a;
    long long b;
    long long g;
  };

  const std::vector<Case> cases = {
      {0, 0, 0},     {0, 5, 5},  {5, 0, 5},   {48, 18, 6}, {-48, 18, 6}, {48, -18, 6},
      {-48, -18, 6}, {7, 13, 1}, {-7, 13, 1}, {7, -13, 1}, {-7, -13, 1},
  };

  for (auto c : cases) {
    [[maybe_unused]] T81BigInt A = make(c.a);
    [[maybe_unused]] T81BigInt B = make(c.b);
    [[maybe_unused]] T81BigInt G = gcd(A, B);

    // gcd non-negative
    assert(!G.is_negative());

    // gcd correct for known pairs
    assert(G == make(c.g) && "gcd basic case failed");

    // Divides a and b: a % G == 0, b % G == 0
    if (!G.is_zero()) {
      [[maybe_unused]] DivModResult da = t81::divmod(A, G);
      [[maybe_unused]] DivModResult db = t81::divmod(B, G);
      assert(da.r.is_zero());
      assert(db.r.is_zero());
    }
  }
}

static void test_base81_roundtrip() {
  using t81::T81BigInt;
  std::vector<std::string> cases = {
      "0",   "1",
      "+",    // 62
      "+ω",   // leading digit 62 must remain parseable
      "Z",    // 35
      "a",    // 36
      "∞",    // multi-byte codepoint
      "1∞",   // multi-digit, high codepoint
      "-σω",  // negative, multi-digit
  };
  for (const auto& s : cases) {
    [[maybe_unused]] T81BigInt a = T81BigInt::from_base81_string(s);
    [[maybe_unused]] auto t = a.to_base81_string();
    // The to_base81_string may normalize leading zeros; reparse and compare values.
    [[maybe_unused]] T81BigInt b = T81BigInt::from_base81_string(t);
    assert(a == b);
  }
  [[maybe_unused]] bool threw = false;
  try {
    (void)T81BigInt::from_base81_string("~");
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
  threw = false;
  try {
    (void)T81BigInt::from_base81_string("00");
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);

  {
    T81BigInt sixty_two = T81BigInt::from_base81_string("+");
    assert(sixty_two.to_int64() == 62);
    T81BigInt neg_sixty_two = T81BigInt::from_base81_string("-+");
    assert(neg_sixty_two.to_int64() == -62);
  }
}

static void test_pow_basic_cases() {
  [[maybe_unused]] auto make = [](long long x) { return T81BigInt(x); };

  struct Case {
    long long base;
    long long exp;
    long long expected;
  };

  const std::vector<Case> cases = {
      {2, 3, 8},  {-2, 3, -8}, {2, 4, 16},  {-2, 4, 16}, {5, 0, 1},
      {-5, 0, 1}, {5, 1, 5},   {-5, 1, -5}, {0, 5, 0},   {3, 5, 243},
  };

  for (auto c : cases) {
    [[maybe_unused]] T81BigInt base = make(c.base);
    [[maybe_unused]] T81BigInt exp = make(c.exp);
    [[maybe_unused]] T81BigInt expected = make(c.expected);
    [[maybe_unused]] T81BigInt result = T81BigInt::pow(base, exp);
    assert(result == expected);
  }

  [[maybe_unused]] bool threw = false;
  try {
    (void)T81BigInt::pow(make(2), make(-3));
  } catch (const std::domain_error&) {
    threw = true;
  }
  assert(threw);
}

int main() {
  // existing tests...
  // test_existing_roundtrip();

  test_divmod_basic_cases();
  test_gcd_basic_cases();
  test_base81_roundtrip();
  test_pow_basic_cases();

  return 0;
}
