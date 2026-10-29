#include <cassert>
#include <iostream>
#include <limits>
#include "statistics.hpp"
void test_classification_outcomes() {
  // Scenario 1: BLOCKED due to PMU unavailable
  {
    MeasureSeries s;
    s.pmu_available = false;
    assert(classify_pattern_result(s) == Outcome::BLOCKED);
  }

  // Scenario 2: SUPPORTED synthetic inputs (cand inst = 70% of base1 inst)
  {
    MeasureSeries s;
    s.candidate_inst = {70, 70, 70, 70, 70, 70, 70, 70, 70, 70};
    s.base1_inst = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
    s.candidate_miss = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
    s.base1_miss = {12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
    s.candidate_lat = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
    s.fastest_base_lat = {55, 55, 55, 55, 55, 55, 55, 55, 55, 55};
    assert(classify_pattern_result(s) == Outcome::SUPPORTED);
  }

  // Scenario 3: NOT SUPPORTED synthetic inputs (cand inst = 95% of base1 inst, reduction < 15%)
  {
    MeasureSeries s;
    s.candidate_inst = {95, 95, 95, 95, 95, 95, 95, 95, 95, 95};
    s.base1_inst = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
    s.candidate_miss = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
    s.base1_miss = {12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
    s.candidate_lat = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
    s.fastest_base_lat = {55, 55, 55, 55, 55, 55, 55, 55, 55, 55};
    assert(classify_pattern_result(s) == Outcome::NOT_SUPPORTED);
  }

  // Scenario 4: INCONCLUSIVE synthetic inputs (cand inst = 84% of base1 inst with higher variance)
  {
    MeasureSeries s;
    s.candidate_inst = {82, 86, 81, 87, 83, 85, 82, 86, 84, 84};
    s.base1_inst = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
    s.candidate_miss = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
    s.base1_miss = {12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
    s.candidate_lat = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
    s.fastest_base_lat = {55, 55, 55, 55, 55, 55, 55, 55, 55, 55};
    assert(classify_pattern_result(s) == Outcome::INCONCLUSIVE);
  }

  // Regression: empty and incomplete data must fail closed, never generate NaNs or OOB.
  {
    MeasureSeries s;
    assert(classify_pattern_result(s) == Outcome::BLOCKED);
    s.candidate_inst.assign(10, 70);
    s.base1_inst.assign(9, 100);
    assert(classify_pattern_result(s) == Outcome::BLOCKED);
  }
  // Regression from Codex: significant, consistent miss regression overrides instruction gain.
  {
    MeasureSeries s;
    s.candidate_inst.assign(10, 70);
    s.base1_inst.assign(10, 100);
    s.candidate_miss.assign(10, 11);
    s.base1_miss.assign(10, 10);
    s.candidate_lat.assign(10, 50);
    s.fastest_base_lat.assign(10, 55);
    assert(classify_pattern_result(s) == Outcome::NOT_SUPPORTED);
    s.candidate_miss[0] = std::numeric_limits<double>::quiet_NaN();
    assert(classify_pattern_result(s) == Outcome::BLOCKED);
  }

  std::cout << "ALL_SYNTHETIC_CLASSIFICATION_TESTS_PASSED" << std::endl;
}

void test_adversarial_statistics() {
  MeasureSeries good;
  good.candidate_inst.assign(10, 70);
  good.base1_inst.assign(10, 100);
  good.candidate_miss.assign(10, 10);
  good.base1_miss.assign(10, 10);
  good.candidate_lat.assign(10, 50);
  good.fastest_base_lat.assign(10, 55);
  const std::vector<double> z(10, 0), one(10, 1);
  assert(compute_paired_t_pvalue(z, z) == 1);
  assert(compute_paired_t_pvalue(one, z) == 0);
  assert(paired_interval99(one, z) == std::make_pair(1.0, 1.0));
  assert(paired_interval99(z, z) == std::make_pair(0.0, 0.0));
  const auto bounds = compute_bootstrap_reduction_bounds(good.candidate_inst, good.base1_inst);
  assert(std::abs(bounds.first - .30) < 1e-15 && bounds.first == bounds.second);
  auto s = good;
  s.candidate_inst.assign(10, 85);
  assert(classify_pattern_result(s) == Outcome::SUPPORTED);  // inclusive 15% boundary
  s.candidate_inst.assign(10, 85.0001);
  assert(classify_pattern_result(s) == Outcome::NOT_SUPPORTED);
  // Noise does not become a significant regression through an arbitrary mean allowance.
  s = good;
  s.candidate_miss.assign(10, 0);
  s.candidate_miss[0] = 1e9;
  assert(paired_interval99(s.candidate_miss, s.base1_miss).first < 0);
  assert(classify_pattern_result(s) == Outcome::INCONCLUSIVE);
  s = good;
  s.candidate_lat.assign(10, 56);
  assert(classify_pattern_result(s) == Outcome::NOT_SUPPORTED);
  // Every metric must have exactly ten finite, nonnegative observations.
  for (int field = 0; field < 6; ++field) {
    for (int defect = 0; defect < 7; ++defect) {
      s = good;
      std::vector<double>* fields[] = {&s.candidate_inst, &s.base1_inst,    &s.candidate_miss,
                                       &s.base1_miss,     &s.candidate_lat, &s.fastest_base_lat};
      auto& v = *fields[field];
      if (defect == 0) v.clear();
      if (defect == 1) v.pop_back();
      if (defect == 2) v.push_back(1);
      if (defect == 3) v[0] = std::numeric_limits<double>::quiet_NaN();
      if (defect == 4) v[0] = std::numeric_limits<double>::infinity();
      if (defect == 5) v[0] = -1;
      if (defect == 6) v[0] = std::numeric_limits<double>::max();
      assert(classify_pattern_result(s) == Outcome::BLOCKED);
    }
  }
  s = good;
  s.base1_inst[0] = 0;
  assert(classify_pattern_result(s) == Outcome::BLOCKED);
  for (int flag = 0; flag < 3; ++flag) {
    s = good;
    if (flag == 0) s.pmu_available = false;
    if (flag == 1) s.correctness_pass = false;
    if (flag == 2) s.budget_pass = false;
    assert(classify_pattern_result(s) == Outcome::BLOCKED);
  }
  for (int helper = 0; helper < 3; ++helper) {
    bool threw = false;
    try {
      if (helper == 0) (void)compute_bootstrap_reduction_bounds({}, one);
      if (helper == 1) (void)compute_paired_t_pvalue(one, {});
      if (helper == 2) (void)paired_interval99({}, {});
    } catch (const std::invalid_argument&) {
      threw = true;
    }
    assert(threw);
  }
  // df=9 two-tail quantile landmarks. Invert a series with known SE=1.
  std::vector<double> a(10), b(10, 10);
  const double sd = std::sqrt(10.0);
  const double scale = sd / std::sqrt(82.5 / 9.0);
  for (double t : {0.0, 2.2621571628540993, 3.2498355415921254, 4.7809125859335495}) {
    for (std::size_t i = 0; i < 10; ++i) a[i] = 10 + t + (static_cast<double>(i) - 4.5) * scale;
    const double expected = t == 0 ? 1 : t < 3 ? .05 : t < 4 ? .01 : .001;
    assert(std::abs(compute_paired_t_pvalue(a, b) - expected) < 1e-11);
    assert(std::abs(compute_paired_t_pvalue(a, b) - compute_paired_t_pvalue(b, a)) < 1e-14);
  }
  std::cout << "ADVERSARIAL_STATISTICS_PASS invalid_series=42 flags=3 helpers=3 quantiles=4\n";
}

int main() {
  test_adversarial_statistics();
  test_classification_outcomes();
  return 0;
}
