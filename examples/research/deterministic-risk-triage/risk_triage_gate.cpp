// Deterministic Cyber-Defense Risk & Bayesian Triage Gate
// Consumes locked v2.0.0 Core Data Types: T81Prob<27>, T81Fraction<81>, T81Result
// Demonstrates bit-exact integer log-odds Bayesian updating (no float underflow)
// paired with exact rational severity thresholds and fail-closed Result gating.

#include "t81/crypto/sha3.hpp"
#include "t81/types/T81Fraction.hpp"
#include "t81/types/T81Prob.hpp"
#include "t81/types/T81Result.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using t81::T81Int;
using t81::T81Result;
using t81::T81String;
using t81::T81Symbol;
using t81::v1::T81Fraction;
using t81::v1::T81Prob;

namespace {

struct Signal {
  std::string name;
  std::int64_t log_odds_delta;  // Fixed-point base-phi log-odds units (kScale = 512)
  std::int64_t impact_num;      // Exact rational impact numerator
  std::int64_t impact_den;      // Exact rational impact denominator
};

struct TriageVerdict {
  std::string action;
  std::int64_t posterior_log_odds;
  std::int64_t impact_num;
  std::int64_t impact_den;
  std::string digest_sha3_512;
};

std::string to_hex(const std::array<std::uint8_t, 64>& d) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.reserve(128);
  for (std::uint8_t b : d) {
    out.push_back(kHex[(b >> 4) & 0xF]);
    out.push_back(kHex[b & 0xF]);
  }
  return out;
}

T81Result<TriageVerdict> evaluate_alert(
    std::int64_t prior_log_odds,
    const std::vector<Signal>& signals,
    std::int64_t escalate_log_odds_floor,
    const T81Fraction<81>& max_allowed_impact) {
  if (signals.empty()) {
    return T81Result<TriageVerdict>::failure(
        T81Symbol::intern("empty_signals"),
        T81String("fail-closed: empty telemetry signal vector"));
  }

  T81Prob<27> posterior{T81Int<27>(prior_log_odds)};
  T81Fraction<81> total_impact(T81Int<81>(0), T81Int<81>(1));

  for (const auto& s : signals) {
    if (s.impact_den <= 0) {
      return T81Result<TriageVerdict>::failure(
          T81Symbol::intern("invalid_denominator"),
          T81String("fail-closed: non-positive impact denominator on signal " + s.name));
    }
    // Pure balanced-ternary saturating log-odds update (no exp/log/div in hot path)
    posterior = posterior + T81Prob<27>{T81Int<27>(s.log_odds_delta)};
    // Exact rational impact accumulation (zero IEEE-754 rounding drift)
    total_impact = total_impact + T81Fraction<81>(T81Int<81>(s.impact_num), T81Int<81>(s.impact_den));
  }

  const std::int64_t post_raw = posterior.log_odds().to_int64();
  const bool high_confidence_compromise = (post_raw >= escalate_log_odds_floor);
  const bool impact_exceeds_budget = (total_impact > max_allowed_impact);

  const std::string action =
      (high_confidence_compromise || impact_exceeds_budget) ? "CONTAIN_AND_ESCALATE" : "ALLOW_MONITORED";

  // Canonical SHA3-512 commitment over exact integer/rational state (zero float serialization)
  const std::string canonical = action + "|" + std::to_string(post_raw) + "|" +
                                std::to_string(total_impact.num().to_int64()) + "/" +
                                std::to_string(total_impact.den().to_int64());
  const std::vector<std::uint8_t> bytes(canonical.begin(), canonical.end());
  const auto digest = t81::crypto::sha3_512(bytes);

  return T81Result<TriageVerdict>::success(TriageVerdict{
      action,
      post_raw,
      total_impact.num().to_int64(),
      total_impact.den().to_int64(),
      "sha3-512:" + to_hex(digest),
  });
}

}  // namespace

int main() {
  // Exact rational budget: 3/10 (classic IEEE-754 0.1 + 0.2 == 0.3 boundary)
  const T81Fraction<81> max_budget(T81Int<81>(3), T81Int<81>(10));
  const std::int64_t escalate_floor = 1024;  // +2.0 base-phi log-odds units

  // Case 1: Benign telemetry right at exact 3/10 boundary (1/10 + 2/10 == 3/10 -> NOT exceeded)
  const auto r1 = evaluate_alert(
      -512,
      {
          {"signed_binary_verified", -256, 1, 10},
          {"expected_config_reload", 128, 2, 10},
      },
      escalate_floor,
      max_budget);

  // Case 2: Prompt-injection + unsigned model weight attempt -> exceeds both log-odds & impact
  const auto r2 = evaluate_alert(
      -512,
      {
          {"unsigned_model_weight", 1200, 1, 3},
          {"mode_downgrade_attempt", 900, 1, 4},
      },
      escalate_floor,
      max_budget);

  // Case 3: Malformed denominator -> fail-closed T81Result error
  const auto r3 = evaluate_alert(0, {{"corrupt_sensor", 100, 1, 0}}, escalate_floor, max_budget);

  if (!r1.is_ok() || !r2.is_ok() || r3.is_ok()) {
    std::cerr << "FAIL: unexpected triage result state\n";
    return 1;
  }
  if (r1.value().action != "ALLOW_MONITORED" || r1.value().impact_num != 3 ||
      r1.value().impact_den != 10) {
    std::cerr << "FAIL: Case 1 exact 1/10 + 2/10 == 3/10 boundary failed\n";
    return 1;
  }
  if (r2.value().action != "CONTAIN_AND_ESCALATE" || r2.value().impact_num != 7 ||
      r2.value().impact_den != 12) {
    std::cerr << "FAIL: Case 2 escalation or exact 1/3 + 1/4 == 7/12 failed\n";
    return 1;
  }

  std::cout << "{\n"
            << "  \"schema\": \"t81.deterministic-risk-triage.v1\",\n"
            << "  \"ok\": true,\n"
            << "  \"case1_exact_boundary\": {\"action\": \"" << r1.value().action
            << "\", \"posterior_log_odds\": " << r1.value().posterior_log_odds
            << ", \"impact\": \"" << r1.value().impact_num << "/" << r1.value().impact_den
            << "\", \"digest\": \"" << r1.value().digest_sha3_512 << "\"},\n"
            << "  \"case2_adversarial_escalate\": {\"action\": \"" << r2.value().action
            << "\", \"posterior_log_odds\": " << r2.value().posterior_log_odds
            << ", \"impact\": \"" << r2.value().impact_num << "/" << r2.value().impact_den
            << "\", \"digest\": \"" << r2.value().digest_sha3_512 << "\"},\n"
            << "  \"case3_malformed_fail_closed\": {\"error\": \"" << r3.error().message.str() << "\"}\n"
            << "}\n";
  return 0;
}
