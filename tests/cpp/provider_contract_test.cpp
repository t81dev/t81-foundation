#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "provider/controlled_provider.hpp"
#include "provider/provider_contract.hpp"
#include "provider/provider_governor.hpp"
#include "provider/t81_vm_provider.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"

#define T81_REQUIRE(cond)                                                             \
  do {                                                                                \
    if (!(cond)) {                                                                    \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #cond << "\n"; \
      std::exit(1);                                                                   \
    }                                                                                 \
  } while (0)

namespace {

using namespace t81::provider;

std::string sha3_512_prefixed_bytes(const std::vector<std::byte>& bytes) {
  auto u8 = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()),
                                          bytes.size());
  return "sha3-512:" + t81::crypto::sha3_512_hex(u8);
}

std::vector<std::byte> make_load_imm_halt_bytecode(std::int64_t imm_val) {
  t81::tisc::Program prog;
  prog.insns.push_back(t81::tisc::Insn{t81::tisc::Opcode::LoadImm, 1, imm_val, 0});
  prog.insns.push_back(t81::tisc::Insn{t81::tisc::Opcode::Halt, 0, 0, 0});
  return t81::tisc::encode(prog);
}

std::vector<std::byte> to_bytes(const std::string& s) {
  std::vector<std::byte> out(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    out[i] = static_cast<std::byte>(s[i]);
  }
  return out;
}

void test_at01_provider_independent_fail_closed_denial() {
  std::cout << "[AT-01] Provider-independent fail-closed Axion denial...\n";

  const std::string approved_hash = "sha3-512:" + std::string(128, 'a');
  const std::string unapproved_hash = "sha3-512:" + std::string(128, 'b');

  t81::axion::Policy strict_policy;
  strict_policy.tier = 1;
  strict_policy.allowed_tensor_hashes.push_back(approved_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(strict_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);

  T81VmProvider vm_provider;
  ControlledProvider ctrl_provider;

  CancellationToken cancel;
  ExecutionContext ctx;
  ctx.correlation_id = "at01-deny";
  ctx.principal_id = "agent-audit";
  ctx.policy_sha3_512 = "sha3-512:" + std::string(128, '1');
  ctx.user_consent_verified = true;
  ctx.seed = 12345;

  // Deny against deterministic provider
  ProviderRequest vm_req;
  vm_req.operation = "subgraph.run";
  vm_req.artifact = {"demo-vm", "tisc-v1.9.0", unapproved_hash, std::nullopt};
  vm_req.input = {"application/x-t81-tisc", make_load_imm_halt_bytecode(42), std::nullopt};

  auto vm_out = governor.execute(vm_provider, vm_req, ctx, cancel);
  T81_REQUIRE(!vm_out.result.ok);
  T81_REQUIRE(vm_out.result.trit == -1);
  T81_REQUIRE(vm_out.result.failure_code == FailureCode::PolicyDenied);
  T81_REQUIRE(!vm_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);
  T81_REQUIRE(vm_out.evidence_ref.has_value());
  T81_REQUIRE(ProviderGovernor::verify_persisted_evidence(*canonfs, *vm_out.evidence_ref,
                                                          vm_out.evidence_sha3_512)
                  .valid);

  // Deny against probabilistic provider under identical policy
  ProviderRequest ctrl_req;
  ctrl_req.operation = "text.generate";
  ctrl_req.artifact = {"demo-rng", "mock-rng", unapproved_hash, std::nullopt};
  ctrl_req.input = {"text/plain;charset=utf-8", to_bytes("hello"), std::nullopt};

  auto ctrl_out = governor.execute(ctrl_provider, ctrl_req, ctx, cancel);
  T81_REQUIRE(!ctrl_out.result.ok);
  T81_REQUIRE(ctrl_out.result.trit == -1);
  T81_REQUIRE(ctrl_out.result.failure_code == FailureCode::PolicyDenied);
  T81_REQUIRE(!ctrl_out.provider_invoked);
  T81_REQUIRE(ctrl_provider.invocation_count() == 0);
  T81_REQUIRE(ctrl_out.evidence_ref.has_value());

  // Missing/null policy must also fail closed
  ProviderGovernor null_gov(nullptr, canonfs);
  auto null_out = null_gov.execute(vm_provider, vm_req, ctx, cancel);
  T81_REQUIRE(!null_out.result.ok);
  T81_REQUIRE(null_out.result.failure_code == FailureCode::PolicyDenied);
  T81_REQUIRE(vm_provider.invocation_count() == 0);
}

void test_at02_deterministic_bit_exact_replay() {
  std::cout << "[AT-02] Deterministic T81VmProvider bit-exact repeat replay...\n";

  const auto bytecode = make_load_imm_halt_bytecode(42);
  const std::string bc_hash = sha3_512_prefixed_bytes(bytecode);

  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(bc_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);

  T81VmProvider vm_provider(allow_policy);
  CancellationToken cancel;

  ProviderRequest req;
  req.operation = "subgraph.run";
  req.artifact = {"load-imm-42", "tisc-v1.9.0", bc_hash, std::nullopt};
  req.input = {"application/x-t81-tisc", bytecode, std::nullopt};

  ExecutionContext ctx;
  ctx.correlation_id = "at02-replay";
  ctx.principal_id = "agent-det";
  ctx.policy_sha3_512 = "sha3-512:" + std::string(128, '2');
  ctx.budget.max_steps = 100;

  auto run1 = governor.execute(vm_provider, req, ctx, cancel);
  auto run2 = governor.execute(vm_provider, req, ctx, cancel);

  T81_REQUIRE(run1.result.ok && run2.result.ok);
  T81_REQUIRE(run1.result.trit == 1 && run2.result.trit == 1);
  T81_REQUIRE(run1.provider_invoked && run2.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 2);
  T81_REQUIRE(run1.result.dualrail_fallback_count == 0);
  T81_REQUIRE(run1.result.output.bytes == run2.result.output.bytes);
  T81_REQUIRE(run1.evidence_sha3_512 == run2.evidence_sha3_512);
  T81_REQUIRE(run1.evidence_ref.has_value() && run2.evidence_ref.has_value());
  T81_REQUIRE(run1.evidence_ref->hash == run2.evidence_ref->hash);

  const std::string out_str(reinterpret_cast<const char*>(run1.result.output.bytes.data()),
                            run1.result.output.bytes.size());
  T81_REQUIRE(out_str.find("\"r1\":42") != std::string::npos);

  auto ver = ProviderGovernor::verify_persisted_evidence(*canonfs, *run1.evidence_ref,
                                                         run1.evidence_sha3_512);
  T81_REQUIRE(ver.valid);

  // Verify capabilities JSON serialization
  const std::string cap_json =
      ProviderGovernor::serialize_capabilities_json(vm_provider.descriptor());
  T81_REQUIRE(cap_json.find("\"schema\":\"t81.provider.capabilities.v1\"") != std::string::npos);
  T81_REQUIRE(cap_json.find("\"execution_class\":\"deterministic\"") != std::string::npos);
}

void test_at03_probabilistic_honesty_and_consent_gate() {
  std::cout << "[AT-03] Probabilistic ControlledProvider consent gate & bounded attestation...\n";

  const std::string model_hash = "sha3-512:" + std::string(128, 'c');
  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(model_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);

  ControlledProvider ctrl_provider;
  CancellationToken cancel;

  ProviderRequest req;
  req.operation = "text.generate";
  req.artifact = {"controlled-rng-v1", "mt19937_64", model_hash, std::nullopt};
  req.input = {"text/plain;charset=utf-8", to_bytes("prompt"), std::nullopt};
  req.parameters["tokens"] = "6";

  ExecutionContext ctx;
  ctx.correlation_id = "at03-prob";
  ctx.principal_id = "agent-prob";
  ctx.policy_sha3_512 = "sha3-512:" + std::string(128, '3');
  ctx.seed = 424242;
  ctx.budget.max_output_tokens = 16;

  // 1. Without verified user consent -> fail-closed ConsentRequired, 0 invocations
  ctx.user_consent_verified = false;
  auto denied = governor.execute(ctrl_provider, req, ctx, cancel);
  T81_REQUIRE(!denied.result.ok);
  T81_REQUIRE(denied.result.trit == -1);
  T81_REQUIRE(denied.result.failure_code == FailureCode::ConsentRequired);
  T81_REQUIRE(!denied.provider_invoked);
  T81_REQUIRE(ctrl_provider.invocation_count() == 0);

  // 2. With verified user consent -> succeeds with honest seeded_bounded_attestation
  ctx.user_consent_verified = true;
  auto allowed = governor.execute(ctrl_provider, req, ctx, cancel);
  T81_REQUIRE(allowed.result.ok);
  T81_REQUIRE(allowed.result.trit == 1);
  T81_REQUIRE(allowed.provider_invoked);
  T81_REQUIRE(ctrl_provider.invocation_count() == 1);
  T81_REQUIRE(allowed.result.tokens_consumed == 6);
  T81_REQUIRE(allowed.evidence_json.find("\"execution_class\":\"seeded_probabilistic\"") !=
              std::string::npos);
  T81_REQUIRE(allowed.evidence_json.find("\"verification_mode\":\"seeded_bounded_attestation\"") !=
              std::string::npos);
  T81_REQUIRE(allowed.evidence_json.find("\"replayable\":false") != std::string::npos);
  T81_REQUIRE(allowed.evidence_ref.has_value());
  T81_REQUIRE(ProviderGovernor::verify_persisted_evidence(*canonfs, *allowed.evidence_ref,
                                                          allowed.evidence_sha3_512)
                  .valid);

  // 3. RFC-00D4 projection contains all 6 primitives
  const std::string adb = ProviderGovernor::project_to_rfc00d4_json(
      allowed, ctrl_provider.descriptor(), req, ctx, "2026-11-24T00:00:00Z");
  T81_REQUIRE(adb.find("\"meaning\":") != std::string::npos);
  T81_REQUIRE(adb.find("\"evidence\":") != std::string::npos);
  T81_REQUIRE(adb.find("\"method\":") != std::string::npos);
  T81_REQUIRE(adb.find("\"authority\":") != std::string::npos);
  T81_REQUIRE(adb.find("\"accountability\":") != std::string::npos);
  T81_REQUIRE(adb.find("\"challengeability\":") != std::string::npos);
}

void test_at04_cancellation_budget_and_timeout() {
  std::cout << "[AT-04] Cancellation, budget exhaustion, and timeout enforcement...\n";

  const auto bytecode = make_load_imm_halt_bytecode(7);
  const std::string bc_hash = sha3_512_prefixed_bytes(bytecode);
  const std::string rng_hash = "sha3-512:" + std::string(128, 'd');

  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(bc_hash);
  allow_policy.allowed_tensor_hashes.push_back(rng_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);

  T81VmProvider vm_provider(allow_policy);
  ProviderRequest vm_req;
  vm_req.operation = "subgraph.run";
  vm_req.artifact = {"load-imm-7", "tisc-v1.9.0", bc_hash, std::nullopt};
  vm_req.input = {"application/x-t81-tisc", bytecode, std::nullopt};

  ExecutionContext ctx;
  ctx.principal_id = "agent-limits";
  ctx.policy_sha3_512 = "sha3-512:" + std::string(128, '4');

  // 1. Pre-cancelled token -> Cancelled, 0 invocations
  CancellationToken pre_cancelled;
  pre_cancelled.request_cancel();
  auto c_out = governor.execute(vm_provider, vm_req, ctx, pre_cancelled);
  T81_REQUIRE(!c_out.result.ok);
  T81_REQUIRE(c_out.result.trit == -1);
  T81_REQUIRE(c_out.result.failure_code == FailureCode::Cancelled);
  T81_REQUIRE(!c_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);

  // 2. Step-budget exhaustion (2-insn program with max_steps=1)
  CancellationToken active;
  ctx.budget.max_steps = 1;
  auto b_out = governor.execute(vm_provider, vm_req, ctx, active);
  T81_REQUIRE(!b_out.result.ok);
  T81_REQUIRE(b_out.result.trit == -1);
  T81_REQUIRE(b_out.result.failure_code == FailureCode::BudgetExhausted);
  T81_REQUIRE(b_out.result.output.bytes.empty());

  // 3. Timeout enforcement on slow controlled provider
  ControlledProvider slow_provider(/*simulated_token_delay_ms=*/15);
  ProviderRequest slow_req;
  slow_req.operation = "text.generate";
  slow_req.artifact = {"slow-rng", "mt19937_64", rng_hash, std::nullopt};
  slow_req.input = {"text/plain;charset=utf-8", to_bytes("hi"), std::nullopt};
  slow_req.parameters["tokens"] = "5";

  ctx.user_consent_verified = true;
  ctx.seed = 99;
  ctx.budget.max_output_tokens = 16;
  ctx.budget.timeout_ms = 20;
  auto t_out = governor.execute(slow_provider, slow_req, ctx, active);
  T81_REQUIRE(!t_out.result.ok);
  T81_REQUIRE(t_out.result.trit == -1);
  T81_REQUIRE(t_out.result.failure_code == FailureCode::TimedOut);
  T81_REQUIRE(t_out.result.output.bytes.empty());
}

void test_at05_offline_tamper_and_honesty_rejection() {
  std::cout << "[AT-05] Offline tamper detection and probabilistic honesty rejection...\n";

  const auto bytecode = make_load_imm_halt_bytecode(99);
  const std::string bc_hash = sha3_512_prefixed_bytes(bytecode);

  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(bc_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);

  T81VmProvider vm_provider(allow_policy);
  ProviderRequest req;
  req.operation = "subgraph.run";
  req.artifact = {"load-imm-99", "tisc-v1.9.0", bc_hash, std::nullopt};
  req.input = {"application/x-t81-tisc", bytecode, std::nullopt};

  ExecutionContext ctx;
  ctx.principal_id = "agent-verify";
  ctx.policy_sha3_512 = "sha3-512:" + std::string(128, '5');
  CancellationToken cancel;

  auto out = governor.execute(vm_provider, req, ctx, cancel);
  T81_REQUIRE(out.result.ok);

  // 1. Flip 1 byte in evidence JSON -> rejected fail-closed
  std::string tampered = out.evidence_json;
  const auto pos = tampered.find("\"trit\":1");
  T81_REQUIRE(pos != std::string::npos);
  tampered[pos + 7] = '0';
  auto tamper_res = ProviderGovernor::verify_evidence_json(tampered, out.evidence_sha3_512);
  T81_REQUIRE(!tamper_res.valid);

  // 2. Dishonest probabilistic bundle claiming bit_exact_replay -> rejected fail-closed
  const std::string dishonest =
      "{\"schema\":\"t81.provider.evidence.v1\","
      "\"provider\":{\"execution_class\":\"seeded_probabilistic\"},"
      "\"challengeability\":{\"replayable\":true,\"verification_mode\":\"bit_exact_replay\"}}";
  auto dishonest_res = ProviderGovernor::verify_evidence_json(dishonest, "");
  T81_REQUIRE(!dishonest_res.valid);
}

}  // namespace

int main() {
  test_at01_provider_independent_fail_closed_denial();
  test_at02_deterministic_bit_exact_replay();
  test_at03_probabilistic_honesty_and_consent_gate();
  test_at04_cancellation_budget_and_timeout();
  test_at05_offline_tamper_and_honesty_rejection();
  std::cout << "ALL M1 PROVIDER CONTRACT ACCEPTANCE TESTS (AT-01..AT-05) PASSED.\n";
  return 0;
}
