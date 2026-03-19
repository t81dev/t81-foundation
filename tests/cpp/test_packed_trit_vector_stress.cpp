#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>
#include "t81/packed_trit_vector.hpp"

using namespace t81;

bool check_vec(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void test_stress_chained_ops() {
  std::cout << "[Phase 2] Stress Testing Chained Operations & Padding..." << std::endl;
  std::vector<int> sizes = {1, 2, 3, 5, 7, 15, 17, 31, 33, 63, 65, 127, 129, 255, 257, 1027};

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(-1, 1);

  for (int size : sizes) {
    // Generate a, b, c, d
    std::vector<int8_t> va(size), vb(size), vc(size), vd(size);
    for (int i = 0; i < size; ++i) {
      va[i] = dist(rng);
      vb[i] = dist(rng);
      vc[i] = dist(rng);
      vd[i] = dist(rng);
    }

    auto pa = ComputeTritVector::from_trits(va).value();
    auto pb = ComputeTritVector::from_trits(vb).value();
    auto pc = ComputeTritVector::from_trits(vc).value();
    auto pd = ComputeTritVector::from_trits(vd).value();

    // Chain 1: ((a AND b) OR c) AND d
    // Scalar Ref
    std::vector<int8_t> ref1(size);
    for (int i = 0; i < size; ++i) {
      int8_t tmp = PackedTritVector::scalar_and(va[i], vb[i]);
      tmp = PackedTritVector::scalar_or(tmp, vc[i]);
      ref1[i] = PackedTritVector::scalar_and(tmp, vd[i]);
    }

    // SWAR
    auto res1 = pa.t_and(pb).value().t_or(pc).value().t_and(pd).value();

    if (!check_vec(res1.to_trits().value(), ref1)) {
      std::cerr << "FAIL: Chain 1 mismatch at size " << size << std::endl;
      std::exit(1);
    }

    // Check Padding Integrity
    // The last byte of res1 should have unused bits masked to 0.
    // number of bytes = (size + 3) / 4
    // trits in last byte = size % 4 (if 0, then 4 trits, full byte).
    size_t trits_in_last = size % 4;
    if (trits_in_last != 0) {
      uint8_t last_byte = res1.data().back();
      // used bits = trits_in_last * 2.
      uint8_t mask = (1 << (trits_in_last * 2)) - 1;
      if ((last_byte & ~mask) != 0) {
        std::cerr << "FAIL: Padding corruption at size " << size << " LastByte=" << std::hex
                  << (int)last_byte << " Mask=" << (int)mask << std::dec << std::endl;
        std::exit(1);
      }
    }

    // Chain 2: NOT(a AND (b OR c))
    // Scalar Ref
    std::vector<int8_t> ref2(size);
    for (int i = 0; i < size; ++i) {
      int8_t tmp = PackedTritVector::scalar_or(vb[i], vc[i]);
      tmp = PackedTritVector::scalar_and(va[i], tmp);
      ref2[i] = PackedTritVector::scalar_not(tmp);
    }

    // SWAR
    auto res2 = pa.t_and(pb.t_or(pc).value()).value().t_not().value();

    if (!check_vec(res2.to_trits().value(), ref2)) {
      std::cerr << "FAIL: Chain 2 mismatch at size " << size << std::endl;
      std::exit(1);
    }

    // Check Padding Integrity
    if (trits_in_last != 0) {
      uint8_t last_byte = res2.data().back();
      uint8_t mask = (1 << (trits_in_last * 2)) - 1;
      if ((last_byte & ~mask) != 0) {
        std::cerr << "FAIL: Padding corruption (Chain 2) at size " << size << std::endl;
        std::exit(1);
      }
    }
  }

  std::cout << "PASS: All stress tests passed for " << sizes.size() << " sizes." << std::endl;
}

int main() {
  test_stress_chained_ops();
  return 0;
}
