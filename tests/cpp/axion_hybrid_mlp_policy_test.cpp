// axion_hybrid_mlp_policy_test.cpp
//
// Validates that the HybridMLP Axion policy (kernel/axion/policies/hybrid_mlp.axp)
// parses correctly and evaluates the expected verdicts:
//
//   1. Policy parses without error.
//   2. Policy compiles to non-empty bytecode and round-trips through serialization.
//   3. Tier < 3 is denied.
//   4. Instruction count > 8M is denied.
//   5. Stack usage > 512KB is denied.
//   6. Compliant context (tier=3, instructions=1M, stack=256KB,
//      audit reason present) is allowed.
//
// This test is intentionally a standalone main() to match the existing
// Axion test convention (no framework dependency).

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"

// The policy text is embedded here rather than file-loaded so the test is
// self-contained and runnable without the build tree's working directory.
// It MUST stay byte-for-byte equivalent to kernel/axion/policies/hybrid_mlp.axp
// (minus comment lines, which the lexer skips).
static constexpr std::string_view kHybridMlpPolicy = R"policy(
(policy
  (tier 3)
  (max-instructions 8000000)
  (max-stack 524288)
  (max-tensors 32)
  (max-tensor-elements 2097152)
  (ternary-weight-domain-check false)
  (activation-ceiling 0.8)
  (require-axion-event
    (reason "hybrid-mlp-dispatch-approved")))
)policy";

int main() {
  using namespace t81::axion;

  int failures = 0;
  auto expect = [&](bool cond, const char* msg) {
    if (!cond) {
      std::cerr << "FAIL: " << msg << "\n";
      ++failures;
    }
  };

  // 1. Parse
  auto res = parse_policy(kHybridMlpPolicy);
  expect(res.has_value(), "parse_policy: hybrid_mlp.axp did not parse");
  if (!res.has_value()) {
    std::cerr << "  error: " << res.error() << "\n";
    return 1;
  }
  Policy policy = std::move(res.value());

  expect(policy.tier == 3,                                   "tier should be 3");
  expect(policy.max_instructions.has_value(),                "max-instructions should be set");
  expect(policy.max_instructions.value_or(0) == 8000000,    "max-instructions should be 8000000");
  expect(policy.max_stack.has_value(),                       "max-stack should be set");
  expect(policy.max_stack.value_or(0) == 524288,            "max-stack should be 524288");
  expect(!policy.ternary_weight_domain_check,                "ternary-weight-domain-check should be false");
  expect(policy.activation_ceiling_max_nonzero_fraction.has_value(),
         "activation-ceiling should be set");
  expect(!policy.axion_event_requirements.empty(),           "require-axion-event should be set");
  if (!policy.axion_event_requirements.empty()) {
    expect(policy.axion_event_requirements[0].reason == "hybrid-mlp-dispatch-approved",
           "axion event reason mismatch");
  }

  // 2. Compile + round-trip
  policy.compile_to_bytecode();
  expect(!policy.bytecode.empty(), "bytecode should be non-empty after compile");

  std::stringstream ss;
  policy.serialize(ss);
  auto deser = Policy::deserialize(ss);
  expect(deser.has_value(), "deserialize should succeed");

  // 3–6: Evaluate verdicts via PolicyEngine
  Policy eval_policy = policy;  // copy for engine (bytecode already compiled)
  PolicyEngine engine(std::move(eval_policy));

  // 3. Tier too low → Deny
  {
    SyscallContext ctx{};
    ctx.current_tier = 2;
    ctx.instruction_count = 100;
    ctx.stack_usage = 1024;
    ctx.trace_reasons = {"hybrid-mlp-dispatch-approved"};
    auto v = engine.evaluate(ctx);
    expect(v.kind == VerdictKind::Deny, "tier=2 should be Denied");
  }

  // 4. Instruction count exceeded → Deny
  {
    SyscallContext ctx{};
    ctx.current_tier = 3;
    ctx.instruction_count = 9000000;  // > 8M limit
    ctx.stack_usage = 1024;
    ctx.trace_reasons = {"hybrid-mlp-dispatch-approved"};
    auto v = engine.evaluate(ctx);
    expect(v.kind == VerdictKind::Deny, "instruction_count=9M should be Denied");
  }

  // 5. Stack exceeded → Deny
  {
    SyscallContext ctx{};
    ctx.current_tier = 3;
    ctx.instruction_count = 1000000;
    ctx.stack_usage = 600000;  // > 512KB limit
    ctx.trace_reasons = {"hybrid-mlp-dispatch-approved"};
    auto v = engine.evaluate(ctx);
    expect(v.kind == VerdictKind::Deny, "stack_usage=600KB should be Denied");
  }

  // 6. Compliant context → Allow
  {
    SyscallContext ctx{};
    ctx.current_tier = 3;
    ctx.instruction_count = 1000000;
    ctx.stack_usage = 262144;  // 256KB — within limit
    ctx.trace_reasons = {"hybrid-mlp-dispatch-approved"};
    auto v = engine.evaluate(ctx);
    expect(v.kind == VerdictKind::Allow, "compliant context should be Allowed");
  }

  if (failures == 0) {
    std::cout << "axion_hybrid_mlp_policy_test: all checks passed\n";
    return 0;
  }
  std::cerr << failures << " check(s) failed\n";
  return 1;
}
