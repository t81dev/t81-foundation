#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
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
  const auto unapproved_bytecode = make_load_imm_halt_bytecode(42);
  const std::string unapproved_vm_hash = sha3_512_prefixed_bytes(unapproved_bytecode);
  const std::string unapproved_rng_hash = "sha3-512:" + std::string(128, 'b');

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
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(strict_policy);
  ctx.user_consent_verified = true;
  ctx.seed = 12345;

  // Deny against deterministic provider
  ProviderRequest vm_req;
  vm_req.operation = "subgraph.run";
  vm_req.artifact = {"demo-vm", "tisc-v1.9.0", unapproved_vm_hash, std::nullopt};
  vm_req.input = {"application/x-t81-tisc", unapproved_bytecode, std::nullopt};

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
  ctrl_req.artifact = {"demo-rng", "mock-rng", unapproved_rng_hash, std::nullopt};
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
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(allow_policy);
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
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(allow_policy);
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
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(allow_policy);

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
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(allow_policy);
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

void test_at06_artifact_digest_bait_and_switch_and_canonfs_precedence() {
  std::cout << "[AT-06] Artifact digest bait-and-switch & CanonFS precedence (F-01)...\n";

  const auto prog_a = make_load_imm_halt_bytecode(11);
  const auto prog_b = make_load_imm_halt_bytecode(22);
  const std::string hash_a = sha3_512_prefixed_bytes(prog_a);
  const std::string hash_b = sha3_512_prefixed_bytes(prog_b);
  T81_REQUIRE(hash_a != hash_b);

  t81::axion::Policy allow_a;
  allow_a.tier = 1;
  allow_a.allowed_tensor_hashes.push_back(hash_a);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_a);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);
  T81VmProvider vm_provider(allow_a);

  CancellationToken cancel;
  ExecutionContext ctx;
  ctx.principal_id = "agent-f01";
  ctx.policy_sha3_512 = ProviderGovernor::compute_policy_sha3_512(allow_a);

  // 1. Inline bait-and-switch: claim hash_a while supplying prog_b bytes -> ArtifactDigestError, 0
  // invocations
  ProviderRequest bait_req;
  bait_req.operation = "subgraph.run";
  bait_req.artifact = {"bait", "tisc-v1.9.0", hash_a, std::nullopt};
  bait_req.input = {"application/x-t81-tisc", prog_b, std::nullopt};

  auto bait_out = governor.execute(vm_provider, bait_req, ctx, cancel);
  T81_REQUIRE(!bait_out.result.ok);
  T81_REQUIRE(bait_out.result.trit == -1);
  T81_REQUIRE(bait_out.result.failure_code == FailureCode::ArtifactDigestError);
  T81_REQUIRE(!bait_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);

  // 2. Dual canonfs_ref + inline mismatch: CanonFS stores prog_a, inline input supplies prog_b
  auto ref_a = canonfs->write_object(t81::canonfs::ObjectType::RawBlock, prog_a);
  T81_REQUIRE(ref_a.has_value());
  ProviderRequest dual_mismatch;
  dual_mismatch.operation = "subgraph.run";
  dual_mismatch.artifact = {"dual-mismatch", "tisc-v1.9.0", hash_a, *ref_a};
  dual_mismatch.input = {"application/x-t81-tisc", prog_b, std::nullopt};

  auto dual_out = governor.execute(vm_provider, dual_mismatch, ctx, cancel);
  T81_REQUIRE(!dual_out.result.ok);
  T81_REQUIRE(dual_out.result.failure_code == FailureCode::ArtifactDigestError);
  T81_REQUIRE(!dual_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);

  // 3. CanonFS ref mismatch against caller artifact.sha3_512 (non-inline ControlledProvider)
  auto ref_b = canonfs->write_object(t81::canonfs::ObjectType::RawBlock, prog_b);
  T81_REQUIRE(ref_b.has_value());
  ControlledProvider ctrl_provider;
  ctx.user_consent_verified = true;
  ctx.seed = 77;
  ProviderRequest ctrl_mismatch;
  ctrl_mismatch.operation = "text.generate";
  ctrl_mismatch.artifact = {"ctrl-ref-mismatch", "raw", hash_a, *ref_b};
  ctrl_mismatch.input = {"text/plain;charset=utf-8", to_bytes("hi"), std::nullopt};

  auto ctrl_out = governor.execute(ctrl_provider, ctrl_mismatch, ctx, cancel);
  T81_REQUIRE(!ctrl_out.result.ok);
  T81_REQUIRE(ctrl_out.result.failure_code == FailureCode::ArtifactDigestError);
  T81_REQUIRE(!ctrl_out.provider_invoked);
  T81_REQUIRE(ctrl_provider.invocation_count() == 0);

  // 4. Unreadable/missing CanonFS ref fails closed before provider invocation
  t81::canonfs::CanonRef missing_ref{};
  missing_ref.hash.h.bytes[0] = 0xDE;
  missing_ref.hash.h.bytes[1] = 0xAD;
  ProviderRequest missing_req;
  missing_req.operation = "subgraph.run";
  missing_req.artifact = {"missing-ref", "tisc-v1.9.0", hash_a, missing_ref};
  missing_req.input = {"application/x-t81-tisc", prog_a, std::nullopt};

  auto missing_out = governor.execute(vm_provider, missing_req, ctx, cancel);
  T81_REQUIRE(!missing_out.result.ok);
  T81_REQUIRE(missing_out.result.failure_code == FailureCode::ArtifactDigestError);
  T81_REQUIRE(!missing_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);

  // 5. Matching canonfs_ref + inline prog_a + empty caller sha3_512 -> Governor computes & allows
  ProviderRequest valid_dual;
  valid_dual.operation = "subgraph.run";
  valid_dual.artifact = {"valid-dual", "tisc-v1.9.0", "", *ref_a};
  valid_dual.input = {"application/x-t81-tisc", prog_a, std::nullopt};

  auto valid_out = governor.execute(vm_provider, valid_dual, ctx, cancel);
  T81_REQUIRE(valid_out.result.ok);
  T81_REQUIRE(valid_out.authoritative_artifact_sha3_512 == hash_a);
  T81_REQUIRE(vm_provider.invocation_count() == 1);
}

void test_at07_policy_digest_provenance_and_d4_authority() {
  std::cout << "[AT-07] Policy provenance binding (F-02), D1 determinism, and D4 authority...\n";

  const auto prog = make_load_imm_halt_bytecode(55);
  const std::string prog_hash = sha3_512_prefixed_bytes(prog);

  t81::axion::Policy policy_a;
  policy_a.tier = 1;
  policy_a.allowed_tensor_hashes.push_back(prog_hash);

  t81::axion::Policy policy_a_copy;
  policy_a_copy.tier = 1;
  policy_a_copy.allowed_tensor_hashes.push_back(prog_hash);

  t81::axion::Policy policy_b;
  policy_b.tier = 2;
  policy_b.allowed_tensor_hashes.push_back("sha3-512:" + std::string(128, 'f'));

  // D1: Verify Policy::serialize() determinism across repeated calls on identical Policy objects
  const std::string sha_a_1 = ProviderGovernor::compute_policy_sha3_512(policy_a);
  const std::string sha_a_2 = ProviderGovernor::compute_policy_sha3_512(policy_a);
  const std::string sha_a_copy = ProviderGovernor::compute_policy_sha3_512(policy_a_copy);
  const std::string sha_b = ProviderGovernor::compute_policy_sha3_512(policy_b);
  T81_REQUIRE(sha_a_1 == sha_a_2);
  T81_REQUIRE(sha_a_1 == sha_a_copy);
  T81_REQUIRE(sha_a_1 != sha_b);

  auto engine_a = std::make_shared<t81::axion::PolicyEngine>(policy_a);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor gov_a(engine_a, canonfs);
  T81VmProvider vm_provider(policy_a);

  ProviderRequest req;
  req.operation = "subgraph.run";
  req.artifact = {"prog-55", "tisc-v1.9.0", prog_hash, std::nullopt};
  req.input = {"application/x-t81-tisc", prog, std::nullopt};
  CancellationToken cancel;

  // 1. Forged/mismatched context.policy_sha3_512 -> PolicyDenied, 0 invocations, evidence records
  // sha_a_1
  ExecutionContext bad_ctx;
  bad_ctx.principal_id = "agent-f02";
  bad_ctx.policy_sha3_512 = sha_b;
  auto mismatch_out = gov_a.execute(vm_provider, req, bad_ctx, cancel);
  T81_REQUIRE(!mismatch_out.result.ok);
  T81_REQUIRE(mismatch_out.result.trit == -1);
  T81_REQUIRE(mismatch_out.result.failure_code == FailureCode::PolicyDenied);
  T81_REQUIRE(!mismatch_out.provider_invoked);
  T81_REQUIRE(vm_provider.invocation_count() == 0);
  T81_REQUIRE(mismatch_out.authoritative_policy_sha3_512 == sha_a_1);
  T81_REQUIRE(mismatch_out.evidence_json.find("\"policy_sha3_512\":\"" + sha_a_1 + "\"") !=
              std::string::npos);

  // 2. Empty context.policy_sha3_512 -> succeeds and evidence records authoritative sha_a_1
  ExecutionContext empty_ctx;
  empty_ctx.principal_id = "agent-f02";
  auto ok_out = gov_a.execute(vm_provider, req, empty_ctx, cancel);
  T81_REQUIRE(ok_out.result.ok);
  T81_REQUIRE(ok_out.authoritative_policy_sha3_512 == sha_a_1);
  T81_REQUIRE(ok_out.evidence_json.find("\"policy_sha3_512\":\"" + sha_a_1 + "\"") !=
              std::string::npos);

  // 3. M-03: Two policies differing only in TLV-v1-omitted fields produce distinct digests
  t81::axion::Policy policy_a_ext = policy_a;
  policy_a_ext.allowed_ternary_model_hashes.push_back("sha3-512:" + std::string(128, '9'));
  T81_REQUIRE(ProviderGovernor::compute_policy_sha3_512(policy_a_ext) != sha_a_1);

  // 4. m-01: Preflight denial (CapabilityMismatch) still records active authoritative policy digest
  ProviderRequest unsupported_req;
  unsupported_req.operation = "unsupported.op";
  unsupported_req.artifact = {"prog-55", "tisc-v1.9.0", prog_hash, std::nullopt};
  unsupported_req.input = {"application/x-t81-tisc", prog, std::nullopt};
  auto preflight_out = gov_a.execute(vm_provider, unsupported_req, empty_ctx, cancel);
  T81_REQUIRE(!preflight_out.result.ok);
  T81_REQUIRE(preflight_out.result.failure_code == FailureCode::CapabilityMismatch);
  T81_REQUIRE(preflight_out.authoritative_policy_sha3_512 == sha_a_1);
  T81_REQUIRE(preflight_out.evidence_json.find("\"policy_sha3_512\":\"" + sha_a_1 + "\"") !=
              std::string::npos);

  // 5. D4: Permissive VM-internal policy on T81VmProvider CANNOT weaken or bypass strict
  // ProviderGovernor
  auto engine_strict = std::make_shared<t81::axion::PolicyEngine>(policy_b);
  ProviderGovernor gov_strict(engine_strict, canonfs);
  T81VmProvider permissive_vm_provider(policy_a);  // VM-internal policy allows prog_hash
  ExecutionContext d4_ctx;
  d4_ctx.principal_id = "agent-d4";
  auto d4_out = gov_strict.execute(permissive_vm_provider, req, d4_ctx, cancel);
  T81_REQUIRE(!d4_out.result.ok);
  T81_REQUIRE(d4_out.result.failure_code == FailureCode::PolicyDenied);
  T81_REQUIRE(!d4_out.provider_invoked);
  T81_REQUIRE(permissive_vm_provider.invocation_count() == 0);
}

class ThrowingProvider final : public IAiProvider {
public:
  ThrowingProvider() {
    desc_.provider_id = "test.throwing";
    desc_.provider_version = "0.1.0";
    desc_.runtime_environment = "test-throwing";
    desc_.execution_class = ExecutionClass::ExternalProbabilistic;
    desc_.isolation_mode = IsolationMode::InProcessReadOnly;
    desc_.capabilities.supported_operations = {"op.throw"};
    desc_.capabilities.supported_input_types = {"text/plain;charset=utf-8"};
    desc_.capabilities.supported_output_types = {"text/plain;charset=utf-8"};
    desc_.capabilities.supports_cancellation = true;
  }
  const ProviderDescriptor& descriptor() const noexcept override { return desc_; }
  ProviderResult execute(const ProviderRequest&, const ExecutionContext&,
                         const CancellationToken&) override {
    throw std::runtime_error("simulated provider crash");
  }

private:
  ProviderDescriptor desc_;
};

class NonCancellableProvider final : public IAiProvider {
public:
  NonCancellableProvider() {
    desc_.provider_id = "test.noncancellable";
    desc_.provider_version = "0.1.0";
    desc_.runtime_environment = "test-noncancellable";
    desc_.execution_class = ExecutionClass::ExternalProbabilistic;
    desc_.isolation_mode = IsolationMode::InProcessReadOnly;
    desc_.capabilities.supported_operations = {"op.block"};
    desc_.capabilities.supported_input_types = {"text/plain;charset=utf-8"};
    desc_.capabilities.supported_output_types = {"text/plain;charset=utf-8"};
    desc_.capabilities.supports_cancellation = false;
  }
  const ProviderDescriptor& descriptor() const noexcept override { return desc_; }
  ProviderResult execute(const ProviderRequest&, const ExecutionContext&,
                         const CancellationToken&) override {
    ++invocations;
    ProviderResult r;
    r.ok = true;
    r.trit = 0;
    return r;
  }
  std::uint64_t invocations{0};

private:
  ProviderDescriptor desc_;
};

class FailingWriteDriver final : public t81::canonfs::Driver {
public:
  t81::canonfs::Result<t81::canonfs::CanonRef> write_object(t81::canonfs::ObjectType,
                                                            std::span<const std::byte>) override {
    return std::unexpected(t81::canonfs::Error::CapabilityError);
  }
  t81::canonfs::Result<std::vector<std::byte>> read_object_bytes(
      const t81::canonfs::CanonRef&) override {
    return std::unexpected(t81::canonfs::Error::NotFound);
  }
  t81::canonfs::Result<void> publish_capability(const t81::canonfs::CapabilityGrant&) override {
    return {};
  }
  t81::canonfs::Result<void> revoke_capability(const t81::canonfs::CanonRef&) override {
    return {};
  }
  t81::canonfs::Result<void> parity_repair_subtree(const t81::canonfs::CanonRef&) override {
    return {};
  }
  void set_axion_hook(std::function<t81::canonfs::AxionVerdict(
                          t81::canonfs::OpKind, const t81::canonfs::CanonRef&)>) override {}
};

void test_at08_exceptions_cooperative_timeout_memory_and_strict_int() {
  std::cout << "[AT-08] Exceptions, cooperative timeout, payload max_memory_bytes, strict int "
               "(F-03)...\n";

  const std::string any_hash = "sha3-512:" + std::string(128, 'e');
  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(any_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);
  CancellationToken cancel;

  // 1. Provider exception caught -> ProviderFault, trit == -1, cleared output, persisted evidence
  ThrowingProvider thrower;
  ProviderRequest throw_req;
  throw_req.operation = "op.throw";
  throw_req.artifact = {"throw-art", "raw", any_hash, std::nullopt};
  throw_req.input = {"text/plain;charset=utf-8", to_bytes("boom"), std::nullopt};
  ExecutionContext ctx;
  ctx.principal_id = "agent-f03";

  auto throw_out = governor.execute(thrower, throw_req, ctx, cancel);
  T81_REQUIRE(!throw_out.result.ok);
  T81_REQUIRE(throw_out.result.trit == -1);
  T81_REQUIRE(throw_out.result.failure_code == FailureCode::ProviderFault);
  T81_REQUIRE(throw_out.result.output.bytes.empty());
  T81_REQUIRE(throw_out.evidence_ref.has_value());
  T81_REQUIRE(ProviderGovernor::verify_persisted_evidence(*canonfs, *throw_out.evidence_ref,
                                                          throw_out.evidence_sha3_512)
                  .valid);

  // 2. timeout_ms > 0 on non-cancellable provider -> CapabilityMismatch, 0 invocations
  NonCancellableProvider noncancel;
  ProviderRequest nc_req;
  nc_req.operation = "op.block";
  nc_req.artifact = {"nc-art", "raw", any_hash, std::nullopt};
  nc_req.input = {"text/plain;charset=utf-8", to_bytes("x"), std::nullopt};
  ctx.budget.timeout_ms = 50;
  auto nc_out = governor.execute(noncancel, nc_req, ctx, cancel);
  T81_REQUIRE(!nc_out.result.ok);
  T81_REQUIRE(nc_out.result.failure_code == FailureCode::CapabilityMismatch);
  T81_REQUIRE(!nc_out.provider_invoked);
  T81_REQUIRE(noncancel.invocations == 0);

  // 3. Payload size exceeding max_memory_bytes -> BudgetExhausted, 0 invocations
  ControlledProvider ctrl;
  ProviderRequest mem_req;
  mem_req.operation = "text.generate";
  mem_req.artifact = {"mem-art", "mt19937_64", any_hash, std::nullopt};
  mem_req.input = {"text/plain;charset=utf-8", to_bytes("0123456789abcdef"), std::nullopt};
  ExecutionContext mem_ctx;
  mem_ctx.principal_id = "agent-f03";
  mem_ctx.user_consent_verified = true;
  mem_ctx.seed = 1;
  mem_ctx.budget.max_memory_bytes = 8;
  auto mem_out = governor.execute(ctrl, mem_req, mem_ctx, cancel);
  T81_REQUIRE(!mem_out.result.ok);
  T81_REQUIRE(mem_out.result.failure_code == FailureCode::BudgetExhausted);
  T81_REQUIRE(!mem_out.provider_invoked);
  T81_REQUIRE(ctrl.invocation_count() == 0);

  // 4. Strict integer parsing in ControlledProvider rejects "4abc"
  ProviderRequest bad_int_req;
  bad_int_req.operation = "text.generate";
  bad_int_req.artifact = {"int-art", "mt19937_64", any_hash, std::nullopt};
  bad_int_req.input = {"text/plain;charset=utf-8", to_bytes("hi"), std::nullopt};
  bad_int_req.parameters["tokens"] = "4abc";
  ExecutionContext int_ctx;
  int_ctx.principal_id = "agent-f03";
  int_ctx.user_consent_verified = true;
  int_ctx.seed = 1;
  auto int_out = governor.execute(ctrl, bad_int_req, int_ctx, cancel);
  T81_REQUIRE(!int_out.result.ok);
  T81_REQUIRE(int_out.result.trit == -1);
  T81_REQUIRE(int_out.result.failure_code == FailureCode::InvalidBudget);

  // 5. P2 fix: Deadline-clamped wait does not overrun timeout_ms on long simulated delay
  ControlledProvider long_delay_ctrl(/*simulated_token_delay_ms=*/2000);
  ProviderRequest clamp_req;
  clamp_req.operation = "text.generate";
  clamp_req.artifact = {"clamp-art", "mt19937_64", any_hash, std::nullopt};
  clamp_req.input = {"text/plain;charset=utf-8", to_bytes("hi"), std::nullopt};
  clamp_req.parameters["tokens"] = "2";
  ExecutionContext clamp_ctx;
  clamp_ctx.principal_id = "agent-p2-clamp";
  clamp_ctx.user_consent_verified = true;
  clamp_ctx.seed = 42;
  clamp_ctx.budget.timeout_ms = 30;
  const auto t0 = std::chrono::steady_clock::now();
  auto clamp_out = governor.execute(long_delay_ctrl, clamp_req, clamp_ctx, cancel);
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
          .count();
  T81_REQUIRE(!clamp_out.result.ok);
  T81_REQUIRE(clamp_out.result.trit == -1);
  T81_REQUIRE(clamp_out.result.failure_code == FailureCode::TimedOut);
  T81_REQUIRE(elapsed_ms < 500);

  // 6. P2 fix: Concurrent T81VmProvider invocations have isolated telemetry & identical receipts
  const auto prog = make_load_imm_halt_bytecode(77);
  const std::string prog_hash = sha3_512_prefixed_bytes(prog);
  t81::axion::Policy vm_allow;
  vm_allow.tier = 1;
  vm_allow.allowed_tensor_hashes.push_back(prog_hash);
  auto vm_engine = std::make_shared<t81::axion::PolicyEngine>(vm_allow);
  ProviderGovernor vm_gov(vm_engine, canonfs);
  T81VmProvider vm_prov(vm_allow);
  ProviderRequest vm_req;
  vm_req.operation = "subgraph.run";
  vm_req.artifact = {"conc-vm", "tisc-v1.9.0", prog_hash, std::nullopt};
  vm_req.input = {"application/x-t81-tisc", prog, std::nullopt};
  ExecutionContext vm_ctx;
  vm_ctx.principal_id = "agent-p2-conc";

  constexpr int kThreads = 4;
  std::vector<GovernedExecutionOutcome> outcomes(kThreads);
  std::vector<std::thread> workers;
  workers.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    workers.emplace_back(
        [&, i]() { outcomes[i] = vm_gov.execute(vm_prov, vm_req, vm_ctx, cancel); });
  }
  for (auto& w : workers) w.join();

  for (int i = 0; i < kThreads; ++i) {
    T81_REQUIRE(outcomes[i].result.ok);
    T81_REQUIRE(outcomes[i].result.trit == 1);
    T81_REQUIRE(outcomes[i].result.dualrail_fallback_count == 0);
    T81_REQUIRE(outcomes[i].evidence_ref.has_value());
    T81_REQUIRE(outcomes[i].evidence_sha3_512 == outcomes[0].evidence_sha3_512);
    T81_REQUIRE(outcomes[i].evidence_ref->hash == outcomes[0].evidence_ref->hash);
  }
}

void test_at09_persistence_failure_clears_stale_receipt() {
  std::cout << "[AT-09] Persistence failure fail-closed receipt clearing (F-04, D2)...\n";

  const auto prog = make_load_imm_halt_bytecode(123);
  const std::string prog_hash = sha3_512_prefixed_bytes(prog);

  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(prog_hash);
  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);

  T81VmProvider vm_provider(allow_policy);
  ProviderRequest req;
  req.operation = "subgraph.run";
  req.artifact = {"prog-123", "tisc-v1.9.0", prog_hash, std::nullopt};
  req.input = {"application/x-t81-tisc", prog, std::nullopt};
  ExecutionContext ctx;
  ctx.principal_id = "agent-f04";
  CancellationToken cancel;

  // 1. Null CanonFS driver -> EvidencePersistError, empty evidence_json/sha/ref/output
  ProviderGovernor null_driver_gov(policy_engine, nullptr);
  auto null_out = null_driver_gov.execute(vm_provider, req, ctx, cancel);
  T81_REQUIRE(!null_out.result.ok);
  T81_REQUIRE(null_out.result.trit == -1);
  T81_REQUIRE(null_out.result.failure_code == FailureCode::EvidencePersistError);
  T81_REQUIRE(null_out.result.output.bytes.empty());
  T81_REQUIRE(!null_out.evidence_ref.has_value());
  T81_REQUIRE(null_out.evidence_json.empty());
  T81_REQUIRE(null_out.evidence_sha3_512.empty());

  // 2. Failing CanonFS write_object -> EvidencePersistError, empty evidence_json/sha/ref/output
  auto failing_driver = std::make_shared<FailingWriteDriver>();
  ProviderGovernor fail_gov(policy_engine, failing_driver);
  auto fail_out = fail_gov.execute(vm_provider, req, ctx, cancel);
  T81_REQUIRE(!fail_out.result.ok);
  T81_REQUIRE(fail_out.result.trit == -1);
  T81_REQUIRE(fail_out.result.failure_code == FailureCode::EvidencePersistError);
  T81_REQUIRE(fail_out.result.output.bytes.empty());
  T81_REQUIRE(!fail_out.evidence_ref.has_value());
  T81_REQUIRE(fail_out.evidence_json.empty());
  T81_REQUIRE(fail_out.evidence_sha3_512.empty());
}

void test_at10_structural_json_spoofing_rejection() {
  std::cout << "[AT-10] Structural JSON spoofing rejection (F-05)...\n";

  // Attempt to bypass honesty check by injecting "\"execution_class\":\"deterministic\""
  // inside an unrelated string field (authority.principal_id) while actual provider is
  // seeded_probabilistic claiming bit_exact_replay.
  const std::string spoofed =
      "{\"schema\":\"t81.provider.evidence.v1\",\"trit\":1,"
      "\"authority\":{\"principal_id\":\"\\\"execution_class\\\":\\\"deterministic\\\"\"},"
      "\"provider\":{\"execution_class\":\"seeded_probabilistic\"},"
      "\"challengeability\":{\"replayable\":true,\"verification_mode\":\"bit_exact_replay\"}}";
  auto res = ProviderGovernor::verify_evidence_json(spoofed, "");
  T81_REQUIRE(!res.valid);

  // Malformed JSON must also be rejected
  auto bad_json = ProviderGovernor::verify_evidence_json("{not valid json", "");
  T81_REQUIRE(!bad_json.valid);

  // m-02: RFC 8259-invalid leading zeros rejected
  const std::string leading_zero =
      "{\"schema\":\"t81.provider.evidence.v1\",\"trit\":01,"
      "\"provider\":{\"execution_class\":\"deterministic\"},"
      "\"challengeability\":{\"replayable\":true,\"verification_mode\":\"bit_exact_replay\"}}";
  T81_REQUIRE(!ProviderGovernor::verify_evidence_json(leading_zero, "").valid);

  // m-02: Valid UTF-8 \uXXXX escape (including surrogate pair) parses cleanly
  const std::string valid_unicode =
      "{\"schema\":\"t81.provider.evidence.v1\",\"trit\":1,"
      "\"authority\":{\"principal_id\":\"agent-\\u03B1-\\uD83D\\uDE00\"},"
      "\"provider\":{\"execution_class\":\"deterministic\"},"
      "\"challengeability\":{\"replayable\":true,\"verification_mode\":\"bit_exact_replay\"}}";
  T81_REQUIRE(ProviderGovernor::verify_evidence_json(valid_unicode, "").valid);

  // M-01: UINT64_MAX seed round-trips through ControlledProvider + verify_persisted_evidence()
  const std::string model_hash = "sha3-512:" + std::string(128, '7');
  t81::axion::Policy allow_max_seed;
  allow_max_seed.tier = 1;
  allow_max_seed.allowed_tensor_hashes.push_back(model_hash);
  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_max_seed);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);
  ControlledProvider ctrl;
  ProviderRequest req;
  req.operation = "text.generate";
  req.artifact = {"max-seed-rng", "mt19937_64", model_hash, std::nullopt};
  req.input = {"text/plain;charset=utf-8", to_bytes("seed-test"), std::nullopt};
  req.parameters["tokens"] = "4";
  ExecutionContext ctx;
  ctx.principal_id = "agent-m01";
  ctx.user_consent_verified = true;
  ctx.seed = std::numeric_limits<std::uint64_t>::max();
  CancellationToken cancel;

  auto max_out = governor.execute(ctrl, req, ctx, cancel);
  T81_REQUIRE(max_out.result.ok);
  T81_REQUIRE(max_out.evidence_ref.has_value());
  T81_REQUIRE(ProviderGovernor::verify_persisted_evidence(*canonfs, *max_out.evidence_ref,
                                                          max_out.evidence_sha3_512)
                  .valid);
}

void dump_live_json_for_schema_validation() {
  const auto prog = make_load_imm_halt_bytecode(42);
  const std::string prog_hash = sha3_512_prefixed_bytes(prog);

  t81::axion::Policy allow_policy;
  allow_policy.tier = 1;
  allow_policy.allowed_tensor_hashes.push_back(prog_hash);

  auto policy_engine = std::make_shared<t81::axion::PolicyEngine>(allow_policy);
  std::shared_ptr<t81::canonfs::Driver> canonfs = t81::canonfs::make_in_memory_driver();
  ProviderGovernor governor(policy_engine, canonfs);
  T81VmProvider vm_provider(allow_policy);

  ProviderRequest req;
  req.operation = "subgraph.run";
  req.artifact = {"load-imm-42", "tisc-v1.9.0", prog_hash, std::nullopt};
  req.input = {"application/x-t81-tisc", prog, std::nullopt};

  ExecutionContext ctx;
  ctx.principal_id = "agent-schema";
  CancellationToken cancel;

  auto out = governor.execute(vm_provider, req, ctx, cancel);
  T81_REQUIRE(out.result.ok);

  const std::string caps = ProviderGovernor::serialize_capabilities_json(vm_provider.descriptor());
  const std::string rfc00d4 = ProviderGovernor::project_to_rfc00d4_json(
      out, vm_provider.descriptor(), req, ctx, "2026-11-24T00:00:00Z", "unsigned-draft",
      "urn:t81:test-key");

  std::cout << "{\"capabilities\":" << caps << ",\"evidence\":" << out.evidence_json
            << ",\"rfc00d4\":" << rfc00d4 << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::string(argv[1]) == "--dump-json") {
    dump_live_json_for_schema_validation();
    return 0;
  }
  test_at01_provider_independent_fail_closed_denial();
  test_at02_deterministic_bit_exact_replay();
  test_at03_probabilistic_honesty_and_consent_gate();
  test_at04_cancellation_budget_and_timeout();
  test_at05_offline_tamper_and_honesty_rejection();
  test_at06_artifact_digest_bait_and_switch_and_canonfs_precedence();
  test_at07_policy_digest_provenance_and_d4_authority();
  test_at08_exceptions_cooperative_timeout_memory_and_strict_int();
  test_at09_persistence_failure_clears_stale_receipt();
  test_at10_structural_json_spoofing_rejection();
  std::cout << "ALL M1 PROVIDER CONTRACT ACCEPTANCE TESTS (AT-01..AT-10) PASSED.\n";
  return 0;
}
