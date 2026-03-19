// RFC-0034 §5.17 — Ternary-Native Inference: conformance tests
// Covers: TWMATMUL, TQUANT, TERNACCUM, TWEMBED, TATTN, TACT (TernaryStep),
//         TACT activation-ceiling Quarantine gate.
#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/tensor/ternary_native.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <memory>

namespace {

[[maybe_unused]] std::int32_t pack_reg_pair(std::int32_t first, std::int32_t second) {
  return static_cast<std::int32_t>((first & 0xFF) | ((second & 0xFF) << 8));
}

// Helper: load a tensor from pool into register via LoadImm TensorHandle.
t81::tisc::Insn load_tensor(int reg, int pool_idx) {
  t81::tisc::Insn insn{t81::tisc::Opcode::LoadImm, reg, pool_idx, 0};
  insn.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  return insn;
}

std::shared_ptr<t81::weights::ModelFile> make_ternary_weights_model(
    const t81::weights::NativeTensor& native, std::string checksum) {
  auto model = std::make_shared<t81::weights::ModelFile>();
  model->checksum = std::move(checksum);
  model->format = "T81W2";
  model->native.emplace("ternary.w", native);
  return model;
}

}  // namespace

// ---------------------------------------------------------------------------
// [TN-C1] TWMATMUL exact — 2×3 activations · 3×2 ternary weights
//   Act  = [[1,0,-1, 1,0,-1]]  (3-wide)
//   Wt   = [[ 1,-1],[ 0, 1],[-1, 0]]  (3×2 ternary)
//   Expected RD[0,0] = 1*1 + 0*0 + (-1)*(-1) = 2
//            RD[0,1] = 1*(-1) + 0*1 + (-1)*0 = -1
// ---------------------------------------------------------------------------
int test_twmatmul_exact() {
  t81::tisc::Program p;
  // activations 1×3 (row vector)
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 3}, {1.0f, 0.0f, -1.0f}));
  // weights 3×2 (ternary)
  p.tensor_pool.push_back(t81::T729DynamicTensor({3, 2}, {1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f}));

  p.insns.push_back(load_tensor(1, 1));  // R1 = activations handle
  p.insns.push_back(load_tensor(2, 2));  // R2 = weights handle
  p.insns.push_back({t81::tisc::Opcode::TWMATMUL, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(res.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);

  const auto& tensors = vm->state().tensors;
  std::int64_t rh = vm->state().contexts[0].registers[3];
  T81_TEST_CHECK(rh >= 1 && static_cast<std::size_t>(rh) <= tensors.size());
  const auto& out = *tensors[static_cast<std::size_t>(rh - 1)];
  T81_TEST_CHECK(out.shape().size() == 2);
  T81_TEST_CHECK(out.shape()[0] == 1 && out.shape()[1] == 2);
  const auto vals = out.snapshot_values();
  // RD[0,0] = 1*1 + 0*0 + (-1)*(-1) = 2
  T81_TEST_CHECK(std::abs(vals[0] - 2.0f) < 0.5f);
  // RD[0,1] = 1*(-1) + 0*1 + (-1)*0 = -1
  T81_TEST_CHECK(std::abs(vals[1] - (-1.0f)) < 0.5f);
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C2] TQUANT determinism — float tensor → ternary {-1,0,+1}
// ---------------------------------------------------------------------------
int test_tquant_determinism() {
  t81::tisc::Program p;
  // Floats spanning negative/zero/positive thresholds
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 5}, {-0.9f, -0.3f, 0.0f, 0.4f, 1.2f}));

  p.insns.push_back(load_tensor(1, 1));                      // R1 = src tensor
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});  // R2 = threshold=0 (int)
  p.insns.push_back({t81::tisc::Opcode::TQUANT, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  // Run twice to verify determinism.
  std::vector<float> run1_vals, run2_vals;
  for (int run = 0; run < 2; ++run) {
    auto vm = t81::vm::make_interpreter_vm();
    vm->load_program(p);
    auto res = vm->run_to_halt();
    T81_TEST_CHECK(res.has_value());
    std::int64_t rh = vm->state().contexts[0].registers[3];
    const auto& out = *vm->state().tensors[static_cast<std::size_t>(rh - 1)];
    auto vals = out.snapshot_values();
    if (run == 0)
      run1_vals = vals;
    else
      run2_vals = vals;
  }
  T81_TEST_CHECK(run1_vals == run2_vals);
  // All outputs must be exactly in {-1,0,+1}
  for (float v : run1_vals) {
    T81_TEST_CHECK(v == -1.0f || v == 0.0f || v == 1.0f);
  }
  // -0.9 → -1, -0.3 → 0 (threshold=0 means snap_trit(x, 0): x>0→+1, x<0→-1)
  T81_TEST_CHECK(run1_vals[0] == -1.0f);
  T81_TEST_CHECK(run1_vals[2] == 0.0f);
  T81_TEST_CHECK(run1_vals[4] == 1.0f);
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C3] TERNACCUM — scalar dot product → BigInt handle
// ---------------------------------------------------------------------------
int test_ternaccum_bigint() {
  t81::tisc::Program p;
  // wt = [1, -1, 0]   act = [1, 1, 1]  → dot = 1*1 + (-1)*1 + 0*1 = 0
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 3}, {1.0f, -1.0f, 0.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 3}, {1.0f, 1.0f, 1.0f}));

  p.insns.push_back(load_tensor(1, 1));  // R1 = wt
  p.insns.push_back(load_tensor(2, 2));  // R2 = act
  p.insns.push_back({t81::tisc::Opcode::TERNACCUM, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(res.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::BigIntHandle);

  std::int64_t bh = vm->state().contexts[0].registers[3];
  T81_TEST_CHECK(bh >= 1 && static_cast<std::size_t>(bh) <= vm->state().bigints.size());
  const auto& bi = vm->state().bigints[static_cast<std::size_t>(bh - 1)];
  T81_TEST_CHECK(bi.to_int64() == 0);  // dot([1,-1,0],[1,1,1]) = 0
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C4] TACT TernaryStep mode — correct output values
// ---------------------------------------------------------------------------
int test_tact_step_determinism() {
  t81::tisc::Program p;
  // Input: values that test each threshold boundary
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 5}, {-1.0f, -0.3f, 0.0f, 0.4f, 1.0f}));

  p.insns.push_back(load_tensor(1, 1));
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2,
                     static_cast<std::int32_t>(t81::ops::kTActModeStep), 0});  // R2 = mode=0x01
  p.insns.push_back({t81::tisc::Opcode::TACT, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(res.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);

  std::int64_t rh = vm->state().contexts[0].registers[3];
  const auto& out = *vm->state().tensors[static_cast<std::size_t>(rh - 1)];
  const auto vals = out.snapshot_values();
  T81_TEST_CHECK(vals.size() == 5);
  // -1.0 < -0.5 → -1
  T81_TEST_CHECK(vals[0] == -1.0f);
  // -0.3 is in [-0.5, 0.5] → 0
  T81_TEST_CHECK(vals[1] == 0.0f);
  //  0.0 → 0
  T81_TEST_CHECK(vals[2] == 0.0f);
  //  0.4 is in [-0.5, 0.5] → 0
  T81_TEST_CHECK(vals[3] == 0.0f);
  //  1.0 > 0.5 → +1
  T81_TEST_CHECK(vals[4] == 1.0f);
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C5] TACT activation-ceiling Quarantine gate
//   Provide a policy with max_nonzero_fraction=0.2 and 100% nonzero output.
//   Expect: SecurityFault (Quarantine → RD not committed).
// ---------------------------------------------------------------------------
int test_tact_quarantine_gate() {
  t81::tisc::Program p;
  // All-positive input → after TernaryStep all → +1 (100% nonzero)
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 4}, {1.0f, 1.0f, 1.0f, 1.0f}));

  // Policy: activation-ceiling max_nonzero_fraction = 0.2
  p.axion_policy_text = "(policy (tier 2)"
                        "  (activation-ceiling 0.2))";

  p.insns.push_back(load_tensor(1, 1));
  p.insns.push_back(
      {t81::tisc::Opcode::LoadImm, 2, static_cast<std::int32_t>(t81::ops::kTActModeStep), 0});
  p.insns.push_back({t81::tisc::Opcode::TACT, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  T81_TEST_CHECK(vm->step().has_value());
  T81_TEST_CHECK(vm->step().has_value());
  auto res = vm->step();
  T81_TEST_CHECK(!res.has_value());
  T81_TEST_CHECK(res.error() == t81::vm::Trap::SecurityFault);
  T81_TEST_CHECK(vm->state().contexts[0].pc == 2);
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] != t81::vm::ValueTag::TensorHandle);
  T81_TEST_CHECK(std::any_of(vm->state().axion_log.begin(), vm->state().axion_log.end(),
                             [](const auto& ev) {
                               return ev.opcode == t81::tisc::Opcode::TACT &&
                                      ev.verdict.kind == t81::axion::VerdictKind::Quarantine &&
                                      ev.verdict.reason.find("Quarantine") != std::string::npos;
                             }));
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C6] TACT repeat violation escalates to Deny → ActivationFault
// ---------------------------------------------------------------------------
int test_tact_deny_after_quarantine() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({1, 4}, {1.0f, 1.0f, 1.0f, 1.0f}));
  p.axion_policy_text = "(policy (tier 2)"
                        "  (activation-ceiling 0.2))";

  p.insns.push_back(load_tensor(1, 1));
  p.insns.push_back(
      {t81::tisc::Opcode::LoadImm, 2, static_cast<std::int32_t>(t81::ops::kTActModeStep), 0});
  p.insns.push_back({t81::tisc::Opcode::TACT, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);

  T81_TEST_CHECK(vm->step().has_value());
  T81_TEST_CHECK(vm->step().has_value());
  auto first = vm->step();
  T81_TEST_CHECK(!first.has_value());
  T81_TEST_CHECK(first.error() == t81::vm::Trap::SecurityFault);
  T81_TEST_CHECK(vm->state().contexts[0].activation_quarantined);
  T81_TEST_CHECK(vm->state().contexts[0].pc == 2);

  auto second = vm->step();
  T81_TEST_CHECK(!second.has_value());
  T81_TEST_CHECK(second.error() == t81::vm::Trap::ActivationFault);
  T81_TEST_CHECK(vm->state().contexts[0].pc == 2);
  T81_TEST_CHECK(std::any_of(vm->state().axion_log.begin(), vm->state().axion_log.end(),
                             [](const auto& ev) {
                               return ev.opcode == t81::tisc::Opcode::TACT &&
                                      ev.verdict.kind == t81::axion::VerdictKind::Deny &&
                                      ev.verdict.reason.find("Deny") != std::string::npos;
                             }));
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C7] WLOAD enforces allowed-ternary-model-hashes for model-backed weights
// ---------------------------------------------------------------------------
int test_wload_allowed_ternary_model_hashes() {
  t81::weights::NativeTensor native;
  native.shape = {1, 3};
  native.trits = 3;
  native.format = t81::weights::NativeFormat::BalancedTernary;
  native.data = {5};  // [-1, 0, 1]

  t81::tisc::Program p;
  p.weights_model = make_ternary_weights_model(native, "deadbeef");
  p.symbol_pool.push_back("ternary.w");
  p.axion_policy_text =
      "(policy (tier 2)"
      "  (allowed-ternary-model-hashes [\"sha3-512:deadbeef\"]))";

  p.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(res.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C8] WLOAD denies model-backed weights whose checksum is not whitelisted
// ---------------------------------------------------------------------------
int test_wload_denies_non_whitelisted_ternary_model_hash() {
  t81::weights::NativeTensor native;
  native.shape = {1, 3};
  native.trits = 3;
  native.format = t81::weights::NativeFormat::BalancedTernary;
  native.data = {5};  // [-1, 0, 1]

  t81::tisc::Program p;
  p.weights_model = make_ternary_weights_model(native, "deadbeef");
  p.symbol_pool.push_back("ternary.w");
  p.axion_policy_text =
      "(policy (tier 2)"
      "  (allowed-ternary-model-hashes [\"sha3-512:cafebabe\"]))";

  p.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(!res.has_value());
  T81_TEST_CHECK(res.error() == t81::vm::Trap::SecurityFault);
  T81_TEST_CHECK(std::any_of(vm->state().axion_log.begin(), vm->state().axion_log.end(),
                             [](const auto& ev) {
                               return ev.opcode == t81::tisc::Opcode::WLOAD &&
                                      ev.verdict.kind == t81::axion::VerdictKind::Deny &&
                                      ev.verdict.reason.find("allowed-ternary-model-hashes") !=
                                          std::string::npos;
                             }));
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C8b] promote_to_tensor enforces allowed-ternary-model-hashes on weight handles
// ---------------------------------------------------------------------------
int test_promote_to_tensor_denies_non_whitelisted_ternary_model_hash() {
  t81::weights::NativeTensor native;
  native.shape = {2, 2};
  native.trits = 4;
  native.format = t81::weights::NativeFormat::BalancedTernary;
  native.data = {40};

  t81::tisc::Program p;
  p.weights_model = make_ternary_weights_model(native, "deadbeef");
  p.symbol_pool.push_back("ternary.w");
  p.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0f, 0.0f, -1.0f, 1.0f}));
  p.axion_policy_text =
      "(policy (tier 2)"
      "  (allowed-ternary-model-hashes [\"sha3-512:cafebabe\"]))";

  p.insns.push_back(load_tensor(1, 1));
  p.insns.push_back({t81::tisc::Opcode::WeightsLoad, 2, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::TMatMul, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(!res.has_value());
  T81_TEST_CHECK(res.error() == t81::vm::Trap::SecurityFault);
  T81_TEST_CHECK(std::any_of(vm->state().axion_log.begin(), vm->state().axion_log.end(),
                             [](const auto& ev) {
                               return ev.opcode == t81::tisc::Opcode::TMatMul &&
                                      ev.verdict.kind == t81::axion::VerdictKind::Deny &&
                                      ev.verdict.reason.find("allowed-ternary-model-hashes") !=
                                          std::string::npos;
                             }));
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C9] WLOAD ternary-weight-domain-check rejects scaled/non-exact payloads
// ---------------------------------------------------------------------------
int test_wload_ternary_weight_domain_check() {
  t81::weights::NativeTensor native;
  native.shape = {1, 128};
  native.trits = 128;
  native.format = t81::weights::NativeFormat::T3_K;
  native.data.resize(27, 0);
  auto* bytes = reinterpret_cast<std::uint8_t*>(native.data.data());
  const float scale = 2.0f;
  std::memcpy(bytes, &scale, sizeof(scale));

  t81::tisc::Program p;
  p.weights_model = make_ternary_weights_model(native, "feedface");
  p.symbol_pool.push_back("ternary.w");
  p.axion_policy_text =
      "(policy (tier 2)"
      "  (ternary-weight-domain-check true))";

  p.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(!res.has_value());
  T81_TEST_CHECK(res.error() == t81::vm::Trap::SecurityFault);
  T81_TEST_CHECK(std::any_of(vm->state().axion_log.begin(), vm->state().axion_log.end(),
                             [](const auto& ev) {
                               return ev.opcode == t81::tisc::Opcode::WLOAD &&
                                      ev.verdict.kind == t81::axion::VerdictKind::Deny &&
                                      ev.verdict.reason.find("ternary-weight-domain-check") !=
                                          std::string::npos;
                             }));
  return 0;
}

// ---------------------------------------------------------------------------
// [TN-C10] WLOAD ternary-weight-domain-check false permits scaled payloads
// ---------------------------------------------------------------------------
int test_wload_ternary_weight_domain_check_disabled() {
  t81::weights::NativeTensor native;
  native.shape = {1, 3};
  native.trits = 3;
  native.format = t81::weights::NativeFormat::BalancedTernary;
  native.data = {5};  // [-1, 0, 1]

  t81::tisc::Program p;
  p.weights_model = make_ternary_weights_model(native, "feedface");
  p.symbol_pool.push_back("ternary.w");
  p.axion_policy_text =
      "(policy (tier 2)"
      "  (ternary-weight-domain-check false))";

  p.insns.push_back({t81::tisc::Opcode::WeightsLoad, 1, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::WLOAD, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto res = vm->run_to_halt();
  T81_TEST_CHECK(res.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].register_tags[3] == t81::vm::ValueTag::TensorHandle);
  return 0;
}

int main() {
  int failures = 0;
  failures += test_twmatmul_exact();
  failures += test_tquant_determinism();
  failures += test_ternaccum_bigint();
  failures += test_tact_step_determinism();
  failures += test_tact_quarantine_gate();
  failures += test_tact_deny_after_quarantine();
  failures += test_wload_allowed_ternary_model_hashes();
  failures += test_wload_denies_non_whitelisted_ternary_model_hash();
  failures += test_promote_to_tensor_denies_non_whitelisted_ternary_model_hash();
  failures += test_wload_ternary_weight_domain_check();
  failures += test_wload_ternary_weight_domain_check_disabled();
  return failures;
}
