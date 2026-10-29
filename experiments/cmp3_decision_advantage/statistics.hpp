#pragma once
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>
#include "pcg64.hpp"

inline void validate_pair(const std::vector<double>& a, const std::vector<double>& b) {
  if (a.size() != 10 || b.size() != 10)
    throw std::invalid_argument("TEN_MATCHED_OBSERVATIONS_REQUIRED");
  for (const auto* v : {&a, &b})
    for (double x : *v)
      if (!std::isfinite(x) || x < 0 || x > static_cast<double>(UINT64_MAX))
        throw std::invalid_argument("INVALID_OBSERVATION");
}
struct Summary {
  double mean;
  double sd;
};
inline Summary paired_summary(const std::vector<double>& a, const std::vector<double>& b) {
  validate_pair(a, b);
  long double mean = 0, ss = 0;
  for (std::size_t i = 0; i < 10; ++i) mean += static_cast<long double>(a[i]) - b[i];
  mean /= 10;
  for (std::size_t i = 0; i < 10; ++i) {
    const long double d = static_cast<long double>(a[i]) - b[i] - mean;
    ss += d * d;
  }
  return {static_cast<double>(mean), static_cast<double>(std::sqrt(ss / 9))};
}
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
std::pair<double, double> compute_bootstrap_reduction_bounds(const std::vector<double>& cand,
                                                             const std::vector<double>& base1) {
  PCG64 rng;
  std::size_t n = cand.size();
  validate_pair(cand, base1);
  for (double x : base1)
    if (x <= 0) throw std::invalid_argument("ZERO_BASELINE_INSTRUCTIONS");

  constexpr int kResamples = 10000;
  std::vector<double> bootstrap_d(kResamples);

  for (std::size_t b = 0; b < kResamples; ++b) {
    long double sum_c = 0.0L;
    long double sum_b = 0.0L;
    for (std::size_t i = 0; i < n; ++i) {
      std::size_t idx = static_cast<std::size_t>(rng.bounded(n));
      sum_c += cand[idx];
      sum_b += base1[idx];
    }
    bootstrap_d[b] = static_cast<double>(1.0L - (sum_c / sum_b));
  }

  std::sort(bootstrap_d.begin(), bootstrap_d.end());
  // Inverse empirical CDF: ceil(q*10000)-1. One-sided 99% bounds at q=.01/.99.

  return {bootstrap_d[99], bootstrap_d[9899]};
}

// Paired Student-t test p-value calculation for 9 degrees of freedom (n=10)
double compute_paired_t_pvalue(const std::vector<double>& cand, const std::vector<double>& base1) {
  validate_pair(cand, base1);
  const auto summary = paired_summary(cand, base1);
  if (summary.sd == 0) return summary.mean == 0 ? 1.0 : 0.0;
  const double t = std::abs(summary.mean) / (summary.sd / std::sqrt(10.0));
  // For df=9, substituting t=3*cot(theta) gives the two-tail
  // probability integral sin(theta)^8 / integral_0^(pi/2) sin(theta)^8.
  // Composite Simpson integration (4096 panels); analytic normalizer 35*pi/256.
  const double theta = std::atan2(3.0, t);
  constexpr int panels = 4096;
  const double h = theta / panels;
  double sum = std::pow(std::sin(theta), 8);
  for (int i = 1; i < panels; ++i) sum += (i % 2 == 0 ? 2.0 : 4.0) * std::pow(std::sin(i * h), 8);
  return std::clamp(sum * h / 3.0 / (35.0 * std::acos(-1.0) / 256.0), 0.0, 1.0);
}

// Paired two-sided 99% Student-t interval for n=10 (df=9, t_0.995=3.2498355415921254).
std::pair<double, double> paired_interval99(const std::vector<double>& a,
                                            const std::vector<double>& b) {
  validate_pair(a, b);
  const auto summary = paired_summary(a, b);
  const double margin = 3.2498355415921254 * summary.sd / std::sqrt(10.0);
  return {summary.mean - margin, summary.mean + margin};
}

bool valid_series(const MeasureSeries& s) {
  const std::vector<double>* fields[] = {&s.candidate_inst, &s.base1_inst,    &s.candidate_miss,
                                         &s.base1_miss,     &s.candidate_lat, &s.fastest_base_lat};
  for (auto* f : fields) {
    if (f->size() != 10) return false;
    for (double x : *f)
      if (!std::isfinite(x) || x < 0 || x > static_cast<double>(UINT64_MAX)) return false;
  }
  for (const auto* f : {&s.candidate_inst, &s.base1_inst, &s.candidate_lat, &s.fastest_base_lat})
    for (double x : *f)
      if (x <= 0) return false;
  return true;
}

Outcome classify_pattern_result(const MeasureSeries& series) {
  if (!series.pmu_available || !series.correctness_pass || !series.budget_pass ||
      !valid_series(series)) {
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
  if (bound_high_99 < 0.15 ||
      paired_interval99(series.candidate_miss, series.base1_miss).first > 0.0 ||
      paired_interval99(series.candidate_lat, series.fastest_base_lat).first > 0.0) {
    return Outcome::NOT_SUPPORTED;
  }

  // 3. INCONCLUSIVE Rule:
  // Every other complete valid result
  return Outcome::INCONCLUSIVE;
}
