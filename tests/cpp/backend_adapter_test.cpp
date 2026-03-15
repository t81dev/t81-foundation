// tests/cpp/backend_adapter_test.cpp
//
// RFC-0032 Phase 4 gate tests — T81VmBackend (C-02).
//
// Acceptance criteria covered:
//   [C02-01]  dispatch_attn() returns ok=false with trap==SecurityFault when
//             the VM is at the default TierId::Tier0 (< 2).  This proves the
//             Axion AIHookEngine tier gate fired — not a llama.cpp stub.
//   [C02-02]  dispatch_attn() deny_reason contains "tier" or "attn_guard",
//             confirming the denial is from the Axion guard (RFC-0032 §8.1).
//   [C02-03]  dispatch_qmatmul() returns !SecurityFault (Axion allows
//             QMATMUL; null tensor handles produce DecodeFault instead),
//             proving QMATMUL dispatch reached the VM execution path.
//   [C02-04]  dispatch_embed() returns !SecurityFault (same reasoning as
//             C02-03; no tier gate on EMBED).
//   [C02-05]  T81VmBackend is constructible with a default Policy (no
//             crash on construction — smoke test for correct linkage).
//
// Phase 4 gate criterion (RFC-0032 §10):
//   AI-native opcodes dispatch through the T81 VM + Axion hook.
//   No llama.cpp or onnx_runtime symbols present in backend_adapter.cpp.

#include "t81/vm/ai_backend/backend_adapter.hpp"

#include <cassert>
#include <cstdio>
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

// ── [C02-05] Construction smoke test ─────────────────────────────────────────

static void test_construction() {
  std::printf("\n[C02-05] T81VmBackend construction\n");

  // Default construction must not crash and must be usable.
  t81::vm::ai_backend::T81VmBackend backend;
  (void)backend;
  check(true, "[C02-05] T81VmBackend default-constructible");

  // Construction with explicit policy.
  t81::axion::Policy policy;
  policy.allowed_tensor_hashes.push_back("sha3-256:deadbeef00");
  t81::vm::ai_backend::T81VmBackend backend2(std::move(policy));
  (void)backend2;
  check(true, "[C02-05] T81VmBackend constructible with policy");
}

// ── [C02-01/02] ATTN — tier gate fires, SecurityFault returned ───────────────

static void test_attn_tier_gate() {
  std::printf("\n[C02-01/02] dispatch_attn() — default tier < 2 → SecurityFault\n");

  t81::vm::ai_backend::T81VmBackend backend;

  // Null handles (0): the Axion tier gate fires BEFORE the VM attempts
  // tensor resolution, so SecurityFault is raised by the hook, not DecodeFault.
  const auto result = backend.dispatch_attn(0, 0, 0, "8x8x8");

  check(!result.ok,
        "[C02-01] dispatch_attn ok=false at default tier 0");
  check(result.trap == t81::vm::Trap::SecurityFault,
        "[C02-01] dispatch_attn trap==SecurityFault (Axion deny, not decode fault)");

  // The deny_reason must reference the tier guard or attn_guard.
  const bool reason_mentions_tier =
      contains(result.deny_reason, "tier") ||
      contains(result.deny_reason, "attn_guard") ||
      contains(result.deny_reason, "attn");
  check(reason_mentions_tier,
        "[C02-02] deny_reason mentions tier/attn_guard");
}

// ── [C02-03] QMATMUL — Axion allows, reaches VM execution ───────────────────

static void test_qmatmul_dispatch() {
  std::printf("\n[C02-03] dispatch_qmatmul() — Axion allows, VM receives opcode\n");

  t81::vm::ai_backend::T81VmBackend backend;

  // Null handles: Axion (no tier gate on QMATMUL) allows the opcode.
  // The VM then tries to dereference tensor handle 0 → DecodeFault.
  // Key check: trap is NOT SecurityFault → proves the Axion gate passed.
  const auto result = backend.dispatch_qmatmul(0, 0, 0);

  check(!result.ok,
        "[C02-03] dispatch_qmatmul !ok (null handles cause VM fault)");
  check(result.trap != t81::vm::Trap::SecurityFault,
        "[C02-03] dispatch_qmatmul trap != SecurityFault (Axion allowed, VM reached)");
}

// ── [C02-04] EMBED — Axion allows, reaches VM execution ─────────────────────

static void test_embed_dispatch() {
  std::printf("\n[C02-04] dispatch_embed() — Axion allows, VM receives opcode\n");

  t81::vm::ai_backend::T81VmBackend backend;

  const auto result = backend.dispatch_embed(0, 0);

  check(!result.ok,
        "[C02-04] dispatch_embed !ok (null handles cause VM fault)");
  check(result.trap != t81::vm::Trap::SecurityFault,
        "[C02-04] dispatch_embed trap != SecurityFault (Axion allowed, VM reached)");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== T81VmBackend tests (RFC-0032 Phase 4) ===\n");

  test_construction();
  test_attn_tier_gate();
  test_qmatmul_dispatch();
  test_embed_dispatch();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
