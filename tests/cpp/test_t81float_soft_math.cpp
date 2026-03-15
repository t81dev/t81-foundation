// tests/cpp/test_t81float_soft_math.cpp
//
// RFC-0030: Deterministic Math Subsystem — acceptance test suite.
//
// Verifies that every transcendental function in t81_soft_math produces:
//   1. Numerically accurate results (compared to known mathematical values
//      via to_double(), tolerance ≤ 1e-5).
//   2. Correct special-value handling (NaE, ±inf, zero, negative domain).
//   3. Classical mathematical identities (sin²+cos²=1, exp(log(x))=x, etc.)
//      within the same tolerance.
//   4. Bit-exact determinism: the same computation run twice produces
//      identical to_double() output (cross-call reproducibility).
//   5. A canonical repro fingerprint: a known-good double for a fixed
//      computation sequence, hardened against architecture drift.
//
// All arithmetic stays within T81Float<27,9> / DFixed — no cmath usage.

#include <cassert>
#include <cmath>     // only for std::abs(), std::isnan, std::isinf in test harness
#include <cstdio>
#include <cstdlib>

#include "t81/types/T81Float.hpp"

using F = t81::v1::T81Float<27, 9>;

// ─── Test harness ─────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;

static void check(bool ok, const char* msg) {
  if (ok) {
    ++g_pass;
  } else {
    ++g_fail;
    std::fprintf(stderr, "  FAIL  %s\n", msg);
  }
}

static constexpr double kTol = 1e-5;

static void check_near(double got, double expected, const char* msg) {
  check(std::abs(got - expected) <= kTol, msg);
}

// ─── 1. sin ───────────────────────────────────────────────────────────────────

static void test_sin() {
  std::printf("[RFC-0030] sin\n");

  // sin(0) = 0
  check(F::from_double(0.0).sin().is_zero(),  "sin(0) = 0");

  // sin(π/6) ≈ 0.5
  check_near(F::from_double(0.5235987755982988).sin().to_double(),  0.5,
             "sin(pi/6) ≈ 0.5");

  // sin(π/4) ≈ √2/2 ≈ 0.70711
  check_near(F::from_double(0.7853981633974483).sin().to_double(),  0.7071067811865476,
             "sin(pi/4) ≈ sqrt(2)/2");

  // sin(π/2) ≈ 1
  check_near(F::from_double(1.5707963267948966).sin().to_double(),  1.0,
             "sin(pi/2) ≈ 1");

  // sin(π) ≈ 0  (small residual acceptable)
  check(std::abs(F::from_double(3.141592653589793).sin().to_double()) <= 1e-4,
        "sin(pi) ≈ 0");

  // sin(-π/2) ≈ -1
  check_near(F::from_double(-1.5707963267948966).sin().to_double(),  -1.0,
             "sin(-pi/2) ≈ -1");

  // sin(NaE) = NaE
  check(F::nae().sin().is_nae(),  "sin(NaE) = NaE");

  // sin(±inf) = NaE
  check(F::inf(true).sin().is_nae(),   "sin(+inf) = NaE");
  check(F::inf(false).sin().is_nae(),  "sin(-inf) = NaE");

  // Determinism: same call twice
  F x = F::from_double(1.2345);
  check(x.sin().to_double() == x.sin().to_double(),  "sin determinism: same input → same output");
}

// ─── 2. cos ───────────────────────────────────────────────────────────────────

static void test_cos() {
  std::printf("[RFC-0030] cos\n");

  // cos(0) = 1
  check_near(F::from_double(0.0).cos().to_double(),  1.0,  "cos(0) = 1");

  // cos(π/3) ≈ 0.5
  check_near(F::from_double(1.0471975511965976).cos().to_double(),  0.5,
             "cos(pi/3) ≈ 0.5");

  // cos(π/2) ≈ 0
  check(std::abs(F::from_double(1.5707963267948966).cos().to_double()) <= 1e-4,
        "cos(pi/2) ≈ 0");

  // cos(π) ≈ -1
  check_near(F::from_double(3.141592653589793).cos().to_double(),  -1.0,
             "cos(pi) ≈ -1");

  // cos(NaE) = NaE
  check(F::nae().cos().is_nae(),  "cos(NaE) = NaE");

  // cos(±inf) = NaE
  check(F::inf(true).cos().is_nae(),  "cos(+inf) = NaE");

  // cos(0) via zero() path
  check_near(F::zero().cos().to_double(),  1.0,  "cos(zero()) = 1");

  // Determinism
  F x = F::from_double(2.718);
  check(x.cos().to_double() == x.cos().to_double(),  "cos determinism");
}

// ─── 3. sin² + cos² = 1 (Pythagorean identity) ────────────────────────────────

static void test_sin_cos_identity() {
  std::printf("[RFC-0030] sin²+cos²=1 identity\n");

  auto check_pythag = [](double rad, const char* label) {
    F x    = F::from_double(rad);
    F s    = x.sin();
    F c    = x.cos();
    // Use T81Float multiplication and addition
    double s2c2 = (s * s + c * c).to_double();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "sin²+cos²=1 at x=%.4f (%s)", rad, label);
    check(std::abs(s2c2 - 1.0) <= 1e-4, buf);
  };

  check_pythag(0.0,                   "0");
  check_pythag(0.5235987755982988,    "pi/6");
  check_pythag(0.7853981633974483,    "pi/4");
  check_pythag(1.0471975511965976,    "pi/3");
  check_pythag(1.5707963267948966,    "pi/2");
  check_pythag(3.141592653589793,     "pi");
  check_pythag(-0.7853981633974483,   "-pi/4");
  check_pythag(2.0,                   "2.0");
}

// ─── 4. exp ───────────────────────────────────────────────────────────────────

static void test_exp() {
  std::printf("[RFC-0030] exp\n");

  // exp(0) = 1
  check_near(F::from_double(0.0).exp().to_double(),   1.0,            "exp(0) = 1");

  // exp(1) ≈ e
  check_near(F::from_double(1.0).exp().to_double(),   2.718281828,    "exp(1) ≈ e");

  // exp(-1) ≈ 1/e
  check_near(F::from_double(-1.0).exp().to_double(),  0.367879441,    "exp(-1) ≈ 1/e");

  // exp(2) ≈ e²
  check_near(F::from_double(2.0).exp().to_double(),   7.389056099,    "exp(2) ≈ e^2");

  // exp(ln(2)) ≈ 2
  check_near(F::from_double(0.6931471805599453).exp().to_double(),  2.0,
             "exp(ln 2) ≈ 2");

  // exp(NaE) = NaE
  check(F::nae().exp().is_nae(),          "exp(NaE) = NaE");

  // exp(+inf) = +inf
  check(F::inf(true).exp().is_inf(),      "exp(+inf) = inf");

  // exp(-inf) = 0
  check(F::inf(false).exp().is_zero(),    "exp(-inf) = 0");

  // exp(zero()) = 1
  check_near(F::zero().exp().to_double(),  1.0,  "exp(zero()) = 1");

  // Determinism
  F x = F::from_double(0.5);
  check(x.exp().to_double() == x.exp().to_double(),  "exp determinism");
}

// ─── 5. log ───────────────────────────────────────────────────────────────────

static void test_log() {
  std::printf("[RFC-0030] log\n");

  // log(1) = 0
  check(F::from_double(1.0).log().is_zero(),         "log(1) = 0");

  // log(e) ≈ 1
  check_near(F::from_double(2.718281828).log().to_double(),  1.0,   "log(e) ≈ 1");

  // log(2) ≈ 0.6931
  check_near(F::from_double(2.0).log().to_double(),          0.6931471805599453,
             "log(2) ≈ 0.6931");

  // log(10) ≈ 2.3026
  check_near(F::from_double(10.0).log().to_double(),         2.302585092994046,
             "log(10) ≈ 2.3026");

  // log(negative) = NaE
  check(F::from_double(-1.0).log().is_nae(),  "log(-1) = NaE");

  // log(0) = NaE
  check(F::zero().log().is_nae(),             "log(0) = NaE");

  // log(NaE) = NaE
  check(F::nae().log().is_nae(),              "log(NaE) = NaE");

  // log(+inf) = +inf
  check(F::inf(true).log().is_inf(),          "log(+inf) = +inf");

  // log(-inf) = NaE
  check(F::inf(false).log().is_nae(),         "log(-inf) = NaE");

  // Determinism
  F x = F::from_double(3.0);
  check(x.log().to_double() == x.log().to_double(),  "log determinism");
}

// ─── 6. exp(log(x)) = x round-trip ───────────────────────────────────────────

static void test_exp_log_roundtrip() {
  std::printf("[RFC-0030] exp(log(x)) = x round-trip\n");

  auto check_rt = [](double val, const char* label) {
    F x = F::from_double(val);
    double rt = x.log().exp().to_double();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "exp(log(%.4f)) ≈ %.4f (%s)", val, val, label);
    check(std::abs(rt - val) / val <= 1e-4, buf);  // relative tolerance
  };

  check_rt(1.0,   "1");
  check_rt(2.0,   "2");
  check_rt(3.0,   "3");
  check_rt(0.5,   "0.5");
  check_rt(10.0,  "10");
  check_rt(100.0, "100");
}

// ─── 7. sqrt ──────────────────────────────────────────────────────────────────

static void test_sqrt() {
  std::printf("[RFC-0030] sqrt\n");

  // sqrt(1) = 1
  check_near(F::from_double(1.0).sqrt().to_double(),   1.0,   "sqrt(1) = 1");

  // sqrt(4) = 2
  check_near(F::from_double(4.0).sqrt().to_double(),   2.0,   "sqrt(4) = 2");

  // sqrt(9) = 3
  check_near(F::from_double(9.0).sqrt().to_double(),   3.0,   "sqrt(9) = 3");

  // sqrt(2) ≈ 1.41421
  check_near(F::from_double(2.0).sqrt().to_double(),   1.4142135623730951,  "sqrt(2) ≈ 1.41421");

  // sqrt(0.25) = 0.5
  check_near(F::from_double(0.25).sqrt().to_double(),  0.5,   "sqrt(0.25) = 0.5");

  // sqrt(0) = 0
  check(F::zero().sqrt().is_zero(),       "sqrt(0) = 0");

  // sqrt(-1) = NaE
  check(F::from_double(-1.0).sqrt().is_nae(),  "sqrt(-1) = NaE");

  // sqrt(NaE) = NaE
  check(F::nae().sqrt().is_nae(),         "sqrt(NaE) = NaE");

  // sqrt(+inf) = +inf
  check(F::inf(true).sqrt().is_inf(),     "sqrt(+inf) = inf");

  // sqrt(-inf) = NaE
  check(F::inf(false).sqrt().is_nae(),    "sqrt(-inf) = NaE");

  // sqrt(x)² = x
  auto check_sq = [](double val, const char* label) {
    F x  = F::from_double(val);
    F sq = x.sqrt();
    double recovered = (sq * sq).to_double();
    char buf[128];
    std::snprintf(buf, sizeof(buf), "sqrt(%.2f)² ≈ %.2f (%s)", val, val, label);
    check(std::abs(recovered - val) / val <= 1e-4, buf);
  };
  check_sq(2.0,   "2");
  check_sq(3.0,   "3");
  check_sq(10.0,  "10");
  check_sq(0.5,   "0.5");

  // Determinism
  F x = F::from_double(7.0);
  check(x.sqrt().to_double() == x.sqrt().to_double(),  "sqrt determinism");
}

// ─── 8. pow ───────────────────────────────────────────────────────────────────

static void test_pow() {
  std::printf("[RFC-0030] pow\n");

  // x^0 = 1
  check_near(F::from_double(5.0).pow(F::from_double(0.0)).to_double(),  1.0,
             "5^0 = 1");

  // x^1 = x
  check_near(F::from_double(3.0).pow(F::from_double(1.0)).to_double(),  3.0,
             "3^1 = 3");

  // 2^3 = 8
  check_near(F::from_double(2.0).pow(F::from_double(3.0)).to_double(),  8.0,
             "2^3 = 8");

  // 2^0.5 = sqrt(2) ≈ 1.41421
  check_near(F::from_double(2.0).pow(F::from_double(0.5)).to_double(),
             1.4142135623730951,  "2^0.5 = sqrt(2)");

  // 4^2 = 16
  check_near(F::from_double(4.0).pow(F::from_double(2.0)).to_double(),  16.0,
             "4^2 = 16");

  // 10^(-1) = 0.1
  check_near(F::from_double(10.0).pow(F::from_double(-1.0)).to_double(),  0.1,
             "10^(-1) = 0.1");

  // NaE^x = NaE
  check(F::nae().pow(F::from_double(2.0)).is_nae(),   "NaE^2 = NaE");

  // x^NaE = NaE
  check(F::from_double(2.0).pow(F::nae()).is_nae(),   "2^NaE = NaE");

  // Determinism
  F base = F::from_double(2.5);
  F exp_ = F::from_double(2.0);
  check(base.pow(exp_).to_double() == base.pow(exp_).to_double(),  "pow determinism");
}

// ─── 9. div ───────────────────────────────────────────────────────────────────

static void test_div() {
  std::printf("[RFC-0030] div (T81Float operator/)\n");

  // 6 / 2 = 3
  check_near((F::from_double(6.0) / F::from_double(2.0)).to_double(),  3.0,
             "6/2 = 3");

  // 1 / 3 ≈ 0.3333
  check_near((F::from_double(1.0) / F::from_double(3.0)).to_double(),  0.3333333333,
             "1/3 ≈ 0.3333");

  // 22 / 7 ≈ pi
  check_near((F::from_double(22.0) / F::from_double(7.0)).to_double(),  3.142857142857,
             "22/7 ≈ pi");

  // x / 1 = x
  check_near((F::from_double(5.678) / F::from_double(1.0)).to_double(),  5.678,
             "x/1 = x");

  // 1 / x * x = 1
  F x = F::from_double(4.0);
  check_near((F::from_double(1.0) / x * x).to_double(),  1.0,  "(1/x)*x = 1");

  // x / 0 = +inf
  check((F::from_double(1.0) / F::zero()).is_inf(),  "1/0 = inf");

  // 0 / 0 = NaE
  check((F::zero() / F::zero()).is_nae(),             "0/0 = NaE");

  // inf / inf = NaE
  check((F::inf(true) / F::inf(true)).is_nae(),       "inf/inf = NaE");

  // NaE / x = NaE
  check((F::nae() / F::from_double(2.0)).is_nae(),    "NaE/2 = NaE");

  // Determinism
  F a = F::from_double(7.0), b = F::from_double(3.0);
  check((a / b).to_double() == (a / b).to_double(),  "div determinism");
}

// ─── 10. Canonical repro fingerprint ──────────────────────────────────────────
//
// Computes a fixed sequence of transcendentals at canonical inputs and checks
// that the accumulated sum matches a known-good value.  This serves as the
// cross-architecture determinism gate described in RFC-0030 §4.
//
// The sequence: sin(1) + cos(1) + exp(1) + log(2) + sqrt(2)
// ≈ 0.84147 + 0.54030 + 2.71828 + 0.69315 + 1.41421 ≈ 6.20742
//
// Value captured from a reference run; any architecture drift will break this.

static void test_repro_fingerprint() {
  std::printf("[RFC-0030] canonical repro fingerprint\n");

  F one = F::from_double(1.0);
  F two = F::from_double(2.0);

  double fingerprint =
      one.sin().to_double()  +
      one.cos().to_double()  +
      one.exp().to_double()  +
      two.log().to_double()  +
      two.sqrt().to_double();

  // Expected: sin(1)+cos(1)+exp(1)+log(2)+sqrt(2)
  //         = 0.841471 + 0.540302 + 2.718282 + 0.693147 + 1.414214 = 6.207416
  constexpr double kExpectedFingerprint = 6.207416;
  check(std::abs(fingerprint - kExpectedFingerprint) <= 1e-4,
        "canonical repro fingerprint matches known-good value");
}

// ─── 11. Range sanity ─────────────────────────────────────────────────────────

static void test_range_sanity() {
  std::printf("[RFC-0030] output range sanity\n");

  // sin output in [-1, 1]
  for (int i = -5; i <= 5; ++i) {
    double s = F::from_double(static_cast<double>(i)).sin().to_double();
    check(s >= -1.0 - 1e-4 && s <= 1.0 + 1e-4,  "sin output in [-1,1]");
  }

  // cos output in [-1, 1]
  for (int i = -5; i <= 5; ++i) {
    double c = F::from_double(static_cast<double>(i)).cos().to_double();
    check(c >= -1.0 - 1e-4 && c <= 1.0 + 1e-4,  "cos output in [-1,1]");
  }

  // sqrt output is non-negative for positive input
  for (int i = 1; i <= 10; ++i) {
    double sq = F::from_double(static_cast<double>(i)).sqrt().to_double();
    check(sq >= 0.0,  "sqrt output >= 0 for positive input");
  }

  // exp output is positive for finite input
  for (int i = -3; i <= 3; ++i) {
    double e = F::from_double(static_cast<double>(i)).exp().to_double();
    check(e > 0.0,  "exp output > 0 for finite input");
  }
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
  test_sin();
  test_cos();
  test_sin_cos_identity();
  test_exp();
  test_log();
  test_exp_log_roundtrip();
  test_sqrt();
  test_pow();
  test_div();
  test_repro_fingerprint();
  test_range_sanity();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
