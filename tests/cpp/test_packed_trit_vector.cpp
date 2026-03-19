#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>
#include "t81/packed_trit_vector.hpp"

using namespace t81;

// Helper to check vector equality
bool check_vec(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Phase 1 Tests
void test_phase1_roundtrip() {
  std::cout << "[Phase 1] Testing Roundtrip..." << std::endl;
  std::vector<int8_t> trits = {-1, 0, 1, 1, 0, -1, 0, 1};
  auto vec_res = PackedTritVector::from_trits(trits);
  assert(!vec_res.is_err());
  auto vec = vec_res.value();
  assert(vec.size() == trits.size());

  auto unpacked_res = vec.to_trits();
  assert(!unpacked_res.is_err());
  auto unpacked = unpacked_res.value();
  assert(check_vec(unpacked, trits));
}

void test_scalar_logic_basis() {
  std::cout << "[Scalar] Testing Logic Basis..." << std::endl;
  // TNot
  assert(PackedTritVector::scalar_not(1) == -1);
  assert(PackedTritVector::scalar_not(0) == 0);
  assert(PackedTritVector::scalar_not(-1) == 1);

  // TAnd: min
  assert(PackedTritVector::scalar_and(-1, -1) == -1);
  assert(PackedTritVector::scalar_and(-1, 0) == -1);
  assert(PackedTritVector::scalar_and(-1, 1) == -1);
  assert(PackedTritVector::scalar_and(0, 0) == 0);
  assert(PackedTritVector::scalar_and(0, 1) == 0);
  assert(PackedTritVector::scalar_and(1, 1) == 1);

  // TOr: max
  assert(PackedTritVector::scalar_or(-1, -1) == -1);
  assert(PackedTritVector::scalar_or(-1, 0) == 0);
  assert(PackedTritVector::scalar_or(-1, 1) == 1);
  assert(PackedTritVector::scalar_or(0, 0) == 0);
  assert(PackedTritVector::scalar_or(0, 1) == 1);
  assert(PackedTritVector::scalar_or(1, 1) == 1);

  // TXor: a - b (wrapped) - Canonical TISC semantics
  assert(PackedTritVector::scalar_xor(-1, -1) == 0);
  assert(PackedTritVector::scalar_xor(-1, 0) == -1);
  assert(PackedTritVector::scalar_xor(-1, 1) == 1);  // -1 - 1 = -2 -> 1
  assert(PackedTritVector::scalar_xor(0, -1) == 1);  // 0 - -1 = 1
  assert(PackedTritVector::scalar_xor(0, 0) == 0);
  assert(PackedTritVector::scalar_xor(0, 1) == -1);
  assert(PackedTritVector::scalar_xor(1, -1) == -1);  // 1 - -1 = 2 -> -1
  assert(PackedTritVector::scalar_xor(1, 0) == 1);
  assert(PackedTritVector::scalar_xor(1, 1) == 0);
}

// Explicit TXor Truth Table Guard
void test_txor_truth_table() {
  std::cout << "[Guard] Testing TXor Truth Table (9 cases)..." << std::endl;
  struct Case {
    int8_t a;
    int8_t b;
    int8_t expected;
  };
  std::vector<Case> cases = {{-1, -1, 0}, {-1, 0, -1}, {-1, 1, 1}, {0, -1, 1}, {0, 0, 0},
                             {0, 1, -1},  {1, -1, -1}, {1, 0, 1},  {1, 1, 0}};

  for (const auto& c : cases) {
    // 1. Scalar check
    assert(PackedTritVector::scalar_xor(c.a, c.b) == c.expected);

    // 2. Phase 1 check
    auto p1_a = PackedTritVector::from_trits({c.a}).value();
    auto p1_b = PackedTritVector::from_trits({c.b}).value();
    auto p1_res = p1_a.t_xor(p1_b).value();
    assert(p1_res.to_trits().value()[0] == c.expected);

    // 3. Phase 2A check
    auto p2_a = ComputeTritVector::from_trits({c.a}).value();
    auto p2_b = ComputeTritVector::from_trits({c.b}).value();
    auto p2_res = p2_a.t_xor(p2_b).value();
    assert(p2_res.to_trits().value()[0] == c.expected);
  }
}

// Phase 2A Tests
void test_phase2a_roundtrip() {
  std::cout << "[Phase 2A] Testing Roundtrip..." << std::endl;
  std::vector<int8_t> trits = {-1, 0, 1, 1, 0, -1, 0, 1};
  auto vec_res = ComputeTritVector::from_trits(trits);
  assert(!vec_res.is_err());
  auto vec = vec_res.value();
  assert(vec.size() == trits.size());

  // Check data size (2 bits per trit -> 8 trits fits in 2 bytes)
  assert(vec.data().size() == 2);

  auto unpacked_res = vec.to_trits();
  assert(!unpacked_res.is_err());
  auto unpacked = unpacked_res.value();
  assert(check_vec(unpacked, trits));
}

void test_phase2a_conversion() {
  std::cout << "[Phase 2A] Testing Conversion from Phase 1..." << std::endl;
  std::vector<int8_t> trits = {-1, 0, 1};
  auto p1 = PackedTritVector::from_trits(trits).value();

  auto p2_res = ComputeTritVector::from_phase1(p1);
  assert(!p2_res.is_err());
  auto p2 = p2_res.value();

  assert(check_vec(p2.to_trits().value(), trits));
}

void test_phase2a_from_packed_validation() {
  std::cout << "[Phase 2A] Testing Packed-Byte Validation..." << std::endl;

  auto valid = ComputeTritVector::from_packed({0x13}, 4);
  assert(!valid.is_err());
  assert(check_vec(valid.value().to_trits().value(), std::vector<int8_t>({-1, 0, 1, 0})));

  auto invalid_pattern = ComputeTritVector::from_packed({0x02}, 1);
  assert(invalid_pattern.is_err());
  assert(invalid_pattern.error().code.value() == T81Symbol::intern("INVALID_PACKED_DATA").value());

  auto invalid_tail = ComputeTritVector::from_packed({0xD3}, 3);
  assert(invalid_tail.is_err());
  assert(invalid_tail.error().code.value() == T81Symbol::intern("INVALID_TAIL_PADDING").value());

  auto invalid_length = ComputeTritVector::from_packed({}, 1);
  assert(invalid_length.is_err());
  assert(invalid_length.error().code.value() == T81Symbol::intern("INVALID_PACKED_LENGTH").value());
}

void test_cross_representation_consistency() {
  std::cout << "[Cross-Rep] Testing Consistency..." << std::endl;
  std::vector<int8_t> t1 = {-1, 0, 1, 1, -1};
  std::vector<int8_t> t2 = {1, -1, 0, 1, -1};

  // Scalar Reference
  std::vector<int8_t> ref_not, ref_and, ref_or, ref_xor;
  for (size_t i = 0; i < t1.size(); ++i) {
    ref_not.push_back(PackedTritVector::scalar_not(t1[i]));
    ref_and.push_back(PackedTritVector::scalar_and(t1[i], t2[i]));
    ref_or.push_back(PackedTritVector::scalar_or(t1[i], t2[i]));
    ref_xor.push_back(PackedTritVector::scalar_xor(t1[i], t2[i]));
  }

  // Phase 1
  auto p1_v1 = PackedTritVector::from_trits(t1).value();
  auto p1_v2 = PackedTritVector::from_trits(t2).value();
  assert(check_vec(p1_v1.t_not().value().to_trits().value(), ref_not));
  assert(check_vec(p1_v1.t_and(p1_v2).value().to_trits().value(), ref_and));
  assert(check_vec(p1_v1.t_or(p1_v2).value().to_trits().value(), ref_or));
  assert(check_vec(p1_v1.t_xor(p1_v2).value().to_trits().value(), ref_xor));

  // Phase 2A
  auto p2_v1 = ComputeTritVector::from_trits(t1).value();
  auto p2_v2 = ComputeTritVector::from_trits(t2).value();
  assert(check_vec(p2_v1.t_not().value().to_trits().value(), ref_not));
  assert(check_vec(p2_v1.t_and(p2_v2).value().to_trits().value(), ref_and));
  assert(check_vec(p2_v1.t_or(p2_v2).value().to_trits().value(), ref_or));
  assert(check_vec(p2_v1.t_xor(p2_v2).value().to_trits().value(), ref_xor));
}

void test_randomized_determinism() {
  std::cout << "[Random] Testing Deterministic Randomized Vectors..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  for (int len : {0, 1, 2, 4, 5, 6, 16, 64, 100, 255, 256, 257, 1024, 4096}) {
    std::vector<int8_t> t1_data;
    std::vector<int8_t> t2_data;
    for (int i = 0; i < len; ++i) {
      t1_data.push_back(static_cast<int8_t>(dist(rng)));
      t2_data.push_back(static_cast<int8_t>(dist(rng)));
    }

    // Phase 1
    auto p1_v1 = PackedTritVector::from_trits(t1_data).value();
    auto p1_v2 = PackedTritVector::from_trits(t2_data).value();

    // Phase 2A
    auto p2_v1 = ComputeTritVector::from_trits(t1_data).value();
    auto p2_v2 = ComputeTritVector::from_trits(t2_data).value();

    // Check equivalence for all ops
    // Not
    assert(check_vec(p1_v1.t_not().value().to_trits().value(),
                     p2_v1.t_not().value().to_trits().value()));

    // And
    assert(check_vec(p1_v1.t_and(p1_v2).value().to_trits().value(),
                     p2_v1.t_and(p2_v2).value().to_trits().value()));

    // Or
    assert(check_vec(p1_v1.t_or(p1_v2).value().to_trits().value(),
                     p2_v1.t_or(p2_v2).value().to_trits().value()));

    // Xor
    assert(check_vec(p1_v1.t_xor(p1_v2).value().to_trits().value(),
                     p2_v1.t_xor(p2_v2).value().to_trits().value()));
  }
}

void test_errors() {
  std::cout << "[Errors] Testing Error Conditions..." << std::endl;

  // Invalid trit
  std::vector<int8_t> bad = {2};
  assert(PackedTritVector::from_trits(bad).is_err());
  assert(ComputeTritVector::from_trits(bad).is_err());

  // Length mismatch
  std::vector<int8_t> t1 = {0};
  std::vector<int8_t> t2 = {0, 0};

  auto p1_v1 = PackedTritVector::from_trits(t1).value();
  auto p1_v2 = PackedTritVector::from_trits(t2).value();
  assert(p1_v1.t_and(p1_v2).is_err());

  auto p2_v1 = ComputeTritVector::from_trits(t1).value();
  auto p2_v2 = ComputeTritVector::from_trits(t2).value();
  assert(p2_v1.t_and(p2_v2).is_err());
}

void test_phase2c_swar_equivalence() {
  std::cout << "[Phase 2C] Testing SWAR vs Reference Implementation..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  for (int len : {0, 1, 2, 4, 5, 7, 8, 16, 64, 100, 255, 256, 1024, 1027}) {
    std::vector<int8_t> t1_data;
    std::vector<int8_t> t2_data;
    for (int i = 0; i < len; ++i) {
      t1_data.push_back(static_cast<int8_t>(dist(rng)));
      t2_data.push_back(static_cast<int8_t>(dist(rng)));
    }

    auto v1 = ComputeTritVector::from_trits(t1_data).value();
    auto v2 = ComputeTritVector::from_trits(t2_data).value();

    // Not (SWAR)
    auto not_swar = v1.t_not().value();
    auto not_ref = v1.t_not_ref().value();
    assert(check_vec(not_swar.to_trits().value(), not_ref.to_trits().value()));
    assert(not_swar.data() == not_ref.data());  // Byte-wise identical

    // And (SWAR)
    auto and_swar = v1.t_and(v2).value();
    auto and_ref = v1.t_and_ref(v2).value();
    assert(check_vec(and_swar.to_trits().value(), and_ref.to_trits().value()));
    assert(and_swar.data() == and_ref.data());

    // Or (SWAR)
    auto or_swar = v1.t_or(v2).value();
    auto or_ref = v1.t_or_ref(v2).value();
    assert(check_vec(or_swar.to_trits().value(), or_ref.to_trits().value()));
    assert(or_swar.data() == or_ref.data());

    // Xor (Still LUT)
    auto xor_lut = v1.t_xor(v2).value();
    auto xor_ref = v1.t_xor_ref(v2).value();
    assert(check_vec(xor_lut.to_trits().value(), xor_ref.to_trits().value()));
    assert(xor_lut.data() == xor_ref.data());
  }
}

void test_phase2b_lut_explicit() {
  std::cout << "[Phase 2B] Testing LUT (explicit) vs Reference..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  for (int len : {0, 1, 2, 4, 16, 64}) {
    std::vector<int8_t> t1_data;
    std::vector<int8_t> t2_data;
    for (int i = 0; i < len; ++i) {
      t1_data.push_back(static_cast<int8_t>(dist(rng)));
      t2_data.push_back(static_cast<int8_t>(dist(rng)));
    }

    auto v1 = ComputeTritVector::from_trits(t1_data).value();
    auto v2 = ComputeTritVector::from_trits(t2_data).value();

    // Not
    auto not_lut = v1.t_not_lut().value();
    auto not_ref = v1.t_not_ref().value();
    assert(check_vec(not_lut.to_trits().value(), not_ref.to_trits().value()));

    // And
    auto and_lut = v1.t_and_lut(v2).value();
    auto and_ref = v1.t_and_ref(v2).value();
    assert(check_vec(and_lut.to_trits().value(), and_ref.to_trits().value()));

    // Or
    auto or_lut = v1.t_or_lut(v2).value();
    auto or_ref = v1.t_or_ref(v2).value();
    assert(check_vec(or_lut.to_trits().value(), or_ref.to_trits().value()));

    // Xor
    auto xor_lut = v1.t_xor_lut(v2).value();
    auto xor_ref = v1.t_xor_ref(v2).value();
    assert(check_vec(xor_lut.to_trits().value(), xor_ref.to_trits().value()));
  }
}

void test_trailing_byte_masking() {
  std::cout << "[Phase 2B] Testing Trailing Byte Masking..." << std::endl;
  // Length 1: 1 trit. Fits in 2 bits. Remaining 6 bits of byte should be 0.
  // Trit 0 -> 00.
  // If we do NOT(0) -> NOT(00) -> 00 (scalar_not(0)=0).
  // If we have padding bits non-zero, they should be masked.

  // Use length 1 with value 0.
  std::vector<int8_t> trits = {0};
  auto v = ComputeTritVector::from_trits(trits).value();

  // Apply t_not. scalar_not(0) is 0.
  // If LUT logic was naive: LUT[0] -> LUT[00 00 00 00] -> ?
  // 00 -> 0. scalar_not(0)=0 -> 00.
  // So result is 00 00 00 00.

  // Let's try value 1. NOT(1) = -1.
  // 1 -> 01. NOT -> 11.
  // Input: 01 00 00 00 (little endian in trit slots?)
  // Lane 0: 01. Lane 1: 00. ...
  // LUT should map Lane 0: 01->11. Lane 1: 00->00.
  // Result: 11 00 00 00.

  trits = {1};
  v = ComputeTritVector::from_trits(trits).value();
  auto v_not = v.t_not().value();

  // Check trits
  auto res_trits = v_not.to_trits().value();
  assert(res_trits[0] == -1);

  // Check raw data padding
  // Only 1 trit used (2 bits). Mask is 0x03.
  // Byte should be (11) & 0x03 = 0x03.
  // If high bits were set by LUT (unlikely for 0->0 map), they should be cleared.
  // Actually, for 0->0, it's fine.

  // What if we have invalid padding? We can't inject it easily.
  // But we can ensure that valid ops don't dirty the padding.
  [[maybe_unused]] uint8_t byte = v_not.data()[0];
  assert((byte & 0xFC) == 0);  // Top 6 bits must be 0
}

void test_inplace_apis() {
  std::cout << "[Phase 2D] Testing In-Place APIs..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  size_t len = 64;
  std::vector<int8_t> t1_data(len), t2_data(len);
  for (size_t i = 0; i < len; ++i) {
    t1_data[i] = static_cast<int8_t>(dist(rng));
    t2_data[i] = static_cast<int8_t>(dist(rng));
  }

  // Baseline
  auto v1 = ComputeTritVector::from_trits(t1_data).value();
  auto v2 = ComputeTritVector::from_trits(t2_data).value();
  auto v1_orig = v1.to_trits().value();

  // Test t_not_inplace
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();  // Copy
    assert(v.t_not_inplace().is_ok());
    auto expected = v1.t_not().value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
    // Double negation should return to original
    assert(v.t_not_inplace().is_ok());
    assert(check_vec(v.to_trits().value(), v1_orig));
  }

  // Test t_and_inplace
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();  // Copy
    assert(v.t_and_inplace(v2).is_ok());
    auto expected = v1.t_and(v2).value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
  }

  // Test t_or_inplace
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();  // Copy
    assert(v.t_or_inplace(v2).is_ok());
    auto expected = v1.t_or(v2).value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
  }

  // Test aliasing (src == dst)
  {
    // AND with self -> self
    auto v = ComputeTritVector::from_trits(t1_data).value();
    assert(v.t_and_inplace(v).is_ok());  // aliasing
    assert(check_vec(v.to_trits().value(), v1_orig));

    // OR with self -> self
    v = ComputeTritVector::from_trits(t1_data).value();
    assert(v.t_or_inplace(v).is_ok());
    assert(check_vec(v.to_trits().value(), v1_orig));
  }
}

void test_simd_vs_swar() {
  std::cout << "[Phase 2D/2E] Testing SIMD vs SWAR Differential..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  // Test various lengths to hit SIMD loops, tails, and thresholds
  // 256 trits (64 bytes) is the current threshold.
  // We test below, at, and above.
  for (size_t len : {16, 31, 32, 33, 64, 100, 255, 256, 257, 1024, 4096}) {
    std::vector<int8_t> t1_data(len), t2_data(len);
    for (size_t i = 0; i < len; ++i) {
      t1_data[i] = static_cast<int8_t>(dist(rng));
      t2_data[i] = static_cast<int8_t>(dist(rng));
    }

    auto v1 = ComputeTritVector::from_trits(t1_data).value();
    auto v2 = ComputeTritVector::from_trits(t2_data).value();

    // Not
    // v1.t_not() uses SIMD dispatch (if enabled/large enough), v1.t_not_swar() forces SWAR
    auto not_simd = v1.t_not().value();
    auto not_swar = t81::swar::t_not_swar(v1).value();
    assert(check_vec(not_simd.to_trits().value(), not_swar.to_trits().value()));
    assert(not_simd.data() == not_swar.data());

    // And
    auto and_simd = v1.t_and(v2).value();
    auto and_swar = t81::swar::t_and_swar(v1, v2).value();
    assert(check_vec(and_simd.to_trits().value(), and_swar.to_trits().value()));
    assert(and_simd.data() == and_swar.data());

    // Or
    auto or_simd = v1.t_or(v2).value();
    auto or_swar = t81::swar::t_or_swar(v1, v2).value();
    assert(check_vec(or_simd.to_trits().value(), or_swar.to_trits().value()));
    assert(or_simd.data() == or_swar.data());
  }
}

#if defined(__aarch64__) && defined(__ARM_NEON)
void test_neon_explicit() {
  std::cout << "[Phase 2E] Testing NEON Explicit Kernels..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  for (size_t len : {16, 64, 256, 1024}) {
    std::vector<int8_t> t1_data(len), t2_data(len);
    for (size_t i = 0; i < len; ++i) {
      t1_data[i] = static_cast<int8_t>(dist(rng));
      t2_data[i] = static_cast<int8_t>(dist(rng));
    }

    auto v1 = ComputeTritVector::from_trits(t1_data).value();
    auto v2 = ComputeTritVector::from_trits(t2_data).value();

    // We can call kernels directly if they are public.
    // Since ComputeTritVector has raw data access via data(), we can use that.

    // Prepare output buffers
    std::vector<uint8_t> out_not(v1.data().size());
    std::vector<uint8_t> out_and(v1.data().size());
    std::vector<uint8_t> out_or(v1.data().size());

    // Call NEON kernels
    ComputeTritVector::kernel_not_neon(v1.data().data(), out_not.data(), v1.data().size());
    ComputeTritVector::kernel_and_neon(v1.data().data(), v2.data().data(), out_and.data(),
                                       v1.data().size());
    ComputeTritVector::kernel_or_neon(v1.data().data(), v2.data().data(), out_or.data(),
                                      v1.data().size());

    // Compare with SWAR
    auto not_swar = t81::swar::t_not_swar(v1).value();
    auto and_swar = t81::swar::t_and_swar(v1, v2).value();
    auto or_swar = t81::swar::t_or_swar(v1, v2).value();

    assert(out_not == not_swar.data());
    assert(out_and == and_swar.data());
    assert(out_or == or_swar.data());
  }
}
#endif

int main() {
#if defined(__AVX2__)
  std::cout << "[Info] AVX2 Enabled" << std::endl;
#else
  std::cout << "[Info] AVX2 Disabled - Using Fallback" << std::endl;
#endif
  test_phase1_roundtrip();
  test_scalar_logic_basis();
  test_txor_truth_table();
  test_phase2a_roundtrip();
  test_phase2a_conversion();
  test_phase2a_from_packed_validation();
  test_cross_representation_consistency();
  test_randomized_determinism();
  test_errors();
  test_phase2c_swar_equivalence();
  test_phase2b_lut_explicit();
  test_trailing_byte_masking();
  test_inplace_apis();
  test_simd_vs_swar();
#if defined(__aarch64__) && defined(__ARM_NEON)
  test_neon_explicit();
#endif

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
