// Analyze only complete, matched, protocol-sized CSV observations.
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "statistics.hpp"

struct Metrics {
  std::vector<double> inst, miss, lat;
};
using Pattern = std::array<Metrics, 5>;
static double mean(const std::vector<double>& x) {
  return std::accumulate(x.begin(), x.end(), 0.0) / 10;
}
static void report(const std::string& label, const std::vector<double>& x,
                   const std::vector<double>& reference) {
  const auto s = paired_summary(x, reference);
  const double m = 2.2621571628540993 * s.sd / std::sqrt(10.0);
  std::cout << label << " mean=" << s.mean << " sample_sd=" << s.sd << " ci95=[" << s.mean - m
            << ',' << s.mean + m << "] raw=";
  for (std::size_t i = 0; i < 10; ++i) std::cout << x[i] - reference[i] << ',';
  std::cout << '\n';
}
int main(int argc, char** argv) try {
  if (argc != 2) throw std::invalid_argument("CSV_PATH_REQUIRED");
  std::ifstream input(argv[1]);
  if (!input) throw std::invalid_argument("MISSING_OBSERVATIONS");
  std::string line;
  if (!std::getline(input, line) ||
      line != "pattern,mode,repetition,instructions,branch_misses,elapsed_ns,checksum")
    throw std::invalid_argument("INVALID_CSV_HEADER");
  std::array<Pattern, 3> data;
  std::array<std::array<std::array<bool, 10>, 5>, 3> seen{};
  std::array<std::uint64_t, 3> checksums{};
  std::array<bool, 3> checksum_seen{};
  while (std::getline(input, line)) {
    std::replace(line.begin(), line.end(), ',', ' ');
    std::istringstream row(line);
    std::size_t p, mode, repetition;
    double inst, miss, lat;
    std::uint64_t checksum;
    std::string extra;
    if (!(row >> p >> mode >> repetition >> inst >> miss >> lat >> checksum) || row >> extra ||
        p >= 3 || mode >= 5 || repetition >= 10 || seen[p][mode][repetition])
      throw std::invalid_argument("INVALID_OR_DUPLICATE_OBSERVATION");
    if (checksum_seen[p] && checksum != checksums[p]) throw std::invalid_argument("PAYLOAD_PARITY");
    checksum_seen[p] = true;
    checksums[p] = checksum;
    auto& m = data[p][mode];
    if (m.inst.empty()) {
      m.inst.resize(10);
      m.miss.resize(10);
      m.lat.resize(10);
    }
    m.inst[repetition] = inst;
    m.miss[repetition] = miss;
    m.lat[repetition] = lat;
    seen[p][mode][repetition] = true;
  }
  for (std::size_t p = 0; p < 3; ++p)
    for (std::size_t m = 0; m < 5; ++m) {
      for (bool present : seen[p][m])
        if (!present) throw std::invalid_argument("MISSING_OBSERVATIONS");
      for (const auto* x : {&data[p][m].inst, &data[p][m].miss, &data[p][m].lat})
        validate_pair(*x, *x);
      for (const auto* v : {&data[p][m].inst, &data[p][m].lat})
        for (double x : *v)
          if (x <= 0) throw std::invalid_argument("ZERO_INSTRUCTIONS_OR_LATENCY");
    }
  // No statistical calculations or result output before validating all 150 rows.
  std::cout << std::setprecision(17);
  Outcome overall = Outcome::SUPPORTED;
  for (std::size_t p = 0; p < 3; ++p) {
    const auto& d = data[p];
    std::size_t fastest = 1;
    for (std::size_t m = 2; m < 5; ++m)
      if (mean(d[m].lat) < mean(d[fastest].lat)) fastest = m;
    MeasureSeries s{d[0].inst, d[1].inst, d[0].miss, d[1].miss, d[0].lat, d[fastest].lat};
    const auto outcome = classify_pattern_result(s);
    if (outcome == Outcome::NOT_SUPPORTED)
      overall = outcome;
    else if (outcome == Outcome::INCONCLUSIVE && overall == Outcome::SUPPORTED)
      overall = outcome;
    std::cout << "pattern=" << p << " outcome=" << outcome_to_string(outcome)
              << " fastest=" << fastest << '\n';
    const auto bounds = compute_bootstrap_reduction_bounds(s.candidate_inst, s.base1_inst);
    std::cout << "instruction_reduction=" << 1 - mean(s.candidate_inst) / mean(s.base1_inst)
              << " one_sided99_bounds=[" << bounds.first << ',' << bounds.second
              << "] p=" << compute_paired_t_pvalue(s.candidate_inst, s.base1_inst) << '\n';
    for (const auto& item :
         {std::make_pair("miss", std::make_pair(s.candidate_miss, s.base1_miss)),
          std::make_pair("latency", std::make_pair(s.candidate_lat, s.fastest_base_lat))}) {
      const auto ci = paired_interval99(item.second.first, item.second.second);
      std::cout << item.first << " paired_ci99=[" << ci.first << ',' << ci.second << "]\n";
    }
    if (outcome == Outcome::NOT_SUPPORTED) {
      if (bounds.second < .15) std::cout << "reason=INSTRUCTION_UPPER_BOUND_BELOW_15_PERCENT\n";
      if (paired_interval99(s.candidate_miss, s.base1_miss).first > 0)
        std::cout << "reason=MISS_REGRESSION\n";
      if (paired_interval99(s.candidate_lat, s.fastest_base_lat).first > 0)
        std::cout << "reason=LATENCY_REGRESSION\n";
    }
    for (std::size_t mode = 0; mode < 5; ++mode) {
      const std::vector<double> zero(10, 0);
      const std::string label = "p" + std::to_string(p) + "m" + std::to_string(mode);
      report(label + "_inst", d[mode].inst, zero);
      report(label + "_miss", d[mode].miss, zero);
      report(label + "_lat", d[mode].lat, zero);
      if (mode > 0) {
        report(label + "_paired_inst", d[0].inst, d[mode].inst);
        report(label + "_paired_miss", d[0].miss, d[mode].miss);
        report(label + "_paired_lat", d[0].lat, d[mode].lat);
      }
    }
  }
  std::cout << "overall=" << outcome_to_string(overall) << '\n';
} catch (const std::exception& e) {
  std::cout << "BLOCKED — " << e.what() << '\n';
  return 6;
}
