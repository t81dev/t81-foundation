#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "exact_oracle_adapter.hpp"
#include "tier1_markov_harness.hpp"
#include "tier2_work_harness.hpp"

// Fail-closed test assertion active under -DNDEBUG
#define SCG08_REQUIRE(cond, msg)                                                             \
  do {                                                                                       \
    if (!(cond)) {                                                                           \
      std::cerr << "\n[SCG08 TEST FAILED] " << (msg) << " (" #cond ")" << " at " << __FILE__ \
                << ":" << __LINE__ << std::endl;                                             \
      std::exit(1);                                                                          \
    }                                                                                        \
  } while (false)

namespace scg08 {

// ============================================================================
// Test 1: Tier 1 Markov Carry Dynamics and Decoupled Counters
// ============================================================================
void test_tier1_markov_dynamics() {
  std::cout << "[SCG-08 GATE 1] Testing Tier 1 Markov carry dynamics (smoke "
               "test)...\n";

  auto t_run1 = Tier1MarkovHarness::simulate_ternary(5, 20, 0x5C608001ULL);
  auto t_run2 = Tier1MarkovHarness::simulate_ternary(5, 20, 0x5C608001ULL);

  SCG08_REQUIRE(t_run1.total_digits == 100, "Tier 1 ternary total digits must be 100");
  SCG08_REQUIRE(t_run1.total_digits == t_run2.total_digits, "Tier 1 deterministic replay mismatch");
  SCG08_REQUIRE(t_run1.quiescent_to_active == t_run2.quiescent_to_active, "Tier 1 replay mismatch");
  SCG08_REQUIRE(t_run1.active_cout == t_run2.active_cout, "Tier 1 replay mismatch");
  SCG08_REQUIRE(t_run1.carry_runs == t_run2.carry_runs, "Tier 1 replay mismatch");

  std::cout << "  Ternary f_gen|0: " << t_run1.f_gen_given_0() << ", f_occ: " << t_run1.f_occ()
            << ", mean_run_len: " << t_run1.mean_run_length() << "\n";

  SCG08_REQUIRE(t_run1.f_gen_given_0() > 0.10 && t_run1.f_gen_given_0() < 0.35,
                "Ternary f_gen|0 outside sanity range");
  SCG08_REQUIRE(t_run1.f_occ() > 0.15 && t_run1.f_occ() < 0.35,
                "Ternary f_occ outside sanity range");
  SCG08_REQUIRE(t_run1.mean_run_length() > 1.0 && t_run1.mean_run_length() < 2.5,
                "Ternary mean run length outside sanity range");

  // Binary control run
  auto b_run = Tier1MarkovHarness::simulate_binary(5, 20, 0x5C608001ULL);
  SCG08_REQUIRE(b_run.total_digits == 100, "Tier 1 binary total digits must be 100");
  std::cout << "  Binary f_gen|0: " << b_run.f_gen_given_0() << ", f_occ: " << b_run.f_occ()
            << ", mean_run_len: " << b_run.mean_run_length() << "\n";

  SCG08_REQUIRE(b_run.f_gen_given_0() > 0.10 && b_run.f_gen_given_0() < 0.40,
                "Binary f_gen|0 outside sanity range");
  SCG08_REQUIRE(b_run.f_occ() > 0.30 && b_run.f_occ() < 0.70, "Binary f_occ outside sanity range");
  SCG08_REQUIRE(b_run.mean_run_length() >= 1.5,
                "Binary mean run length must be longer than single-trit expectations");
}

// ============================================================================
// Test 2: Accumulator-Width Specification Verification
// ============================================================================
void test_accumulator_width_specification() {
  std::cout << "[SCG-08 GATE 1] Verifying exact accumulator-width tables per "
               "Addendum 06...\n";

  for (size_t B : {64, 128, 256, 512, 1024}) {
    const auto& spec = get_accumulator_spec(B);

    size_t expected_operand_trits =
        static_cast<size_t>(std::ceil(static_cast<double>(B) / std::log2(3.0)));
    SCG08_REQUIRE(spec.operand_trits == expected_operand_trits,
                  "Operand trits mismatch against ceil(B / log2(3))");

    ExactBigIntOracle max_sum = ExactBigIntOracle::from_power_of_two(B);
    size_t t_add_calc = 0;
    while (true) {
      t_add_calc++;
      ExactBigIntOracle p3(1);
      for (size_t i = 0; i < t_add_calc; ++i) p3 = ExactBigIntOracle::mul(p3, ExactBigIntOracle(3));
      p3 = ExactBigIntOracle::sub(p3, ExactBigIntOracle(1));
      ExactBigIntOracle req = ExactBigIntOracle::mul(max_sum, ExactBigIntOracle(2));
      req = ExactBigIntOracle::add(req, ExactBigIntOracle(1));
      if (p3 >= req) break;
    }
    SCG08_REQUIRE(spec.add_trits_min == t_add_calc, "add_trits_min mismatch");

    ExactBigIntOracle max_prod = ExactBigIntOracle::from_power_of_two(2 * B - 2);
    size_t t_mul_calc = 0;
    while (true) {
      t_mul_calc++;
      ExactBigIntOracle p3(1);
      for (size_t i = 0; i < t_mul_calc; ++i) p3 = ExactBigIntOracle::mul(p3, ExactBigIntOracle(3));
      p3 = ExactBigIntOracle::sub(p3, ExactBigIntOracle(1));
      ExactBigIntOracle req = ExactBigIntOracle::mul(max_prod, ExactBigIntOracle(2));
      req = ExactBigIntOracle::add(req, ExactBigIntOracle(1));
      if (p3 >= req) break;
    }
    SCG08_REQUIRE(spec.mul_trits_min == t_mul_calc, "mul_trits_min mismatch");

    SCG08_REQUIRE(spec.ternary_limbs_add * 27 >= spec.add_trits_min,
                  "Ternary addition limbs must accommodate add_trits_min");
    SCG08_REQUIRE(spec.ternary_limbs_mul * 27 >= spec.mul_trits_min,
                  "Ternary multiplication limbs must accommodate mul_trits_min");

    std::cout << "  Band B=" << B << " bits: OpTrits=" << spec.operand_trits
              << ", AddTrits=" << spec.add_trits_min << " (" << spec.ternary_limbs_add << " limbs)"
              << ", MulTrits=" << spec.mul_trits_min << " (" << spec.ternary_limbs_mul
              << " limbs)\n";
  }
}

// ============================================================================
// Test 3: Tier 2 Multi-Precision Arithmetic Work & 5-D Oracle Verification
// ============================================================================
template <size_t T_In, size_t T_Add, size_t T_Mul, size_t B_In, size_t B_Add, size_t B_Mul>
void run_band_verification(size_t bit_width) {
  std::cout << "  Verifying Band B=" << bit_width << " bits (TernaryLimbs: In=" << T_In
            << ", Add=" << T_Add << ", Mul=" << T_Mul << "; BinaryLimbs: In=" << B_In
            << ", Add=" << B_Add << ", Mul=" << B_Mul << ")...\n";

  ExactBigIntOracle val_zero(0);
  ExactBigIntOracle val_one(1);
  ExactBigIntOracle val_neg_one(-1);

  ExactBigIntOracle val_max = ExactBigIntOracle::from_power_of_two(bit_width - 1);
  val_max = ExactBigIntOracle::sub(val_max, ExactBigIntOracle(1));

  ExactBigIntOracle val_min = ExactBigIntOracle::from_power_of_two(bit_width - 1);
  val_min.negative = true;

  std::vector<std::pair<ExactBigIntOracle, ExactBigIntOracle>> test_pairs = {
      {val_zero, val_zero}, {val_one, val_one}, {val_one, val_neg_one}, {val_max, val_min},
      {val_max, val_max},   {val_min, val_min}, {val_min, val_one}};

  for (const auto& [oa, ob] : test_pairs) {
    OracleValidationReport oracle_rep;

    // Dimension 4: Correct sign handling
    oracle_rep.sign_handling = true;
    if (oa.negative && oa > ExactBigIntOracle(0)) oracle_rep.sign_handling = false;
    if (!oa.negative && oa < ExactBigIntOracle(0)) oracle_rep.sign_handling = false;
    enforce_oracle_parity(oracle_rep.sign_handling, "Oracle sign representation invalid");

    // Dimension 2: Overflow classification
    bool a_in_domain = is_in_signed_b_domain(oa, bit_width);
    bool b_in_domain = is_in_signed_b_domain(ob, bit_width);
    oracle_rep.overflow_classification = a_in_domain && b_in_domain;
    enforce_oracle_parity(oracle_rep.overflow_classification,
                          "Operand overflow classification failed");

    // Convert to Ternary representations
    auto ta = TernaryMultiLimb<T_In>::from_oracle(oa);
    auto tb = TernaryMultiLimb<T_In>::from_oracle(ob);

    // Dimension 3: Canonical balanced-ternary digits verification
    auto ta_trits = ta.to_trits();
    auto tb_trits = tb.to_trits();
    oracle_rep.canonical_digits = true;
    for (int8_t t : ta_trits) {
      if (t < -1 || t > 1) oracle_rep.canonical_digits = false;
    }
    for (int8_t t : tb_trits) {
      if (t < -1 || t > 1) oracle_rep.canonical_digits = false;
    }
    enforce_oracle_parity(oracle_rep.canonical_digits, "Ternary operands must be canonical trits");

    // Operation Accounting
    OperationAccounting acc_t_add;
    OperationAccounting acc_t_mul;

    // Execute Ternary Addition
    auto t_sum = ArithmeticWorkHarness::add_ternary<T_In, T_Add>(ta, tb, acc_t_add);
    auto t_sum_trits = t_sum.to_trits();
    auto computed_oa_sum = trits_to_oracle(t_sum_trits);
    auto expected_oa_sum = ExactBigIntOracle::add(oa, ob);

    // Dimension 1: Exact numerical equality
    enforce_oracle_parity(computed_oa_sum == expected_oa_sum,
                          "Ternary addition numerical equality failed");

    // Dimension 5: Absence of truncation (full precision retained)
    oracle_rep.absence_of_truncation = (computed_oa_sum == expected_oa_sum);
    enforce_oracle_parity(oracle_rep.absence_of_truncation, "Ternary addition truncation detected");

    // Execute Ternary Multiplication
    auto t_prod = ArithmeticWorkHarness::mul_ternary<T_In, T_Mul>(ta, tb, acc_t_mul);
    auto t_prod_trits = t_prod.to_trits();
    auto computed_oa_prod = trits_to_oracle(t_prod_trits);
    auto expected_oa_prod = ExactBigIntOracle::mul(oa, ob);

    enforce_oracle_parity(computed_oa_prod == expected_oa_prod,
                          "Ternary multiplication numerical equality failed");

    // Verify independent binary control execution and oracle equality
    OperationAccounting acc_b_add;
    OperationAccounting acc_b_mul;

    auto ba = BinaryMultiLimb<B_In>::from_oracle(oa);
    auto bb = BinaryMultiLimb<B_In>::from_oracle(ob);

    auto b_sum = ArithmeticWorkHarness::add_binary<B_In, B_Add>(ba, bb, acc_b_add);
    auto b_prod = ArithmeticWorkHarness::mul_binary<B_In, B_Mul>(ba, bb, acc_b_mul);

    enforce_oracle_parity(b_sum.to_oracle() == expected_oa_sum,
                          "Binary addition numerical equality failed");
    enforce_oracle_parity(b_prod.to_oracle() == expected_oa_prod,
                          "Binary multiplication numerical equality failed");

    // Check operation accounting invariant
    SCG08_REQUIRE(acc_t_add.compute_w_ops() > 0, "W_ops for addition must be non-zero");
    SCG08_REQUIRE(acc_t_mul.compute_w_ops() > 0, "W_ops for multiplication must be non-zero");
    SCG08_REQUIRE(acc_t_mul.diagnostic_convolution_ops > 0,
                  "Diagnostic convolution ops must be tracked");
    SCG08_REQUIRE(acc_t_mul.diagnostic_reduction_ops > 0,
                  "Diagnostic reduction ops must be tracked");
    SCG08_REQUIRE(acc_t_mul.norm_alu_add_sub > 0, "Normalization primitives must be tracked");
  }
}

void test_tier2_all_bands() {
  std::cout << "[SCG-08 GATE 1] Testing Tier 2 multi-precision arithmetic across "
               "all 5 bands...\n";
  // B=64:   Ternary(2, 2, 3),    Binary(1, 2, 2)
  run_band_verification<2, 2, 3, 1, 2, 2>(64);
  // B=128:  Ternary(3, 4, 6),    Binary(2, 3, 4)
  run_band_verification<3, 4, 6, 2, 3, 4>(128);
  // B=256:  Ternary(6, 7, 12),   Binary(4, 5, 8)
  run_band_verification<6, 7, 12, 4, 5, 8>(256);
  // B=512:  Ternary(12, 12, 24), Binary(8, 9, 16)
  run_band_verification<12, 12, 24, 8, 9, 16>(512);
  // B=1024: Ternary(24, 24, 48), Binary(16, 17, 32)
  run_band_verification<24, 24, 48, 16, 17, 32>(1024);
}

// ============================================================================
// Test 4: Deterministic Exact-Count Assertions (Zero, Sparse, Dense, Carry-Propagating)
// ============================================================================
void test_deterministic_exact_counts() {
  std::cout << "[SCG-08 GATE 1] Verifying deterministic exact operation counts "
               "(zero, sparse, dense, carry-propagating)...\n";

  // Case A: Zero Operands (0 + 0, 0 * 0)
  ExactBigIntOracle v_zero(0);
  auto t_zero = TernaryMultiLimb<2>::from_oracle(v_zero);
  auto b_zero = BinaryMultiLimb<1>::from_oracle(v_zero);

  OperationAccounting acc_t_add_0, acc_t_mul_0, acc_b_add_0, acc_b_mul_0;
  ArithmeticWorkHarness::add_ternary<2, 2>(t_zero, t_zero, acc_t_add_0);
  ArithmeticWorkHarness::mul_ternary<2, 3>(t_zero, t_zero, acc_t_mul_0);
  ArithmeticWorkHarness::add_binary<1, 2>(b_zero, b_zero, acc_b_add_0);
  ArithmeticWorkHarness::mul_binary<1, 2>(b_zero, b_zero, acc_b_mul_0);

  // Ternary Zero Add: 2 limb iterations, each addc() performs 38 lookups, 0 carry-ins, 2 branch
  // checks
  SCG08_REQUIRE(acc_t_add_0.op_alu_add_sub == 2, "Ternary zero add ALU count mismatch");
  SCG08_REQUIRE(acc_t_add_0.op_table_lookup == 76,
                "Ternary zero add table lookup count mismatch (2 * 38)");
  SCG08_REQUIRE(acc_t_add_0.op_branch_cond == 2, "Ternary zero add branch evaluation mismatch");
  SCG08_REQUIRE(acc_t_add_0.compute_w_ops() == 80, "Ternary zero add W_ops mismatch");

  // Binary Zero Add: 1 limb addition + 1 accumulator zeroing, 2 branch checks
  SCG08_REQUIRE(acc_b_add_0.op_alu_add_sub == 2, "Binary zero add ALU count mismatch");
  SCG08_REQUIRE(acc_b_add_0.op_branch_cond == 2, "Binary zero add branch count mismatch");
  SCG08_REQUIRE(acc_b_add_0.compute_w_ops() == 4, "Binary zero add W_ops mismatch");

  // Ternary Zero Mul: 54 outer loop ai==0 checks, 0 inner multiplies, 1 norm pass
  SCG08_REQUIRE(acc_t_mul_0.op_alu_mul == 0, "Ternary zero mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_0.op_branch_cond == 54, "Ternary zero mul branch evaluation mismatch");
  SCG08_REQUIRE(acc_t_mul_0.op_norm_pass == 1, "Ternary zero mul norm pass mismatch");
  SCG08_REQUIRE(acc_t_mul_0.compute_w_ops() == 55, "Ternary zero mul W_ops mismatch");

  // Binary Zero Mul: 1 outer limb ai==0 check, 0 multiplies
  SCG08_REQUIRE(acc_b_mul_0.op_alu_mul == 0, "Binary zero mul ALU mul mismatch");
  SCG08_REQUIRE(acc_b_mul_0.op_branch_cond == 1, "Binary zero mul branch count mismatch");
  SCG08_REQUIRE(acc_b_mul_0.compute_w_ops() == 1, "Binary zero mul W_ops mismatch");

  // Case B: Sparse Operands (1 + 0, 1 * 1)
  ExactBigIntOracle v_one(1);
  auto t_one = TernaryMultiLimb<2>::from_oracle(v_one);
  auto b_one = BinaryMultiLimb<1>::from_oracle(v_one);

  OperationAccounting acc_t_add_sp, acc_t_mul_sp, acc_b_add_sp, acc_b_mul_sp;
  ArithmeticWorkHarness::add_ternary<2, 2>(t_one, t_zero, acc_t_add_sp);
  ArithmeticWorkHarness::mul_ternary<2, 3>(t_one, t_one, acc_t_mul_sp);
  ArithmeticWorkHarness::add_binary<1, 2>(b_one, b_zero, acc_b_add_sp);
  ArithmeticWorkHarness::mul_binary<1, 2>(b_one, b_one, acc_b_mul_sp);

  SCG08_REQUIRE(acc_t_add_sp.op_table_lookup == 76, "Ternary sparse add table lookup mismatch");
  SCG08_REQUIRE(acc_t_add_sp.compute_w_ops() == 80, "Ternary sparse add W_ops mismatch");
  SCG08_REQUIRE(acc_b_add_sp.compute_w_ops() == 4, "Binary sparse add W_ops mismatch");

  // Ternary Sparse Mul (1 * 1): single non-zero trit in a and b
  SCG08_REQUIRE(acc_t_mul_sp.op_alu_mul == 1, "Ternary sparse mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.op_alu_add_sub == 1, "Ternary sparse mul ALU add mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.op_branch_cond == 109, "Ternary sparse mul branch count mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.compute_w_ops() == 114, "Ternary sparse mul W_ops mismatch");

  // Binary Sparse Mul (1 * 1):
  SCG08_REQUIRE(acc_b_mul_sp.op_alu_mul == 1, "Binary sparse mul ALU mul mismatch");
  SCG08_REQUIRE(acc_b_mul_sp.compute_w_ops() == 8, "Binary sparse mul W_ops mismatch");

  // Case C: Dense Operands (val_max + val_max, val_max * val_max)
  ExactBigIntOracle v_max = ExactBigIntOracle::from_power_of_two(63);
  v_max = ExactBigIntOracle::sub(v_max, ExactBigIntOracle(1));
  auto t_max = TernaryMultiLimb<2>::from_oracle(v_max);
  auto b_max = BinaryMultiLimb<1>::from_oracle(v_max);

  OperationAccounting acc_t_add_dense, acc_t_mul_dense, acc_b_add_dense, acc_b_mul_dense;
  ArithmeticWorkHarness::add_ternary<2, 2>(t_max, t_max, acc_t_add_dense);
  ArithmeticWorkHarness::mul_ternary<2, 3>(t_max, t_max, acc_t_mul_dense);
  ArithmeticWorkHarness::add_binary<1, 2>(b_max, b_max, acc_b_add_dense);
  ArithmeticWorkHarness::mul_binary<1, 2>(b_max, b_max, acc_b_mul_dense);

  SCG08_REQUIRE(acc_t_add_dense.compute_w_ops() == 80, "Ternary dense add W_ops mismatch");
  SCG08_REQUIRE(acc_b_add_dense.compute_w_ops() == 4, "Binary dense add W_ops mismatch");
  SCG08_REQUIRE(acc_t_mul_dense.op_alu_mul == 841, "Ternary dense mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_dense.compute_w_ops() == 5826, "Ternary dense mul W_ops mismatch");
  SCG08_REQUIRE(acc_b_mul_dense.compute_w_ops() == 9, "Binary dense mul W_ops mismatch");

  // Case D: Carry-Propagating Operands
  // In balanced ternary, limb 0 max is 27 trits of 1. Adding 1 generates a carry-out.
  // Limb 1 must perform a nested addc(c_limb), executing a 3rd addc() (3 * 38 = 114 table lookups).
  std::vector<int8_t> trits_cprop(54, 0);
  for (size_t i = 0; i < 27; ++i) trits_cprop[i] = 1;
  auto t_cprop_a = TernaryMultiLimb<2>::from_trits(trits_cprop);
  auto t_cprop_b = t_one;

  OperationAccounting acc_t_add_cprop;
  ArithmeticWorkHarness::add_ternary<2, 2>(t_cprop_a, t_cprop_b, acc_t_add_cprop);

  SCG08_REQUIRE(acc_t_add_cprop.op_alu_add_sub == 3, "Ternary carry-prop add ALU count mismatch");
  SCG08_REQUIRE(acc_t_add_cprop.op_table_lookup == 114,
                "Ternary carry-prop table lookup mismatch (3 * 38)");
  SCG08_REQUIRE(acc_t_add_cprop.op_branch_cond == 2, "Ternary carry-prop branch count mismatch");
  SCG08_REQUIRE(acc_t_add_cprop.compute_w_ops() == 119, "Ternary carry-prop W_ops mismatch");

  // In binary, B=128: limb 0 = 0xFFFFFFFFFFFFFFFF, adding 1 propagates carry to limb 1
  BinaryMultiLimb<2> b_cprop_a;
  b_cprop_a.limbs[0] = 0xFFFFFFFFFFFFFFFFULL;
  b_cprop_a.limbs[1] = 0;
  BinaryMultiLimb<2> b_cprop_b;
  b_cprop_b.limbs[0] = 1;
  b_cprop_b.limbs[1] = 0;

  OperationAccounting acc_b_add_cprop;
  ArithmeticWorkHarness::add_binary<2, 3>(b_cprop_a, b_cprop_b, acc_b_add_cprop);

  SCG08_REQUIRE(acc_b_add_cprop.op_alu_add_sub == 3, "Binary carry-prop add ALU count mismatch");
  SCG08_REQUIRE(acc_b_add_cprop.op_branch_cond == 3, "Binary carry-prop branch count mismatch");
  SCG08_REQUIRE(acc_b_add_cprop.compute_w_ops() == 6, "Binary carry-prop W_ops mismatch");

  std::cout << "  Deterministic exact-count verifications passed.\n";
}

// ============================================================================
// Test 5: Operation Accounting and Metric Formula Reconciliation
// ============================================================================
void test_operation_accounting_formula() {
  std::cout << "[SCG-08 GATE 1] Verifying frozen weighted metric W_ops "
               "formula...\n";

  OperationAccounting acc;
  acc.op_alu_add_sub = 10;
  acc.op_alu_mul = 5;
  acc.op_table_lookup = 7;
  acc.op_branch_cond = 3;
  acc.op_norm_pass = 2;

  acc.diagnostic_convolution_ops = 100;
  acc.diagnostic_reduction_ops = 50;
  acc.norm_alu_add_sub = 200;

  uint64_t w_ops = acc.compute_w_ops();
  SCG08_REQUIRE(w_ops == 37, "W_ops formula calculation mismatch");

  acc.diagnostic_convolution_ops += 999;
  acc.norm_alu_add_sub += 999;
  SCG08_REQUIRE(acc.compute_w_ops() == 37,
                "Diagnostic counters must NOT aggregate into frozen W_ops metric");
}

// ============================================================================
// Test 5: Cumulative Trial Budget Enforcement
// ============================================================================
void test_trial_budget_enforcement() {
  std::cout << "[SCG-08 GATE 1] Verifying cumulative trial budget "
               "accounting...\n";

  size_t current_trials = TrialBudgetEnforcer::instance().get_cumulative_trials();
  std::cout << "  Cumulative trials recorded so far: " << current_trials << " / "
            << TrialBudgetEnforcer::GATE1_MAX_CUMULATIVE_TRIALS << "\n";

  SCG08_REQUIRE(current_trials <= TrialBudgetEnforcer::GATE1_MAX_CUMULATIVE_TRIALS,
                "Gate 1 cumulative trial budget exceeded during test execution!");
}

// ============================================================================
// Test 6: Deterministic Seed Replay Invariance
// ============================================================================
void test_deterministic_replay() {
  std::cout << "[SCG-08 GATE 1] Verifying PRNG seed 0x5C608001ULL deterministic "
               "replay...\n";

  auto res1 = Tier1MarkovHarness::simulate_ternary(5, 10, 0x5C608001ULL);
  auto res2 = Tier1MarkovHarness::simulate_ternary(5, 10, 0x5C608001ULL);

  SCG08_REQUIRE(res1.f_gen_given_0() == res2.f_gen_given_0(), "PRNG replay mismatch");
  SCG08_REQUIRE(res1.f_occ() == res2.f_occ(), "PRNG replay mismatch");
  SCG08_REQUIRE(res1.mean_run_length() == res2.mean_run_length(), "PRNG replay mismatch");
}

// ============================================================================
// Test 7: Fail-Closed Negative Tests for Oracle Mismatch and Budget Exceeded
// ============================================================================
void test_fail_closed_negative_cases() {
  std::cout << "[SCG-08 GATE 1] Running negative tests: deliberate oracle mismatch "
               "and budget violation fail closed...\n";

  pid_t pid1 = fork();
  SCG08_REQUIRE(pid1 >= 0, "Fork failed for oracle mismatch negative test");
  if (pid1 == 0) {
    enforce_oracle_parity(false, "Deliberate negative test mismatch");
    std::exit(0);
  }
  int status1 = 0;
  waitpid(pid1, &status1, 0);
  SCG08_REQUIRE(WIFEXITED(status1) && WEXITSTATUS(status1) == 1,
                "Deliberate oracle mismatch must exit fail-closed with code 1");

  pid_t pid2 = fork();
  SCG08_REQUIRE(pid2 >= 0, "Fork failed for trial budget negative test");
  if (pid2 == 0) {
    TrialBudgetEnforcer::instance().reset_for_test(1000);
    TrialBudgetEnforcer::instance().record_trial();
    std::exit(0);
  }
  int status2 = 0;
  waitpid(pid2, &status2, 0);
  SCG08_REQUIRE(WIFEXITED(status2) && WEXITSTATUS(status2) == 1,
                "Cumulative trial budget exceeded must exit fail-closed with code 1");

  std::cout << "  Negative tests passed: both oracle mismatch and budget overflow fail closed.\n";
}

}  // namespace scg08

int main() {
  std::cout << "==============================================================="
               "======\n";
  std::cout << " SCG-08 Phase 1 Gate 1: Carry Propagation & Arithmetic Work "
               "Harness \n";
  std::cout << " Specification: T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06      "
               "     \n";
  std::cout << " Parent Charter: SCG08_PHASE0_RESEARCH_CHARTER.md             "
               "     \n";
  std::cout << "==============================================================="
               "======\n";

  scg08::test_accumulator_width_specification();
  scg08::test_tier1_markov_dynamics();
  scg08::test_tier2_all_bands();
  scg08::test_deterministic_exact_counts();
  scg08::test_operation_accounting_formula();
  scg08::test_deterministic_replay();
  scg08::test_fail_closed_negative_cases();
  scg08::test_trial_budget_enforcement();

  std::cout << "==============================================================="
               "======\n";
  std::cout << " [GATE 1 RESULT] ALL SMOKE-TESTS AND VERIFICATIONS PASSED      "
               "     \n";
  std::cout << " Cumulative Trials: "
            << scg08::TrialBudgetEnforcer::instance().get_cumulative_trials() << " / "
            << scg08::TrialBudgetEnforcer::GATE1_MAX_CUMULATIVE_TRIALS << " (Budget Respected)\n";
  std::cout << " Gate 2 Status: STRICTLY WITHHELD                               "
               "     \n";
  std::cout << "==============================================================="
               "======\n";

  return 0;
}
