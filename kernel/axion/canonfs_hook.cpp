#include "t81/canonfs/axion_hook.hpp"

#include <memory>
#include <optional>
#include <string_view>

#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/reasons.hpp"
#include "t81/isa/opcodes.hpp"

namespace t81::canonfs {
namespace {
std::vector<std::string> g_axion_trace;
std::size_t g_meta_ptr = 0;

const char* action_to_string(OpKind kind) {
  switch (kind) {
    case OpKind::Read:
      return "Read";
    case OpKind::Write:
      return "Write";
    case OpKind::Publish:
      return "Publish";
    case OpKind::Revoke:
      return "Revoke";
    case OpKind::Repair:
      return "Repair";
  }
  return "Unknown";
}

void push_reason(std::string reason) { g_axion_trace.push_back(std::move(reason)); }
}  // namespace

std::function<AxionVerdict(OpKind, const CanonRef&)> make_axion_policy_hook(
    std::string policy_text) {
  std::shared_ptr<t81::axion::Engine> engine;
  // Parsing can throw on out-of-range numbers. Invalid/absent policy always
  // installs a rejecting callback, never an unconfigured bootstrap callback.
  try {
    auto parsed = t81::axion::parse_policy(policy_text);
    if (parsed) engine = t81::axion::make_policy_engine(std::move(*parsed));
  } catch (...) {
    // The returned callback below denies when no engine could be constructed.
  }

  return [engine](OpKind kind, const CanonRef&) mutable {
    const std::size_t slot = g_meta_ptr++;
    auto reason_str = t81::axion::reasons::canonical_meta_slot_reason(slot, action_to_string(kind));
    push_reason(reason_str);

    // Secondary trace record: canonical meta event reason for policy audit.
    push_reason(t81::axion::reasons::canonical_meta_event_reason(slot, action_to_string(kind)));

    if (!engine) {
      return AxionVerdict{false, "CanonFS policy missing or invalid"};
    }
    t81::axion::SyscallContext ctx;
    ctx.caller = "CanonFS";
    ctx.syscall = "canonfs-" + std::string(action_to_string(kind));
    ctx.pc = 0;
    ctx.final_authorization = true;
    // Per-operation proposed metadata mutation, not a VM/session counter.
    ctx.meta_write_count = kind == OpKind::Read ? 0 : 1;
    // Authorize from this request's events, not another operation/driver's
    // historical audit entries. These events describe an attempted operation.
    const auto event_reason =
        t81::axion::reasons::canonical_meta_event_reason(slot, action_to_string(kind));
    ctx.trace_reasons = {reason_str, event_reason};
    try {
      auto verdict = engine->evaluate(ctx);
      if (verdict.kind != t81::axion::VerdictKind::Allow) {
        return AxionVerdict{false, verdict.reason};
      }
    } catch (...) {
      return AxionVerdict{false, "CanonFS policy evaluation failed"};
    }
    return AxionVerdict{true, reason_str};
  };
}

const std::vector<std::string>& axion_trace() { return g_axion_trace; }

void reset_axion_trace() {
  g_axion_trace.clear();
  g_meta_ptr = 0;
}

}  // namespace t81::canonfs
