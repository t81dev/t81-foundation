#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
#include "t81/tritwise/tritwise.hpp"

using namespace t81::tritwise;
using t81::ComputeTritVector;

bool check_vec(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

bool check_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

void test_backend_equivalence() {
  std::cout << "[Equivalence] Testing Backend Equivalence (Scalar vs SWAR vs SIMD)..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  // Test a mix of sizes, including small (<64B), medium, and large (>1KB)
  // Also odd sizes to check padding handling.
  std::vector<size_t> sizes = {1, 4, 15, 16, 17, 63, 64, 65, 255, 256, 257, 1024, 4097};

  for (size_t len : sizes) {
    std::vector<int8_t> a_trits(len), b_trits(len);
    for (size_t i = 0; i < len; ++i) {
      a_trits[i] = static_cast<int8_t>(dist(rng));
      b_trits[i] = static_cast<int8_t>(dist(rng));
    }

    auto va = ComputeTritVector::from_trits(a_trits).value();
    auto vb = ComputeTritVector::from_trits(b_trits).value();

    // Scalar trit oracle
    std::vector<int8_t> ref_and(len), ref_or(len), ref_not(len), ref_xor(len);
    for (size_t i = 0; i < len; ++i) {
      ref_and[i] = std::min(a_trits[i], b_trits[i]);
      ref_or[i] = std::max(a_trits[i], b_trits[i]);
      ref_not[i] = -a_trits[i];

      // TXor (Difference)
      int res = a_trits[i] - b_trits[i];
      if (res > 1) res = -1;
      if (res < -1) res = 1;
      ref_xor[i] = (int8_t)res;
    }

    // Scalar packed-byte oracle
    auto ref_and_cv = va.t_and_ref(vb).value();
    auto ref_or_cv = va.t_or_ref(vb).value();
    auto ref_not_cv = va.t_not_ref().value();
    auto ref_xor_cv = va.t_xor_ref(vb).value();

    assert(check_vec(ref_and_cv.to_trits().value(), ref_and));
    assert(check_vec(ref_or_cv.to_trits().value(), ref_or));
    assert(check_vec(ref_not_cv.to_trits().value(), ref_not));
    assert(check_vec(ref_xor_cv.to_trits().value(), ref_xor));

    // Best-backend dispatch
    auto res_and = va.t_and(vb).value();
    auto res_or = va.t_or(vb).value();
    auto res_not = va.t_not().value();
    auto res_xor = va.t_xor(vb).value();

    assert(check_vec(res_and.to_trits().value(), ref_and));
    assert(check_vec(res_or.to_trits().value(), ref_or));
    assert(check_vec(res_not.to_trits().value(), ref_not));
    assert(check_vec(res_xor.to_trits().value(), ref_xor));
    assert(check_bytes(res_and.data(), ref_and_cv.data()));
    assert(check_bytes(res_or.data(), ref_or_cv.data()));
    assert(check_bytes(res_not.data(), ref_not_cv.data()));
    assert(check_bytes(res_xor.data(), ref_xor_cv.data()));

    // Explicit backend validation against the scalar packed-byte oracle.
    size_t byte_len = va.data().size();
    std::vector<uint8_t> swar_not(byte_len), swar_and(byte_len), swar_or(byte_len);
    std::vector<uint8_t> simd_not(byte_len), simd_and(byte_len), simd_or(byte_len);

    const uint8_t* raw_a = va.data().data();
    const uint8_t* raw_b = vb.data().data();

    ComputeTritVector::kernel_not_swar(raw_a, swar_not.data(), byte_len);
    ComputeTritVector::kernel_and_swar(raw_a, raw_b, swar_and.data(), byte_len);
    ComputeTritVector::kernel_or_swar(raw_a, raw_b, swar_or.data(), byte_len);
    if (len % 4 != 0 && byte_len != 0) {
      const uint8_t mask = static_cast<uint8_t>((1u << ((len % 4) * 2)) - 1u);
      swar_not.back() &= mask;
      swar_and.back() &= mask;
      swar_or.back() &= mask;
    }

    assert(check_bytes(swar_not, ref_not_cv.data()));
    assert(check_bytes(swar_and, ref_and_cv.data()));
    assert(check_bytes(swar_or, ref_or_cv.data()));

#if defined(__x86_64__) && defined(__AVX2__)
    ComputeTritVector::kernel_not_avx2(raw_a, simd_not.data(), byte_len);
    ComputeTritVector::kernel_and_avx2(raw_a, raw_b, simd_and.data(), byte_len);
    ComputeTritVector::kernel_or_avx2(raw_a, raw_b, simd_or.data(), byte_len);
    if (len % 4 != 0 && byte_len != 0) {
      const uint8_t mask = static_cast<uint8_t>((1u << ((len % 4) * 2)) - 1u);
      simd_not.back() &= mask;
      simd_and.back() &= mask;
      simd_or.back() &= mask;
    }

    assert(check_bytes(simd_not, ref_not_cv.data()));
    assert(check_bytes(simd_and, ref_and_cv.data()));
    assert(check_bytes(simd_or, ref_or_cv.data()));
    assert(check_bytes(swar_not, simd_not));
    assert(check_bytes(swar_and, simd_and));
    assert(check_bytes(swar_or, simd_or));
#endif

#if defined(__aarch64__) && defined(__ARM_NEON)
    ComputeTritVector::kernel_not_neon(raw_a, simd_not.data(), byte_len);
    ComputeTritVector::kernel_and_neon(raw_a, raw_b, simd_and.data(), byte_len);
    ComputeTritVector::kernel_or_neon(raw_a, raw_b, simd_or.data(), byte_len);
    if (len % 4 != 0 && byte_len != 0) {
      const uint8_t mask = static_cast<uint8_t>((1u << ((len % 4) * 2)) - 1u);
      simd_not.back() &= mask;
      simd_and.back() &= mask;
      simd_or.back() &= mask;
    }

    assert(check_bytes(simd_not, ref_not_cv.data()));
    assert(check_bytes(simd_and, ref_and_cv.data()));
    assert(check_bytes(simd_or, ref_or_cv.data()));
    assert(check_bytes(swar_not, simd_not));
    assert(check_bytes(swar_and, simd_and));
    assert(check_bytes(swar_or, simd_or));
#endif
  }
}

void test_txor_fallback_routing() {
  std::cout << "[Compliance] Testing TXor Fallback Routing..." << std::endl;
  // Verify that tritwise_xor matches scalar truth table exactly.
  // We rely on the fact that t_xor calls t_xor_lut.

  // Truth table cases
  struct Case {
    int8_t a, b, expected;
  };
  std::vector<Case> cases = {{-1, -1, 0}, {-1, 0, -1}, {-1, 1, 1}, {0, -1, 1}, {0, 0, 0},
                             {0, 1, -1},  {1, -1, -1}, {1, 0, 1},  {1, 1, 0}};

  for (const auto& c : cases) {
    std::vector<int8_t> va = {c.a};
    std::vector<int8_t> vb = {c.b};

    auto ca = ComputeTritVector::from_trits(va).value();
    auto cb = ComputeTritVector::from_trits(vb).value();

    // In-place API
    assert(tritwise_xor(ca, cb).is_ok());

    auto res = ca.to_trits().value();
    assert(res[0] == c.expected);
  }
}

void test_public_tritwise_memory_semantics() {
  std::cout << "[Memory] Testing Public Tritwise In-Place Semantics..." << std::endl;

  auto original = ComputeTritVector::from_trits({-1, 0, 1, -1, 1, 0, -1}).value();
  auto expected_self = original.to_trits().value();

  {
    auto v = original;
    const auto* before = v.data().data();
    require(tritwise_and(v, v).is_ok());
    require(v.data().data() == before);
    require(check_vec(v.to_trits().value(), expected_self));
  }

  {
    auto v = original;
    const auto* before = v.data().data();
    require(tritwise_or(v, v).is_ok());
    require(v.data().data() == before);
    require(check_vec(v.to_trits().value(), expected_self));
  }

  {
    auto v = ComputeTritVector::from_trits({1, -1, 0}).value();
    auto other = ComputeTritVector::from_trits({0, 1, -1}).value();
    require(tritwise_or(v, other).is_ok());
    require((v.data()[0] & 0xC0) == 0);
  }

  {
    auto v = ComputeTritVector::from_trits({1, -1, 0}).value();
    auto before = v.to_trits().value();
    require(tritwise_not(v).is_ok());
    require(tritwise_not(v).is_ok());
    require(check_vec(v.to_trits().value(), before));
    require((v.data()[0] & 0xC0) == 0);
  }
}

int main() {
  test_backend_equivalence();
  test_txor_fallback_routing();
  test_public_tritwise_memory_semantics();
  std::cout << "All backend equivalence tests passed." << std::endl;
  return 0;
}
