#include "internal/vm_support.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "t81/axion/context.hpp"
#include "t81/axion/nondeterminism_detector.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/vm/vm.hpp"

namespace t81::vm::internal {

namespace {

class DenyWithReasonEngine final : public t81::axion::Engine {
public:
  explicit DenyWithReasonEngine(std::string reason) : reason_(std::move(reason)) {}

  t81::axion::Verdict evaluate(const t81::axion::SyscallContext&) override {
    return {t81::axion::VerdictKind::Deny, reason_};
  }

private:
  std::string reason_;
};

}  // namespace

bool hash_matches_policy_entry(std::string_view expected, std::string_view actual_checksum) {
  if (expected == actual_checksum) {
    return true;
  }
  std::string prefixed = "sha3-512:";
  prefixed.append(actual_checksum.begin(), actual_checksum.end());
  return expected == prefixed;
}

bool is_exact_ternary_tensor(const t81::T729DynamicTensor& tensor) {
  if (tensor.numeric_class() != t81::TensorNumericClass::ExactTrit) {
    return false;
  }
  for (float value : tensor.snapshot_values()) {
    if (value != -1.0f && value != 0.0f && value != 1.0f) {
      return false;
    }
  }
  return true;
}

TierPromotionResult try_promote_tier(
    const t81::cog::TierStatus& status,
    const std::function<t81::axion::Verdict(const t81::axion::SyscallContext&)>& callback) {
  if (status.current == t81::cog::TierId::Tier5) {
    return TierPromotionResult(t81::unexpect, TierPromotionError::NotEligible);
  }

  t81::axion::SyscallContext syscall{{},      "system", "promote", "",
                                     nullptr, {},       0,         t81::tisc::Opcode::Nop};
  const auto verdict = callback(syscall);
  if (verdict.kind == t81::axion::VerdictKind::Deny) {
    return TierPromotionResult(t81::unexpect, TierPromotionError::AxionDenied);
  }

  t81::cog::TierStatus next = status;
  switch (status.current) {
    case t81::cog::TierId::Tier0:
      next.current = t81::cog::TierId::Tier1;
      next.label = "Tier1";
      break;
    case t81::cog::TierId::Tier1:
      next.current = t81::cog::TierId::Tier2;
      next.label = "Tier2";
      break;
    case t81::cog::TierId::Tier2:
      next.current = t81::cog::TierId::Tier3;
      next.label = "Tier3";
      break;
    case t81::cog::TierId::Tier3:
      next.current = t81::cog::TierId::Tier4;
      next.label = "Tier4";
      break;
    case t81::cog::TierId::Tier4:
      next.current = t81::cog::TierId::Tier5;
      next.label = "Tier5";
      break;
    default:
      return TierPromotionResult(t81::unexpect, TierPromotionError::NotEligible);
  }
  return next;
}

std::unique_ptr<t81::axion::Engine> make_deny_with_reason_engine(std::string reason) {
  return std::make_unique<DenyWithReasonEngine>(std::move(reason));
}

t81::T81Fraction fraction_from_double(double x) {
  if (x == 0.0) {
    return t81::T81Fraction{};
  }

  if (!std::isfinite(x) || std::abs(x) >= 9e18) {
    using TempFloat = t81::T81Float<72, 9>;
    auto f = TempFloat::from_double(x);
    t81::T81BigInt big = t81::T81BigInt::from_float(f);
    return t81::T81Fraction(std::move(big), t81::T81BigInt::one());
  }

  bool neg = x < 0.0;
  x = std::fabs(x);

  t81::T81BigInt integer(static_cast<std::int64_t>(x));
  double frac = x - static_cast<double>(integer.to_int64());

  t81::T81BigInt p0(1), q0(0);
  t81::T81BigInt p1(integer), q1(1);

  int iter = 0;
  while (frac > 1e-15 && iter++ < 64) {
    const double r = 1.0 / frac;
    if (r > 9e18) break;
    const std::int64_t a = static_cast<std::int64_t>(r);

    t81::T81BigInt term_a(a);
    t81::T81BigInt next_p = term_a * p1 + p0;
    t81::T81BigInt next_q = term_a * q1 + q0;

    p0 = p1;
    q0 = q1;
    p1 = next_p;
    q1 = next_q;

    frac = r - static_cast<double>(a);
  }

  return t81::T81Fraction(neg ? -p1 : p1, q1);
}

}  // namespace t81::vm::internal

namespace t81::axion {

std::size_t DeterminismDetector::hash_event(const t81::vm::AxionEvent& ev, std::size_t seed) {
  const auto mix = [](std::size_t acc, std::size_t value) {
    acc ^= value + 0x9e3779b97f4a7c15ULL + (acc << 6U) + (acc >> 2U);
    return acc;
  };
  seed = mix(seed, static_cast<std::size_t>(ev.opcode));
  seed = mix(seed, static_cast<std::size_t>(ev.tag));
  seed = mix(seed, static_cast<std::size_t>(ev.value));
  seed = mix(seed, static_cast<std::size_t>(ev.verdict.kind));
  seed = mix(seed, std::hash<std::string>{}(ev.verdict.reason));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.reason));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.policy_id));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.pc));
  seed = mix(seed, static_cast<std::size_t>(ev.structured.handle_id));
  seed = mix(seed, std::hash<std::string_view>{}(ev.structured.decision));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.event_type));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.reason_code));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.storage_class));
  seed = mix(seed, std::hash<std::string>{}(ev.structured.numeric_class));
  seed = mix(seed, ev.structured.strict_core_eligible ? 1U : 0U);
  return seed;
}

void DeterminismDetector::record_run(const std::vector<t81::vm::AxionEvent>& log) {
  prev_hashes_ = std::move(curr_hashes_);
  curr_hashes_.clear();
  curr_hashes_.reserve(log.size());

  std::size_t rolling = 0;
  for (const auto& event : log) {
    rolling = hash_event(event, rolling);
    curr_hashes_.push_back(rolling);
  }
}

DivergenceReport DeterminismDetector::check_against_previous() const {
  DivergenceReport report;
  if (prev_hashes_.empty() || curr_hashes_.empty()) {
    return report;
  }

  const std::size_t shared = std::min(prev_hashes_.size(), curr_hashes_.size());
  for (std::size_t i = 0; i < shared; ++i) {
    if (prev_hashes_[i] != curr_hashes_[i]) {
      report.diverged = true;
      report.event_index = i;
      report.reason = "Nondeterministic divergence at event " + std::to_string(i);
      return report;
    }
  }

  if (prev_hashes_.size() != curr_hashes_.size()) {
    report.diverged = true;
    report.event_index = shared;
    report.reason = "Nondeterministic divergence in event count at event " + std::to_string(shared);
  }
  return report;
}

}  // namespace t81::axion
