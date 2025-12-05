#include "t81/canonfs/axion_hook.hpp"

#include <memory>
#include <optional>
#include <sstream>
#include <string_view>

#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/tisc/opcodes.hpp"

namespace t81::canonfs {
namespace {
std::vector<std::string> g_axion_trace;
std::size_t g_meta_ptr = 0;

const char* action_to_string(OpKind kind) {
  switch (kind) {
    case OpKind::Read: return "Read";
    case OpKind::Write: return "Write";
    case OpKind::Publish: return "Publish";
    case OpKind::Revoke: return "Revoke";
  }
  return "Unknown";
}

void push_reason(std::string reason) {
  g_axion_trace.push_back(std::move(reason));
}
}  // namespace

std::function<AxionVerdict(OpKind, const CanonRef&)> make_axion_policy_hook(
    std::string policy_text) {
  std::optional<t81::axion::Policy> policy_storage;
  auto parse_res = t81::axion::parse_policy(policy_text);
  if (parse_res.has_value()) {
    policy_storage = std::move(parse_res.value());
  }
  std::shared_ptr<t81::axion::Engine> engine;
  if (policy_storage) {
    auto policy_engine = t81::axion::make_policy_engine(std::move(policy_storage));
    engine = std::move(policy_engine);
  }

  return [engine](OpKind kind, const CanonRef&) mutable {
    std::ostringstream reason;
    reason << "meta slot axion event segment=meta addr=" << g_meta_ptr++
           << " action=" << action_to_string(kind);
    auto reason_str = reason.str();
    push_reason(reason_str);
    if (!engine) {
      return AxionVerdict{true, reason_str};
    }
    t81::axion::SyscallContext ctx;
    ctx.caller = "CanonFS";
    ctx.syscall = "canonfs-" + std::string(action_to_string(kind));
    ctx.pc = 0;
    ctx.next_opcode = t81::tisc::Opcode::Nop;
    std::vector<std::string_view> trace_views;
    trace_views.reserve(g_axion_trace.size());
    for (const auto& entry : g_axion_trace) {
      trace_views.emplace_back(entry);
    }
    ctx.trace_reasons = std::move(trace_views);
    auto verdict = engine->evaluate(ctx);
    if (verdict.kind == t81::axion::VerdictKind::Deny) {
      return AxionVerdict{false, verdict.reason};
    }
    return AxionVerdict{true, reason_str};
  };
}

const std::vector<std::string>& axion_trace() {
  return g_axion_trace;
}

void reset_axion_trace() {
  g_axion_trace.clear();
  g_meta_ptr = 0;
}

}  // namespace t81::canonfs
