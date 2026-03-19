#include <cassert>
#include <cstdlib>
#include <vector>

#include "t81/simd/simd.hpp"

namespace {

bool check_vec(const std::vector<int8_t>& a, const std::vector<int8_t>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

void test_public_api_aliases() {
  using t81::simd::ComputeTritVector;

  auto a = ComputeTritVector::from_trits({-1, 0, 1, -1, 1, 0, -1, 1}).value();
  auto b = ComputeTritVector::from_trits({1, -1, 0, -1, 0, 1, 1, -1}).value();

  auto not_res = t81::simd::t_not(a).value();
  auto neg_res = t81::simd::t_neg(a).value();
  auto and_res = t81::simd::t_and(a, b).value();
  auto min_res = t81::simd::t_min(a, b).value();
  auto or_res = t81::simd::t_or(a, b).value();
  auto max_res = t81::simd::t_max(a, b).value();

  require(check_vec(not_res.to_trits().value(), neg_res.to_trits().value()));
  require(check_vec(and_res.to_trits().value(), min_res.to_trits().value()));
  require(check_vec(or_res.to_trits().value(), max_res.to_trits().value()));
}

void test_inplace_and_kernel_surface() {
  using t81::simd::ComputeTritVector;

  auto a = ComputeTritVector::from_trits({-1, 0, 1, -1, 1, 0, -1, 1}).value();
  auto b = ComputeTritVector::from_trits({1, -1, 0, -1, 0, 1, 1, -1}).value();
  auto expected = t81::simd::t_and(a, b).value();

  auto inplace = a;
  require(t81::simd::t_and_inplace(inplace, b).is_ok());
  require(check_vec(inplace.to_trits().value(), expected.to_trits().value()));

  std::vector<uint8_t> kernel_out(a.data().size());
  t81::simd::kernel::t_and(a.data().data(), b.data().data(), kernel_out.data(), a.data().size());
  require(kernel_out == expected.data());
}

void test_capability_introspection() {
  require(t81::simd::avx2_threshold_bytes() == 64);
  require(t81::simd::neon_threshold_bytes() == 64);

#if defined(__x86_64__) && defined(__AVX2__)
  require(t81::simd::is_avx2_available());
  require(t81::simd::t_not_threshold_bytes() == 64);
  require(t81::simd::t_and_threshold_bytes() == 64);
  require(t81::simd::t_or_threshold_bytes() == 64);
  require(t81::simd::get_optimal_threshold() == 64);
#elif defined(__aarch64__) && defined(__ARM_NEON)
  require(t81::simd::is_neon_available());
  require(t81::simd::t_not_threshold_bytes() > 65536);
  require(t81::simd::t_and_threshold_bytes() > 65536);
  require(t81::simd::t_or_threshold_bytes() == 64);
  require(t81::simd::get_optimal_threshold() == 64);
#else
  require(!t81::simd::is_avx2_available());
  require(!t81::simd::is_neon_available());
  require(t81::simd::t_not_threshold_bytes() == 0);
  require(t81::simd::t_and_threshold_bytes() == 0);
  require(t81::simd::t_or_threshold_bytes() == 0);
  require(t81::simd::get_optimal_threshold() == 0);
#endif
}

}  // namespace

int main() {
  test_public_api_aliases();
  test_inplace_and_kernel_surface();
  test_capability_introspection();
  return 0;
}
