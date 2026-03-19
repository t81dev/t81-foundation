// tests/cpp/axion_ai_hooks_test.cpp
//
// RFC-0032 Phase 3 gate tests — AIHookEngine + AiModelLoader.
//
// Acceptance criteria covered:
//   [C03-01]  load_model_via_tloadhash returns ok=false when hash is not in
//             the policy whitelist (SecurityFault path).
//   [C03-02]  load_model_via_tloadhash returns ok=true when hash is in the
//             policy whitelist.
//   [C03-03]  AiLoadResult::trace_event contains the canonical
//             "model_load failure hash=<h> reason=..." string on deny.
//   [C03-04]  AiLoadResult::trace_event contains "model_load success hash=<h>"
//             on allow.
//   [C04-01]  AIHookEngine::ai_trace() contains "model_load failure" entry
//             after a denied TLoadHash call.
//   [C04-02]  AIHookEngine::ai_trace() contains "model_load success" entry
//             after an allowed TLoadHash call.
//   [C04-03]  AIHookEngine emits "attn_guard shape=..." before ATTN opcode.
//   [C04-04]  AIHookEngine denies ATTN when ctx.current_tier < 2.
//   [C04-05]  AIHookEngine allows ATTN when ctx.current_tier >= 2.
//   [C04-06]  AIHookEngine emits "qmatmul_guard policy=allow" for QMATMUL.
//   [C04-07]  AIHookEngine emits "ai_exec_gate backend=t81vm policy=allow"
//             for ATTN (tier >= 2), QMATMUL, EMBED.
//   [C04-08]  AIHookEngine emits "ai_exec_gate backend=t81vm policy=deny"
//             when ATTN is denied due to tier < 2.
//
// Phase 3 gate criterion (RFC-0032 §10):
//   All Axion AI event strings defined in §8.2 appear in trace output.
//   Policy-denial test passes (C03-01).

#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/ai_model_loader.hpp"
#include "t81/axion/context.hpp"
#include "t81/axion/engine.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/isa/opcodes.hpp"

#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
}

static bool contains(const std::string& s, const std::string& sub) {
  return s.find(sub) != std::string::npos;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Build a PolicyEngine that whitelists exactly one hash.
static std::unique_ptr<t81::axion::Engine>
make_policy_with_hash(const std::string& allowed_hash) {
  t81::axion::Policy policy;
  policy.allowed_tensor_hashes.push_back(allowed_hash);
  return std::make_unique<t81::axion::PolicyEngine>(std::move(policy));
}

/// Build a PolicyEngine with an empty whitelist (no hashes allowed).
static std::unique_ptr<t81::axion::Engine> make_policy_no_hashes() {
  t81::axion::Policy policy;
  // allowed_tensor_hashes left empty → TLOADHASH always denied
  return std::make_unique<t81::axion::PolicyEngine>(std::move(policy));
}

/// Build an AIHookEngine wrapping a policy that allows `allowed_hash`.
static std::unique_ptr<t81::axion::AIHookEngine>
make_ai_hook_with_hash(const std::string& allowed_hash) {
  return std::make_unique<t81::axion::AIHookEngine>(
      make_policy_with_hash(allowed_hash));
}

/// Build an AIHookEngine wrapping a policy with no whitelisted hashes.
static std::unique_ptr<t81::axion::AIHookEngine> make_ai_hook_no_hashes() {
  return std::make_unique<t81::axion::AIHookEngine>(make_policy_no_hashes());
}

// ── [C03] AiModelLoader ───────────────────────────────────────────────────────

static void test_model_load_denied() {
  std::printf("\n[C03-01/03] load_model_via_tloadhash — non-whitelisted hash → deny\n");

  auto engine = make_ai_hook_no_hashes();
  const auto result = t81::axion::load_model_via_tloadhash(
      *engine, "sha3-256:deadbeef0000000000000000000000000000000000000000000000000000dead");

  check(!result.ok,
        "[C03-01] ok=false for non-whitelisted hash");
  check(!result.trace_event.empty(),
        "[C03-03] trace_event non-empty on deny");
  check(contains(result.trace_event, "model_load failure"),
        "[C03-03] trace_event contains 'model_load failure'");
  check(contains(result.trace_event, "hash=sha3-256:deadbeef"),
        "[C03-03] trace_event contains the presented hash");
}

static void test_model_load_allowed() {
  std::printf("\n[C03-02/04] load_model_via_tloadhash — whitelisted hash → allow\n");

  const std::string allowed = "sha3-256:aabbccdd11223344aabbccdd11223344aabbccdd11223344aabbccdd11223344";
  auto engine = make_ai_hook_with_hash(allowed);
  const auto result = t81::axion::load_model_via_tloadhash(*engine, allowed);

  check(result.ok,
        "[C03-02] ok=true for whitelisted hash");
  check(contains(result.trace_event, "model_load success"),
        "[C03-04] trace_event contains 'model_load success'");
  check(contains(result.trace_event, "hash=" + allowed),
        "[C03-04] trace_event contains the allowed hash");
}

// ── [C04-01/02] AIHookEngine trace for TLoadHash ──────────────────────────────

static void test_hook_trace_model_load() {
  std::printf("\n[C04-01/02] AIHookEngine trace entries for model load\n");

  const std::string allowed = "sha3-256:1234000000000000000000000000000000000000000000000000000000001234";

  // Denied case
  {
    auto hook = make_ai_hook_no_hashes();
    (void)t81::axion::load_model_via_tloadhash(*hook, "sha3-256:badbadbad");
    const auto& trace = hook->ai_trace();
    bool has_failure = false;
    for (const auto& e : trace) {
      if (contains(e, "model_load failure")) { has_failure = true; break; }
    }
    check(has_failure, "[C04-01] trace contains 'model_load failure' entry");
  }

  // Allowed case
  {
    auto hook = make_ai_hook_with_hash(allowed);
    (void)t81::axion::load_model_via_tloadhash(*hook, allowed);
    const auto& trace = hook->ai_trace();
    bool has_success = false;
    for (const auto& e : trace) {
      if (contains(e, "model_load success")) { has_success = true; break; }
    }
    check(has_success, "[C04-02] trace contains 'model_load success' entry");
  }
}

// ── [C04-03/04/05] ATTN guard ─────────────────────────────────────────────────

static void test_attn_guard() {
  std::printf("\n[C04-03/04/05] ATTN guard: tier check + trace emission\n");

  auto hook = std::make_unique<t81::axion::AIHookEngine>(
      std::make_unique<t81::axion::PolicyEngine>(t81::axion::Policy{}));

  // Test: tier < 2 → deny
  {
    hook->clear_trace();
    t81::axion::SyscallContext ctx;
    ctx.next_opcode   = t81::tisc::Opcode::ATTN;
    ctx.current_tier  = 1;
    ctx.payload       = "64x64x64";

    const auto v = hook->evaluate(ctx);
    check(v.kind == t81::axion::VerdictKind::Deny,
          "[C04-04] ATTN denied when tier < 2");

    const auto& trace = hook->ai_trace();
    bool has_attn_guard = false, has_deny_gate = false;
    for (const auto& e : trace) {
      if (contains(e, "attn_guard shape="))         has_attn_guard = true;
      if (contains(e, "ai_exec_gate") &&
          contains(e, "policy=deny"))               has_deny_gate  = true;
    }
    check(has_attn_guard, "[C04-03] trace contains 'attn_guard shape=' entry");
    check(has_deny_gate,  "[C04-08] trace contains 'ai_exec_gate ... policy=deny' on ATTN deny");
  }

  // Test: tier >= 2 → allow (policy has no restrictions)
  {
    hook->clear_trace();
    t81::axion::SyscallContext ctx;
    ctx.next_opcode   = t81::tisc::Opcode::ATTN;
    ctx.current_tier  = 2;
    ctx.payload       = "32x32x32";

    const auto v = hook->evaluate(ctx);
    check(v.kind == t81::axion::VerdictKind::Allow,
          "[C04-05] ATTN allowed when tier >= 2");

    const auto& trace = hook->ai_trace();
    bool has_allow_gate = false;
    for (const auto& e : trace) {
      if (contains(e, "ai_exec_gate") && contains(e, "policy=allow")) {
        has_allow_gate = true; break;
      }
    }
    check(has_allow_gate, "[C04-07] trace contains 'ai_exec_gate ... policy=allow' for ATTN tier>=2");
  }
}

// ── [C04-06/07] QMATMUL guard ─────────────────────────────────────────────────

static void test_qmatmul_guard() {
  std::printf("\n[C04-06/07] QMATMUL guard trace emission\n");

  auto hook = std::make_unique<t81::axion::AIHookEngine>(
      std::make_unique<t81::axion::PolicyEngine>(t81::axion::Policy{}));

  t81::axion::SyscallContext ctx;
  ctx.next_opcode  = t81::tisc::Opcode::QMATMUL;
  ctx.payload      = "scale=10 wt_hash=sha3-256:cafecafe";

  const auto v = hook->evaluate(ctx);
  check(v.kind == t81::axion::VerdictKind::Allow,
        "[C04-06] QMATMUL allowed by policy with no restrictions");

  const auto& trace = hook->ai_trace();
  bool has_qmatmul_guard = false, has_allow_gate = false;
  for (const auto& e : trace) {
    if (contains(e, "qmatmul_guard policy=allow")) has_qmatmul_guard = true;
    if (contains(e, "ai_exec_gate") && contains(e, "policy=allow")) has_allow_gate = true;
  }
  check(has_qmatmul_guard, "[C04-06] trace contains 'qmatmul_guard policy=allow'");
  check(has_allow_gate,    "[C04-07] trace contains 'ai_exec_gate ... policy=allow' for QMATMUL");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Axion AI hooks tests (RFC-0032 Phase 3) ===\n");

  test_model_load_denied();
  test_model_load_allowed();
  test_hook_trace_model_load();
  test_attn_guard();
  test_qmatmul_guard();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
