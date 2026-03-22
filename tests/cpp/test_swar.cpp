#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

#include "t81/packed_trit_vector.hpp"
#include "t81/swar/swar.hpp"

using namespace t81::swar;
using ComputeTritVector = t81::ComputeTritVector;

// Helper to check vector equality
bool check_vec(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
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

void test_swar_basics() {
  std::cout << "[SWAR] Testing Basic Logic..." << std::endl;
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

    // Not (SWAR explicit) vs Not (Reference)
    auto not_swar = t_not_swar(v1).value();
    auto not_ref = v1.t_not_ref().value();
    assert(check_vec(not_swar.to_trits().value(), not_ref.to_trits().value()));
    assert(not_swar.data() == not_ref.data());  // Byte-wise identical

    // And (SWAR explicit) vs And (Reference)
    auto and_swar = t_and_swar(v1, v2).value();
    auto and_ref = v1.t_and_ref(v2).value();
    assert(check_vec(and_swar.to_trits().value(), and_ref.to_trits().value()));
    assert(and_swar.data() == and_ref.data());

    // Or (SWAR explicit) vs Or (Reference)
    auto or_swar = t_or_swar(v1, v2).value();
    auto or_ref = v1.t_or_ref(v2).value();
    assert(check_vec(or_swar.to_trits().value(), or_ref.to_trits().value()));
    assert(or_swar.data() == or_ref.data());
  }
}

void test_swar_inplace() {
  std::cout << "[SWAR] Testing In-Place APIs..." << std::endl;
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
    assert(t_not_inplace(v).is_ok());
    auto expected = t_not_swar(v1).value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
    // Double negation should return to original
    assert(t_not_inplace(v).is_ok());
    assert(check_vec(v.to_trits().value(), v1_orig));
  }

  // Test t_and_inplace
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();  // Copy
    assert(t_and_inplace(v, v2).is_ok());
    auto expected = t_and_swar(v1, v2).value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
  }

  // Test t_or_inplace
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();  // Copy
    assert(t_or_inplace(v, v2).is_ok());
    auto expected = t_or_swar(v1, v2).value();
    assert(check_vec(v.to_trits().value(), expected.to_trits().value()));
  }

  // Public SWAR aliasing contract: src == dst is deterministic and preserves
  // canonical bytes for idempotent self-composition.
  {
    auto v = ComputeTritVector::from_trits(t1_data).value();
    const auto* before = v.data().data();
    require(t_and_inplace(v, v).is_ok());
    require(v.data().data() == before);
    require(check_vec(v.to_trits().value(), v1_orig));

    v = ComputeTritVector::from_trits(t1_data).value();
    before = v.data().data();
    require(t_or_inplace(v, v).is_ok());
    require(v.data().data() == before);
    require(check_vec(v.to_trits().value(), v1_orig));
  }

  // Tail padding must remain canonical after in-place mutation on partial-byte
  // vectors. For 3 trits only the low 6 bits may be used.
  {
    auto v = ComputeTritVector::from_trits({1, -1, 0}).value();
    auto other = ComputeTritVector::from_trits({0, 1, -1}).value();
    require(t_or_inplace(v, other).is_ok());
    require((v.data()[0] & 0xC0) == 0);
  }
}

void test_swar_public_api() {
  std::cout << "[SWAR] Testing Public Dispatch API..." << std::endl;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-1, 1);

  size_t len = 64;
  std::vector<int8_t> t1_data(len), t2_data(len);
  for (size_t i = 0; i < len; ++i) {
    t1_data[i] = static_cast<int8_t>(dist(rng));
    t2_data[i] = static_cast<int8_t>(dist(rng));
  }

  auto v1 = ComputeTritVector::from_trits(t1_data).value();
  auto v2 = ComputeTritVector::from_trits(t2_data).value();

  auto not_pub = t_not(v1).value();
  auto not_ref = v1.t_not_ref().value();
  assert(check_vec(not_pub.to_trits().value(), not_ref.to_trits().value()));

  auto and_pub = t_and(v1, v2).value();
  auto and_ref = v1.t_and_ref(v2).value();
  assert(check_vec(and_pub.to_trits().value(), and_ref.to_trits().value()));

  auto or_pub = t_or(v1, v2).value();
  auto or_ref = v1.t_or_ref(v2).value();
  assert(check_vec(or_pub.to_trits().value(), or_ref.to_trits().value()));
}

// RFC-0044 §4 (Validation Contract): invalid packed-trit bit patterns must be
// rejected at the validated surface boundary with a deterministic error result.
// The forbidden pattern is 10 (decimal 2) — encodes no valid trit value.
void test_rfc0044_invalid_pattern_rejection() {
  std::cout << "[RFC-0044] Invalid packed-trit pattern rejection..." << std::endl;

  // Build a raw byte containing the invalid 10 pattern in the lowest two bits.
  // Byte layout (2 bits per trit, LSB first): trits 0..3 occupy bits [1:0], [3:2], [5:4], [7:6].
  // Pattern 0x02 = 0b00000010 → trit 0 = 10 (invalid), trit 1 = 00 (Z), trit 2 = 00 (Z), trit 3 = 00 (Z).
  {
    std::vector<uint8_t> bad_bytes = {0x02};  // trit 0 = 10 (invalid)
    auto result = ComputeTritVector::from_packed(bad_bytes, 4);
    require(result.is_err(), "from_packed with 10-pattern in trit 0 must fail");
  }

  // Invalid pattern in a higher trit (bits [3:2] = 10 → 0b00001000 = 0x08).
  {
    std::vector<uint8_t> bad_bytes = {0x08};  // trit 1 = 10 (invalid)
    auto result = ComputeTritVector::from_packed(bad_bytes, 4);
    require(result.is_err(), "from_packed with 10-pattern in trit 1 must fail");
  }

  // Invalid tail padding: trit count = 3 means bits [7:6] of the byte must be
  // zero.  A non-zero value in unused bits must be rejected.
  {
    std::vector<uint8_t> bad_tail = {0x40};  // bits [7:6] = 01 — nonzero tail
    auto result = ComputeTritVector::from_packed(bad_tail, 3);
    require(result.is_err(), "from_packed with nonzero tail padding must fail");
  }

  // Sanity: valid bytes must succeed.
  {
    // 0b10100101 = 0xA5: trits [P, N, P, N] = [01, 11, 01, 11] LSB-first
    // trit 0 bits [1:0]=01=P, trit 1 bits [3:2]=01=P, trit 2 bits [5:4]=10=invalid!
    // Use a known-good byte: 0b11010101 = 0xD5 = trits [P, N, P, N]
    // Actually, let's just build from trits to ensure validity.
    auto valid = ComputeTritVector::from_trits({1, -1, 0, 1});
    require(valid.is_ok(), "from_trits with valid data must succeed");

    // And round-trip through from_packed must also succeed.
    auto repacked = ComputeTritVector::from_packed(valid.value().data(), 4);
    require(repacked.is_ok(), "from_packed with valid bytes (4 trits) must succeed");
    require(repacked.value().to_trits().value() == valid.value().to_trits().value(),
            "from_packed round-trip must preserve trit values");
  }

  // to_trits() on a valid vector must never produce invalid patterns.
  {
    auto v = ComputeTritVector::from_trits({1, 0, -1, 1, -1, 0}).value();
    auto trits = v.to_trits();
    require(trits.is_ok(), "to_trits on valid ComputeTritVector must succeed");
    for (int8_t t : trits.value()) {
      require(t == -1 || t == 0 || t == 1,
              "to_trits must produce only canonical trit values {-1, 0, +1}");
    }
  }

  std::cout << "  PASS\n";
}

int main() {
  test_swar_basics();
  test_swar_inplace();
  test_swar_public_api();
  test_rfc0044_invalid_pattern_rejection();

  std::cout << "All SWAR tests passed!" << std::endl;
  return 0;
}
