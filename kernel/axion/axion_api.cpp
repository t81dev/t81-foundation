#include <cstring>
#include "t81/axion/api.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/policy_engine.hpp"

namespace t81::axion {

AxionContext::AxionContext() : engine_(make_allow_all_engine()) {}

AxionContext::AxionContext(std::unique_ptr<Engine> engine) : engine_(std::move(engine)) {}

AxionContext::~AxionContext() = default;

Verdict AxionContext::evaluate(const SyscallContext& ctx) {
  tele_.requests++;
  if (!engine_) {
    return Verdict{VerdictKind::Allow, "No engine attached (default allow)"};
  }

  // Delegation to the attached engine for policy-based evaluation.
  Verdict v = engine_->evaluate(ctx);

  if (v.kind == VerdictKind::Deny) {
    tele_.denies++;
  }

  tele_.last_ms = 0.042;  // Deterministic placeholder for "processing time"
  return v;
}

Status AxionContext::submit(const Signal& sig, const Buffer& in, Buffer& out) {
  // Evaluate the signal kind and flags against the active policy.
  SyscallContext submit_ctx{};
  submit_ctx.syscall = "submit_signal";
  submit_ctx.instruction_count = sig.kind;  // Using kind as a pseudo-count for policy matching

  // Create a reason for the trace that the policy engine can inspect.
  const std::string reason_kind = "signal kind=" + std::to_string(sig.kind);
  const std::string reason_flags = "signal flags=" + std::to_string(sig.flags);
  submit_ctx.trace_reasons.push_back(reason_kind);
  submit_ctx.trace_reasons.push_back(reason_flags);

  Verdict v = evaluate(submit_ctx);
  if (v.kind == VerdictKind::Deny) {
    return Status::Denied;
  }

  // Processing logic for validated signals.
  constexpr uint8_t trailer_magic[4] = {'A', 'X', 'N', '\x02'};  // bump version
  out.data.clear();
  out.data.reserve(in.data.size() + sizeof(trailer_magic) + 16);

  out.data.insert(out.data.end(), in.data.begin(), in.data.end());
  out.data.insert(out.data.end(), std::begin(trailer_magic), std::end(trailer_magic));

  auto append_u32 = [&](uint32_t x) {
    out.data.push_back(static_cast<uint8_t>(x & 0xFF));
    out.data.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    out.data.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    out.data.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
  };
  auto append_u64 = [&](uint64_t x) {
    for (int i = 0; i < 8; ++i) out.data.push_back(static_cast<uint8_t>((x >> (8 * i)) & 0xFF));
  };

  append_u32(sig.kind);
  append_u32(sig.flags);
  append_u64(sig.nonce);

  tele_.bytes_in += in.data.size();
  tele_.bytes_out += out.data.size();

  return Status::Ok;
}

std::vector<Capability> AxionContext::capabilities() const {
  std::vector<Capability> caps;
  caps.push_back({1, "DeterministicExecution", true});
  caps.push_back({2, "PolicyEnforcement", true});
  caps.push_back({3, "TelemetryObservability", true});
  caps.push_back({4, "CanonFSPersistence", true});
  return caps;
}

}  // namespace t81::axion
