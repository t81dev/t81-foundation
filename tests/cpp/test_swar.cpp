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

int main() {
  test_swar_basics();
  test_swar_inplace();
  test_swar_public_api();

  std::cout << "All SWAR tests passed!" << std::endl;
  return 0;
}
