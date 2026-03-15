// tooling/cli/ai/t81_ai_cli.cpp
//
// T81 AI CLI commands — RFC-0032 Phase 5 (C-07).
// Promoted from: experiments/ai/ux_tools/t81_ai_cli.cpp
//
// Violations removed:
//   • std::this_thread::sleep_for() calls (non-deterministic timing stubs)
//   • std::chrono timing fields and duration calculations (non-deterministic)
//   • nlohmann/json report generation (external dependency)
//   • Hardcoded mock metadata / simulated benchmark results
//
// Wired to promoted subsystems:
//   run        → T81VmBackend::dispatch_embed()          (RFC-0032 Phase 4)
//   verify     → load_model_via_tloadhash()              (RFC-0032 Phase 3)
//   quantize   → quantize_threshold() + pack_ternary_to_base81()  (Phase 1)
//   policy test→ PolicyEngine::evaluate()                (Axion core)
//   benchmark  → dispatch each AI opcode via T81VmBackend; report Axion verdict

#include "t81/axion/ai_model_loader.hpp"
#include "t81/axion/ai_hooks.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/math/quantization/ternary_codec.hpp"
#include "t81/vm/ai_backend/backend_adapter.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace t81::ai::cli {

// ── Helpers ───────────────────────────────────────────────────────────────────

static void print_ok(const char* msg)   { std::printf("  ok  %s\n",   msg); }
static void print_deny(const char* msg) { std::printf("  deny  %s\n", msg); }
static void print_info(const char* msg) { std::printf("  ..  %s\n",   msg); }

// ── Commands ──────────────────────────────────────────────────────────────────

// run --model <hash> [--verbose]
// Verifies the hash via TLOADHASH Axion gate then dispatches EMBED through
// the T81 VM to prove the end-to-end dispatch path is active.
static int cmd_run(const std::string& model_hash, bool verbose) {
  std::printf("=== t81 ai run ===\n");

  if (model_hash.empty()) {
    std::fprintf(stderr, "error: --model <hash> required\n");
    return 1;
  }

  // Step 1: Gate model hash through TLOADHASH Axion policy.
  print_info("verifying model hash via TLOADHASH Axion gate");
  t81::axion::Policy policy;
  policy.allowed_tensor_hashes.push_back(model_hash);
  auto inner = std::make_unique<t81::axion::PolicyEngine>(policy);
  auto hook  = std::make_unique<t81::axion::AIHookEngine>(std::move(inner));
  const auto load_result =
      t81::axion::load_model_via_tloadhash(*hook, model_hash);

  if (!load_result.ok) {
    print_deny(("model rejected: " + load_result.reason).c_str());
    return 1;
  }
  print_ok(("model accepted: " + load_result.hash).c_str());

  if (verbose) {
    std::printf("  trace: %s\n", load_result.trace_event.c_str());
  }

  // Step 2: Dispatch EMBED through T81 VM to confirm inference dispatch path.
  print_info("dispatching EMBED via T81VmBackend (null handles → DecodeFault expected)");
  t81::vm::ai_backend::T81VmBackend backend(std::move(policy));
  const auto dispatch = backend.dispatch_embed(0, 0);

  // DecodeFault (not SecurityFault) proves the Axion gate passed and the VM
  // received the opcode.  SecurityFault would indicate a policy denial.
  if (dispatch.trap == t81::vm::Trap::SecurityFault) {
    print_deny("EMBED was denied by Axion policy (unexpected)");
    return 1;
  }

  print_ok("EMBED reached T81 VM execution path (Axion gate: allow)");
  return 0;
}

// verify --model <hash>
// Evaluates the model hash against the Axion TLOADHASH policy and prints the
// canonical trace event.
static int cmd_verify(const std::string& model_hash) {
  std::printf("=== t81 ai verify ===\n");

  if (model_hash.empty()) {
    std::fprintf(stderr, "error: --model <hash> required\n");
    return 1;
  }

  // Empty whitelist → deny
  {
    print_info("testing deny path (empty policy)");
    auto eng = std::make_unique<t81::axion::PolicyEngine>(t81::axion::Policy{});
    auto hk  = std::make_unique<t81::axion::AIHookEngine>(std::move(eng));
    const auto r = t81::axion::load_model_via_tloadhash(*hk, model_hash);
    if (!r.ok) {
      print_ok(("deny confirmed: " + r.trace_event).c_str());
    } else {
      print_deny("unexpected allow on empty policy");
    }
  }

  // Whitelisted hash → allow
  {
    print_info("testing allow path (hash in whitelist)");
    t81::axion::Policy pol;
    pol.allowed_tensor_hashes.push_back(model_hash);
    auto eng = std::make_unique<t81::axion::PolicyEngine>(pol);
    auto hk  = std::make_unique<t81::axion::AIHookEngine>(std::move(eng));
    const auto r = t81::axion::load_model_via_tloadhash(*hk, model_hash);
    if (r.ok) {
      print_ok(("allow confirmed: " + r.trace_event).c_str());
    } else {
      print_deny(("unexpected deny: " + r.reason).c_str());
      return 1;
    }
  }

  return 0;
}

// quantize --input <file> --output <file>
// Reads whitespace-separated integers from <input>, quantizes with scale 10
// using quantize_threshold, packs to base-81, writes packed bytes to <output>.
static int cmd_quantize(const std::string& input_path,
                        const std::string& output_path) {
  std::printf("=== t81 ai quantize ===\n");

  if (input_path.empty()) {
    std::fprintf(stderr, "error: --input <file> required\n");
    return 1;
  }

  // Read integers from input file.
  std::ifstream in(input_path);
  if (!in) {
    std::fprintf(stderr, "error: cannot open %s\n", input_path.c_str());
    return 1;
  }

  std::vector<std::int32_t> values;
  std::int32_t v;
  while (in >> v) values.push_back(v);

  if (values.empty()) {
    std::fprintf(stderr, "error: no integers in input file\n");
    return 1;
  }

  print_info(("read " + std::to_string(values.size()) + " integers").c_str());

  // Quantize with scale=10, thresholds neg=-9, pos=9.
  constexpr std::int32_t kScale = 10;
  constexpr std::int32_t kNeg   = -kScale + 1;  // -9
  constexpr std::int32_t kPos   =  kScale - 1;  //  9
  auto trits = t81::math::quantization::quantize_threshold(values, kNeg, kPos);

  print_info(("quantized to " + std::to_string(trits.size()) + " trits").c_str());

  // Pack trits to base-81 bytes.
  auto packed = t81::math::quantization::pack_ternary_to_base81(trits);

  print_info(("packed to " + std::to_string(packed.size()) + " base-81 bytes").c_str());

  // Write packed bytes.
  const std::string out_file =
      output_path.empty() ? (input_path + ".t81q") : output_path;
  std::ofstream out(out_file, std::ios::binary);
  if (!out) {
    std::fprintf(stderr, "error: cannot write %s\n", out_file.c_str());
    return 1;
  }
  out.write(reinterpret_cast<const char*>(packed.data()),
            static_cast<std::streamsize>(packed.size()));

  print_ok(("wrote " + out_file + " (" + std::to_string(packed.size()) + " bytes)").c_str());
  return 0;
}

// policy test [--type attn|qmatmul|embed|tloadhash]
// Builds a SyscallContext for the specified AI opcode and evaluates it
// through PolicyEngine, printing the verdict.
static int cmd_policy_test(const std::string& event_type) {
  std::printf("=== t81 ai policy test ===\n");

  using Opcode = t81::tisc::Opcode;
  Opcode opcode = Opcode::QMATMUL;  // default

  if (event_type == "attn") {
    opcode = Opcode::ATTN;
  } else if (event_type == "embed") {
    opcode = Opcode::EMBED;
  } else if (event_type == "tloadhash" || event_type == "model_load") {
    opcode = Opcode::TLoadHash;
  } else if (event_type == "qmatmul" || event_type.empty()) {
    opcode = Opcode::QMATMUL;
  } else {
    std::fprintf(stderr, "unknown type '%s'; using qmatmul\n", event_type.c_str());
  }

  t81::axion::SyscallContext ctx;
  ctx.next_opcode  = opcode;
  ctx.current_tier = 2;
  ctx.payload      = "policy-test-payload";
  ctx.caller       = "t81_ai_cli";

  // Evaluate through PolicyEngine (no hook, raw policy decision).
  t81::axion::PolicyEngine engine(t81::axion::Policy{});
  const auto verdict = engine.evaluate(ctx);

  const char* decision =
      (verdict.kind == t81::axion::VerdictKind::Allow) ? "allow"
    : (verdict.kind == t81::axion::VerdictKind::Deny)  ? "deny"
    : "warn";

  std::printf("  opcode=%-12s  decision=%s",
              t81::tisc::opcode_name(opcode).data(), decision);
  if (!verdict.reason.empty()) std::printf("  reason=%s", verdict.reason.c_str());
  std::printf("\n");

  return (verdict.kind == t81::axion::VerdictKind::Deny) ? 1 : 0;
}

// benchmark
// Dispatches each AI opcode through T81VmBackend and reports whether the
// Axion gate allowed or denied.  No timing fields — determinism only.
static int cmd_benchmark() {
  std::printf("=== t81 ai benchmark ===\n");

  t81::vm::ai_backend::T81VmBackend backend;

  // ATTN: tier-gated — expected SecurityFault at default Tier0.
  {
    const auto r = backend.dispatch_attn(0, 0, 0, "4x4x4");
    const char* verdict =
        (r.trap == t81::vm::Trap::SecurityFault) ? "axion-deny (tier<2)" : "reached-vm";
    std::printf("  ATTN:    %-30s  ok=%-5s\n", verdict, r.ok ? "true" : "false");
  }

  // QMATMUL: no tier gate — Axion allows, VM faults on null tensors.
  {
    const auto r = backend.dispatch_qmatmul(0, 0, 0);
    const char* verdict =
        (r.trap == t81::vm::Trap::SecurityFault) ? "axion-deny" : "reached-vm";
    std::printf("  QMATMUL: %-30s  ok=%-5s\n", verdict, r.ok ? "true" : "false");
  }

  // EMBED: no tier gate — Axion allows.
  {
    const auto r = backend.dispatch_embed(0, 0);
    const char* verdict =
        (r.trap == t81::vm::Trap::SecurityFault) ? "axion-deny" : "reached-vm";
    std::printf("  EMBED:   %-30s  ok=%-5s\n", verdict, r.ok ? "true" : "false");
  }

  // WLOAD: no tier gate — Axion allows.
  {
    const auto r = backend.dispatch_wload(0, 0);
    const char* verdict =
        (r.trap == t81::vm::Trap::SecurityFault) ? "axion-deny" : "reached-vm";
    std::printf("  WLOAD:   %-30s  ok=%-5s\n", verdict, r.ok ? "true" : "false");
  }

  print_ok("benchmark complete (no timing — determinism only)");
  return 0;
}

}  // namespace t81::ai::cli

// ── Usage ─────────────────────────────────────────────────────────────────────

static void print_usage(const char* prog) {
  std::printf(
    "T81 AI CLI — RFC-0032 Phase 5\n"
    "Usage: %s <command> [options]\n\n"
    "Commands:\n"
    "  run      --model <hash>              Verify hash + dispatch EMBED\n"
    "  verify   --model <hash>              Test TLOADHASH allow/deny\n"
    "  quantize --input <file> [--output f] Quantize integers → base-81\n"
    "  policy test [--type <opcode>]        Evaluate policy for opcode\n"
    "  benchmark                            Dispatch all AI opcodes; show verdict\n\n"
    "Opcode types for policy test: attn, qmatmul, embed, tloadhash\n",
    prog);
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
  if (argc < 2) { print_usage(argv[0]); return 0; }

  const std::string cmd = argv[1];
  std::string model_hash, input_file, output_file, event_type;
  bool verbose = false;

  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if      (a == "--model"   && i + 1 < argc) model_hash  = argv[++i];
    else if (a == "--input"   && i + 1 < argc) input_file  = argv[++i];
    else if (a == "--output"  && i + 1 < argc) output_file = argv[++i];
    else if (a == "--type"    && i + 1 < argc) event_type  = argv[++i];
    else if (a == "--verbose")                 verbose     = true;
  }

  if (cmd == "run") {
    return t81::ai::cli::cmd_run(model_hash, verbose);
  } else if (cmd == "verify") {
    return t81::ai::cli::cmd_verify(model_hash);
  } else if (cmd == "quantize") {
    return t81::ai::cli::cmd_quantize(input_file, output_file);
  } else if (cmd == "policy") {
    // "policy test [--type <type>]"
    return t81::ai::cli::cmd_policy_test(event_type);
  } else if (cmd == "benchmark") {
    return t81::ai::cli::cmd_benchmark();
  } else {
    std::fprintf(stderr, "unknown command: %s\n", cmd.c_str());
    print_usage(argv[0]);
    return 1;
  }
}
