// Synthetic disposition test suite for EXP-T81-COMP3-01 statistical decision engine.
// Verifies that outcome classification rules (SUPPORTED, NOT SUPPORTED, INCONCLUSIVE, BLOCKED)
// are mutually exclusive and exhaustive.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

enum class Outcome { BLOCKED, SUPPORTED, NOT_SUPPORTED, INCONCLUSIVE };

const char* outcome_to_string(Outcome o) {
  switch (o) {
    case Outcome::BLOCKED:
      return "BLOCKED";
    case Outcome::SUPPORTED:
      return "SUPPORTED";
    case Outcome::NOT_SUPPORTED:
      return "NOT SUPPORTED";
    case Outcome::INCONCLUSIVE:
      return "INCONCLUSIVE";
  }
  return "UNKNOWN";
}

struct MeasureSeries {
  std::vector<double> candidate_inst;
  std::vector<double> base1_inst;
  std::vector<double> candidate_miss;
  std::vector<double> base1_miss;
  std::vector<double> candidate_lat;
  std::vector<double> fastest_base_lat;
  bool pmu_available{true};
  bool correctness_pass{true};
  bool budget_pass{true};
};

// Computes 99% bootstrap lower/upper bound for instruction reduction d = 1 - mean(cand)/mean(base1)
std::pair<double, double> compute_bootstrap_reduction_bounds(
    const std::vector<double>& cand, const std::vector<double>& base1,
    uint64_t seed = 0x5438315F434D5033ULL) {
  std::mt19937_64 rng(seed);
  std::size_t n = cand.size();
  std::uniform_int_distribution<std::size_t> dist(0, n - 1);

  constexpr int kResamples = 10000;
  std::vector<double> bootstrap_d(kResamples);

  for (int b = 0; b < kResamples; ++b) {
    double sum_c = 0.0;
    double sum_b = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      std::size_t idx = dist(rng);
      sum_c += cand[idx];
      sum_b += base1[idx];
    }
    bootstrap_d[b] = 1.0 - (sum_c / sum_b);
  }

  std::sort(bootstrap_d.begin(), bootstrap_d.end());
  // 0.5% percentile (index 50) for lower bound, 99.5% percentile (index 9950) for upper bound
  return {bootstrap_d[50], bootstrap_d[9950]};
}

// Paired Student-t test p-value calculation for 9 degrees of freedom (n=10)
double compute_paired_t_pvalue(const std::vector<double>& cand, const std::vector<double>& base1) {
  std::size_t n = cand.size();
  std::vector<double> diff(n);
  double sum_d = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    diff[i] = cand[i] - base1[i];
    sum_d += diff[i];
  }
  double mean_d = sum_d / static_cast<double>(n);
  double sq_sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    sq_sum += (diff[i] - mean_d) * (diff[i] - mean_d);
  }
  double std_d = std::sqrt(sq_sum / static_cast<double>(n - 1));
  double t_stat = mean_d / (std_d / std::sqrt(static_cast<double>(n)));

  // Critical t for df=9 at p=0.01 (two-tailed) is ~3.250
  if (std::abs(t_stat) > 3.250) return 0.005;  // p < 0.01
  return 0.05;                                 // p >= 0.01
}

// Paired two-sided 99% Student-t interval for n=10 (df=9, t_0.995=3.249836).
std::pair<double,double> paired_interval99(const std::vector<double>& a,const std::vector<double>& b) {
  const std::size_t n=a.size();
  if(n!=10 || b.size()!=n) throw std::invalid_argument("requires ten matched observations");
  double mean=0;
  for(std::size_t i=0;i<n;i++) mean+=a[i]-b[i];
  mean/=static_cast<double>(n);
  double ss=0;
  for(std::size_t i=0;i<n;i++){double d=a[i]-b[i]-mean;ss+=d*d;}
  const double margin=3.249836256*std::sqrt(ss/(n-1)/n);
  return {mean-margin,mean+margin};
}

bool valid_series(const MeasureSeries& s) {
  const std::vector<double>* fields[]={&s.candidate_inst,&s.base1_inst,&s.candidate_miss,
     &s.base1_miss,&s.candidate_lat,&s.fastest_base_lat};
  for(auto* f:fields) {
    if(f->size()!=10) return false;
    for(double x:*f) if(!std::isfinite(x)||x<0) return false;
  }
  for(double x:s.base1_inst) if(x<=0) return false;
  return true;
}

Outcome classify_pattern_result(const MeasureSeries& series) {
  if (!series.pmu_available || !series.correctness_pass || !series.budget_pass || !valid_series(series)) {
    return Outcome::BLOCKED;
  }

  double mean_cand_miss =
      std::accumulate(series.candidate_miss.begin(), series.candidate_miss.end(), 0.0) /
      static_cast<double>(series.candidate_miss.size());
  double mean_base1_miss =
      std::accumulate(series.base1_miss.begin(), series.base1_miss.end(), 0.0) /
      static_cast<double>(series.base1_miss.size());

  double mean_cand_lat =
      std::accumulate(series.candidate_lat.begin(), series.candidate_lat.end(), 0.0) /
      static_cast<double>(series.candidate_lat.size());
  double mean_fastest_lat =
      std::accumulate(series.fastest_base_lat.begin(), series.fastest_base_lat.end(), 0.0) /
      static_cast<double>(series.fastest_base_lat.size());

  auto [bound_low_99, bound_high_99] =
      compute_bootstrap_reduction_bounds(series.candidate_inst, series.base1_inst);
  double p_val = compute_paired_t_pvalue(series.candidate_inst, series.base1_inst);

  // 1. SUPPORTED Rule:
  // p < 0.01 AND 99% lower bound for instruction reduction >= 0.15 AND mean_miss <= base1 AND
  // mean_lat <= fastest_base
  if (p_val < 0.01 && bound_low_99 >= 0.15 && mean_cand_miss <= mean_base1_miss &&
      mean_cand_lat <= mean_fastest_lat) {
    return Outcome::SUPPORTED;
  }

  // 2. NOT SUPPORTED Rule:
  // 99% upper bound for reduction < 0.15 OR mispredictions strictly higher OR latency strictly
  // higher
  if (bound_high_99 < 0.15 || paired_interval99(series.candidate_miss,series.base1_miss).first > 0.0 ||
      paired_interval99(series.candidate_lat,series.fastest_base_lat).first > 0.0) {
    return Outcome::NOT_SUPPORTED;
  }

  // 3. INCONCLUSIVE Rule:
  // Every other complete valid result
  return Outcome::INCONCLUSIVE;
}

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
    assert(classify_pattern_result(s)==Outcome::BLOCKED);
    s.candidate_inst.assign(10,70);s.base1_inst.assign(9,100);
    assert(classify_pattern_result(s)==Outcome::BLOCKED);
  }
  // Regression from Codex: significant, consistent miss regression overrides instruction gain.
  {
    MeasureSeries s;
    s.candidate_inst.assign(10,70);s.base1_inst.assign(10,100);
    s.candidate_miss.assign(10,11);s.base1_miss.assign(10,10);
    s.candidate_lat.assign(10,50);s.fastest_base_lat.assign(10,55);
    assert(classify_pattern_result(s)==Outcome::NOT_SUPPORTED);
    s.candidate_miss[0]=std::numeric_limits<double>::quiet_NaN();
    assert(classify_pattern_result(s)==Outcome::BLOCKED);
  }

  std::cout << "ALL_SYNTHETIC_CLASSIFICATION_TESTS_PASSED" << std::endl;
}

int main() {
  test_classification_outcomes();
  return 0;
}
