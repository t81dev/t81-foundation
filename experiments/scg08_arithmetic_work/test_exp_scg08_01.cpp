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

  const auto& spec = get_accumulator_spec(bit_width);
  ExactBigIntOracle val_zero(0);
  ExactBigIntOracle val_one(1);
  ExactBigIntOracle val_neg_one(-1);

  ExactBigIntOracle val_max = ExactBigIntOracle::from_power_of_two(bit_width - 1);
  val_max = ExactBigIntOracle::sub(val_max, ExactBigIntOracle(1));

  ExactBigIntOracle val_min = ExactBigIntOracle::from_power_of_two(bit_width - 1);
  val_min.negative = true;

  struct BandTestCase {
    ExactBigIntOracle oa;
    ExactBigIntOracle ob;
    bool expected_sum_overflow;
    bool expected_prod_overflow;
  };

  std::vector<BandTestCase> test_cases = {
      {val_zero, val_zero, false, false},
      {val_one, val_one, false, false},
      {val_one, val_neg_one, false, false},
      {val_max, val_min, false, true},
      {val_max, val_max, true, true},
      {val_min, val_min, true, true},
      {val_min, val_one, false, false},
  };

  for (size_t case_idx = 0; case_idx < test_cases.size(); ++case_idx) {
    const auto& tc = test_cases[case_idx];
    const auto& oa = tc.oa;
    const auto& ob = tc.ob;

    // Convert to Ternary and Binary representations
    auto ta = TernaryMultiLimb<T_In>::from_oracle(oa);
    auto tb = TernaryMultiLimb<T_In>::from_oracle(ob);

    auto ba = BinaryMultiLimb<B_In>::from_oracle(oa);
    auto bb = BinaryMultiLimb<B_In>::from_oracle(ob);

    // Operation Accounting
    OperationAccounting acc_t_add;
    OperationAccounting acc_t_mul;
    OperationAccounting acc_b_add;
    OperationAccounting acc_b_mul;

    // Execute Ternary Addition & Multiplication
    auto t_sum = ArithmeticWorkHarness::add_ternary<T_In, T_Add>(ta, tb, acc_t_add);
    auto t_prod = ArithmeticWorkHarness::mul_ternary<T_In, T_Mul>(ta, tb, acc_t_mul);

    // Execute Binary Addition & Multiplication
    auto b_sum = ArithmeticWorkHarness::add_binary<B_In, B_Add>(ba, bb, acc_b_add);
    auto b_prod = ArithmeticWorkHarness::mul_binary<B_In, B_Mul>(ba, bb, acc_b_mul);

    // Independent Oracle Expectations
    auto expected_oa_sum = ExactBigIntOracle::add(oa, ob);
    auto expected_oa_prod = ExactBigIntOracle::mul(oa, ob);

    // Populate all 5 fields of OracleValidationReport across inputs and results
    // using the object-level evaluator that checks stored trytes before decoding trits
    OracleValidationReport oracle_rep =
        evaluate_oracle_validation_report<T_In, T_Add, T_Mul, B_In, B_Add, B_Mul>(
            bit_width, oa, ob, ta, tb, t_sum, t_prod, ba, bb, b_sum, b_prod, expected_oa_sum,
            expected_oa_prod, tc.expected_sum_overflow, tc.expected_prod_overflow);

    enforce_oracle_parity(oracle_rep.numerical_equality,
                          "Dimension 1 (numerical_equality) failed");
    enforce_oracle_parity(oracle_rep.overflow_classification,
                          "Dimension 2 (overflow_classification) failed");
    enforce_oracle_parity(oracle_rep.canonical_digits,
                          "Dimension 3 (canonical_digits) failed");
    enforce_oracle_parity(oracle_rep.sign_handling,
                          "Dimension 4 (sign_handling) failed");
    enforce_oracle_parity(oracle_rep.absence_of_truncation,
                          "Dimension 5 (absence_of_truncation) failed");
    enforce_oracle_parity(oracle_rep.all_passed(),
                          "OracleValidationReport::all_passed() acceptance condition failed");

    auto t_sum_trits = t_sum.to_trits();

    // Verify operation accounting invariants and deterministic counter formulas
    SCG08_REQUIRE(acc_t_add.compute_w_ops() > 0, "W_ops for addition must be non-zero");
    SCG08_REQUIRE(acc_t_mul.compute_w_ops() > 0, "W_ops for multiplication must be non-zero");
    SCG08_REQUIRE(acc_t_mul.diagnostic_convolution_ops == 1,
                  "Diagnostic convolution ops must be 1 per mul_ternary call");
    SCG08_REQUIRE(acc_t_mul.diagnostic_reduction_ops == 1,
                  "Diagnostic reduction ops must be 1 per mul_ternary call");
    SCG08_REQUIRE(acc_t_mul.norm_alu_add_sub >= 4 * T_Mul * 27,
                  "Normalization primitives must be tracked");
    SCG08_REQUIRE(acc_t_mul.norm_branch_cond == T_Mul * 27,
                  "Normalization branch primitives must equal ACCUM_TRITS");

    // Verify ternary conversion diagnostics
    SCG08_REQUIRE(acc_t_mul.conv_limb_unpacks == 2 * T_In,
                  "mul_ternary must record 2 * T_In limb unpacks");
    SCG08_REQUIRE(acc_t_mul.conv_limb_repacks == T_Mul + 2,
                  "mul_ternary must record T_Mul + 2 limb repacks (2 unit_limb + T_Mul output)");
    SCG08_REQUIRE(acc_t_mul.conv_tryte_decodes == 72 * T_In,
                  "mul_ternary must record 72 * T_In tryte decodes");
    SCG08_REQUIRE(acc_t_mul.conv_tryte_encodes == 18 + 36 * T_In + 9 * T_Mul,
                  "mul_ternary must record exact tryte encodes");

    // Verify add_ternary deterministic counter relationship:
    // op_table_lookup = 38 * (T_In + K_cin), op_alu_add_sub = T_In + 2 * K_cin, op_branch_cond = T_Add
    SCG08_REQUIRE(acc_t_add.op_table_lookup % 38 == 0,
                  "Ternary add table lookups must be a multiple of 38");
    uint64_t k_cin = (acc_t_add.op_table_lookup / 38) - T_In;
    SCG08_REQUIRE(acc_t_add.op_alu_add_sub == T_In + 2 * k_cin,
                  "Ternary add op_alu_add_sub must equal T_In + 2*K_cin (including c1+c2, excluding extension copy)");
    SCG08_REQUIRE(acc_t_add.op_branch_cond == T_Add,
                  "Ternary add op_branch_cond must equal OutLimbs");
    uint64_t ext_carry = (T_Add > T_In && t_sum_trits[T_In * 27] != 0) ? 1 : 0;
    SCG08_REQUIRE(acc_t_add.conv_limb_repacks == k_cin + ext_carry,
                  "Ternary add conv_limb_repacks must equal K_cin + C_ext");
    SCG08_REQUIRE(acc_t_add.conv_tryte_encodes == 9 * (k_cin + ext_carry),
                  "Ternary add conv_tryte_encodes must equal 9 * (K_cin + C_ext)");

    // At B=128 (T_In=3, T_Add=4) and B=256 (T_In=6, T_Add=7), val_max+val_max and val_min+val_min
    // require 82 and 163 trits respectively, propagating a carry into the extension limb (i = T_In).
    if ((bit_width == 128 || bit_width == 256) && (case_idx == 4 || case_idx == 5)) {
      SCG08_REQUIRE(ext_carry == 1,
                    "Extension limb carry must be active for extreme sums at B=128 and B=256");
    }

    // Verify add_binary deterministic counter relationships across same-sign and opposite-sign cases
    if (oa.negative == ob.negative) {
      SCG08_REQUIRE(acc_b_add.op_alu_add_sub == 2 * B_In,
                    "Same-sign binary add must count 2 additions (a+b and +carry) per input limb and 0 for extension copy");
      SCG08_REQUIRE(acc_b_add.op_branch_cond == 1 + 2 * B_In,
                    "Same-sign binary add must count 1 sign check + 2 carry checks per input limb");
      SCG08_REQUIRE(acc_b_add.compute_w_ops() == 4 * B_In + 1,
                    "Same-sign binary add W_ops must equal 4 * B_In + 1");
    } else if (case_idx == 2) {
      // {val_one, val_neg_one}: equal magnitude opposite-sign subtraction (c == 0)
      SCG08_REQUIRE(acc_b_add.op_alu_add_sub == 0,
                    "Equal-magnitude opposite-sign binary add performs 0 limb subtractions");
      SCG08_REQUIRE(acc_b_add.op_branch_cond == B_In + 2,
                    "Equal-magnitude opposite-sign binary add evaluates B_In cmp checks + sign + c==0");
    } else {
      // {val_max, val_min} or {val_min, val_one}: unequal magnitude opposite-sign subtraction
      SCG08_REQUIRE(acc_b_add.op_alu_add_sub == 2 * B_In,
                    "Unequal-magnitude opposite-sign binary add performs 2 add/sub ops per input limb");
    }
    (void)spec;
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

  // Ternary Zero Add (InLimbs=2, OutLimbs=2, K_cin=0):
  // 2 primary addc() calls (2 * 38 = 76 lookups), 0 carry-ins, 2 carry_in!=0 branch evaluations
  SCG08_REQUIRE(acc_t_add_0.op_alu_add_sub == 2, "Ternary zero add ALU count mismatch");
  SCG08_REQUIRE(acc_t_add_0.op_table_lookup == 76,
                "Ternary zero add table lookup count mismatch (2 * 38)");
  SCG08_REQUIRE(acc_t_add_0.op_branch_cond == 2, "Ternary zero add branch evaluation mismatch");
  SCG08_REQUIRE(acc_t_add_0.compute_w_ops() == 80, "Ternary zero add W_ops mismatch");
  SCG08_REQUIRE(acc_t_add_0.conv_limb_repacks == 0, "Ternary zero add conv_limb_repacks mismatch");

  // Binary Zero Add (InLimbs=1, OutLimbs=2):
  // 2 primitive additions on limb 0 (a[0]+b[0] and +carry), 0 for extension-limb copy,
  // 3 branch evaluations (1 sign check + 1 carry!=0 check + 1 carry predicate sum<a[0])
  SCG08_REQUIRE(acc_b_add_0.op_alu_add_sub == 2, "Binary zero add ALU count mismatch");
  SCG08_REQUIRE(acc_b_add_0.op_branch_cond == 3, "Binary zero add branch count mismatch");
  SCG08_REQUIRE(acc_b_add_0.compute_w_ops() == 5, "Binary zero add W_ops mismatch");

  // Ternary Zero Mul (InLimbs=2, OutLimbs=3):
  // 54 outer loop ai==0 checks, 0 inner multiplies, 1 norm pass
  // conv_limb_repacks = OutLimbs + 2 = 3 + 2 = 5 (2 unit_limb in a/b.to_trits() + 3 output limbs)
  SCG08_REQUIRE(acc_t_mul_0.op_alu_mul == 0, "Ternary zero mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_0.op_branch_cond == 54, "Ternary zero mul branch evaluation mismatch");
  SCG08_REQUIRE(acc_t_mul_0.op_norm_pass == 1, "Ternary zero mul norm pass mismatch");
  SCG08_REQUIRE(acc_t_mul_0.compute_w_ops() == 55, "Ternary zero mul W_ops mismatch");
  SCG08_REQUIRE(acc_t_mul_0.conv_limb_unpacks == 4, "Ternary zero mul conv_limb_unpacks mismatch");
  SCG08_REQUIRE(acc_t_mul_0.conv_limb_repacks == 5,
                "Ternary zero mul conv_limb_repacks mismatch (OutLimbs + 2 = 5)");

  // Binary Zero Mul (InLimbs=1, OutLimbs=2):
  // 1 outer limb ai==0 check, 2 zero-detection limb checks, 1 all_zero check, 0 multiplies
  SCG08_REQUIRE(acc_b_mul_0.op_alu_mul == 0, "Binary zero mul ALU mul mismatch");
  SCG08_REQUIRE(acc_b_mul_0.op_branch_cond == 4,
                "Binary zero mul branch count mismatch (1 outer + 2 zero-detect + 1 all_zero)");
  SCG08_REQUIRE(acc_b_mul_0.compute_w_ops() == 4, "Binary zero mul W_ops mismatch");

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
  SCG08_REQUIRE(acc_b_add_sp.op_alu_add_sub == 2, "Binary sparse add ALU count mismatch");
  SCG08_REQUIRE(acc_b_add_sp.op_branch_cond == 3, "Binary sparse add branch count mismatch");
  SCG08_REQUIRE(acc_b_add_sp.compute_w_ops() == 5, "Binary sparse add W_ops mismatch");

  // Ternary Sparse Mul (1 * 1): N_a = 1, N_b = 1 -> 1 mul, 1 add, 54*(1+1)+1 = 109 branches, 1 norm pass
  SCG08_REQUIRE(acc_t_mul_sp.op_alu_mul == 1, "Ternary sparse mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.op_alu_add_sub == 1, "Ternary sparse mul ALU add mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.op_branch_cond == 109, "Ternary sparse mul branch count mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.compute_w_ops() == 114, "Ternary sparse mul W_ops mismatch");
  SCG08_REQUIRE(acc_t_mul_sp.conv_limb_repacks == 5,
                "Ternary sparse mul conv_limb_repacks mismatch (OutLimbs + 2 = 5)");

  // Binary Sparse Mul (1 * 1):
  // 4 primitive mul64 muls (4 * 3 = 12), 4 mul64 helper adds + 4 caller accumulator adds +
  // 0 carry-loop adds (carry == 0 breaks immediately) = 8 adds total, 7 branches
  SCG08_REQUIRE(acc_b_mul_sp.op_alu_mul == 4,
                "Binary sparse mul ALU mul mismatch (4 primitive 32x32 muls)");
  SCG08_REQUIRE(acc_b_mul_sp.op_alu_add_sub == 8,
                "Binary sparse mul ALU add mismatch (4 in mul64 + 4 accumulator + 0 carry loop)");
  SCG08_REQUIRE(acc_b_mul_sp.op_branch_cond == 7, "Binary sparse mul branch count mismatch");
  SCG08_REQUIRE(acc_b_mul_sp.compute_w_ops() == 27,
                "Binary sparse mul W_ops mismatch (8 add + 3*4 mul + 7 branch)");

  // Case C: Dense Operands (val_max + val_max, val_max * val_max at B=64)
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
  SCG08_REQUIRE(acc_b_add_dense.op_alu_add_sub == 2, "Binary dense add ALU count mismatch");
  SCG08_REQUIRE(acc_b_add_dense.op_branch_cond == 3, "Binary dense add branch count mismatch");
  SCG08_REQUIRE(acc_b_add_dense.compute_w_ops() == 5, "Binary dense add W_ops mismatch");
  SCG08_REQUIRE(acc_t_mul_dense.op_alu_mul == 841, "Ternary dense mul ALU mul mismatch");
  SCG08_REQUIRE(acc_t_mul_dense.compute_w_ops() == 5826, "Ternary dense mul W_ops mismatch");
  SCG08_REQUIRE(acc_t_mul_dense.conv_limb_repacks == 5,
                "Ternary dense mul conv_limb_repacks mismatch (OutLimbs + 2 = 5)");
  SCG08_REQUIRE(acc_b_mul_dense.op_alu_mul == 4, "Binary dense mul ALU mul mismatch");
  SCG08_REQUIRE(acc_b_mul_dense.op_alu_add_sub == 9,
                "Binary dense mul ALU add mismatch (4 in mul64 + 4 accumulator + 1 carry loop)");
  SCG08_REQUIRE(acc_b_mul_dense.op_branch_cond == 8, "Binary dense mul branch count mismatch");
  SCG08_REQUIRE(acc_b_mul_dense.compute_w_ops() == 29,
                "Binary dense mul W_ops mismatch (9 add + 3*4 mul + 8 branch)");

  // Case D: Carry-Propagating Operands
  // In balanced ternary, limb 0 max is 27 trits of 1. Adding 1 generates a carry-out c1 = +1.
  // Limb 1 (K_cin = 1) performs a nested addc(c_limb) (3 * 38 = 114 table lookups) and combines
  // c1 + c2 (+1 scalar addition), giving op_alu_add_sub = 2 + 1 + 1 = 4 and W_ops = 4 + 114 + 2 = 120.
  std::vector<int8_t> trits_cprop(54, 0);
  for (size_t i = 0; i < 27; ++i) trits_cprop[i] = 1;
  auto t_cprop_a = TernaryMultiLimb<2>::from_trits(trits_cprop);
  auto t_cprop_b = t_one;

  OperationAccounting acc_t_add_cprop;
  ArithmeticWorkHarness::add_ternary<2, 2>(t_cprop_a, t_cprop_b, acc_t_add_cprop);

  SCG08_REQUIRE(acc_t_add_cprop.op_alu_add_sub == 4,
                "Ternary carry-prop add ALU count mismatch (2 primary addc + 1 carry addc + 1 c1+c2)");
  SCG08_REQUIRE(acc_t_add_cprop.op_table_lookup == 114,
                "Ternary carry-prop table lookup mismatch (3 * 38)");
  SCG08_REQUIRE(acc_t_add_cprop.op_branch_cond == 2, "Ternary carry-prop branch count mismatch");
  SCG08_REQUIRE(acc_t_add_cprop.compute_w_ops() == 120, "Ternary carry-prop W_ops mismatch");
  SCG08_REQUIRE(acc_t_add_cprop.conv_limb_repacks == 1,
                "Ternary carry-prop must record 1 carry-limb repack");
  SCG08_REQUIRE(acc_t_add_cprop.conv_tryte_encodes == 9,
                "Ternary carry-prop must record 9 tryte encodes");

  // In binary, B=128 (InLimbs=2, OutLimbs=3): limb 0 = 0xFFFFFFFFFFFFFFFF, adding 1 propagates carry to limb 1.
  // Each input limb executes 2 additions (a+b and +carry) and 2 branch checks (carry!=0 and carry predicate),
  // plus 1 sign check at entry; extension limb 2 is a pure data copy (0 arithmetic ops).
  BinaryMultiLimb<2> b_cprop_a;
  b_cprop_a.limbs[0] = 0xFFFFFFFFFFFFFFFFULL;
  b_cprop_a.limbs[1] = 0;
  BinaryMultiLimb<2> b_cprop_b;
  b_cprop_b.limbs[0] = 1;
  b_cprop_b.limbs[1] = 0;

  OperationAccounting acc_b_add_cprop;
  ArithmeticWorkHarness::add_binary<2, 3>(b_cprop_a, b_cprop_b, acc_b_add_cprop);

  SCG08_REQUIRE(acc_b_add_cprop.op_alu_add_sub == 4,
                "Binary carry-prop add ALU count mismatch (2 per input limb, 0 for extension copy)");
  SCG08_REQUIRE(acc_b_add_cprop.op_branch_cond == 5,
                "Binary carry-prop branch count mismatch (1 sign + 2 per input limb)");
  SCG08_REQUIRE(acc_b_add_cprop.compute_w_ops() == 9, "Binary carry-prop W_ops mismatch");

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
  acc.conv_limb_unpacks = 40;
  acc.conv_limb_repacks = 30;
  acc.conv_tryte_decodes = 720;
  acc.conv_tryte_encodes = 540;

  uint64_t w_ops = acc.compute_w_ops();
  SCG08_REQUIRE(w_ops == 37, "W_ops formula calculation mismatch");

  acc.diagnostic_convolution_ops += 999;
  acc.norm_alu_add_sub += 999;
  acc.conv_limb_unpacks += 999;
  acc.conv_limb_repacks += 999;
  acc.conv_tryte_decodes += 999;
  acc.conv_tryte_encodes += 999;
  SCG08_REQUIRE(acc.compute_w_ops() == 37,
                "Diagnostic and conversion counters must NOT aggregate into W_ops metric");
}

// ============================================================================
// Test 6: Cumulative Trial Budget Enforcement
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
// Test 7: Deterministic Seed Replay Invariance
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

// Helper to verify that a callable in a child process exits with code 1 and emits the mandatory
// oracle-mismatch diagnostic on stderr
template <typename Fn>
static void expect_fail_closed_fn_in_child(Fn&& fn) {
  int pipefd[2] = {-1, -1};
  SCG08_REQUIRE(pipe(pipefd) == 0, "Pipe creation failed for negative oracle test");
  pid_t pid = fork();
  SCG08_REQUIRE(pid >= 0, "Fork failed for negative oracle test");
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);
    fn();
    std::exit(0);
  }
  close(pipefd[1]);
  std::string stderr_output;
  char buf[256];
  ssize_t n = 0;
  while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
    stderr_output.append(buf, static_cast<size_t>(n));
  }
  close(pipefd[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  SCG08_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 1,
                "Corrupted oracle validation case must exit fail-closed with code 1");
  SCG08_REQUIRE(
      stderr_output.find("INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)") !=
          std::string::npos,
      "Corrupted oracle validation case must emit mandatory oracle-mismatch diagnostic on stderr");
}

static void expect_fail_closed_in_child(bool condition, const char* context_msg) {
  expect_fail_closed_fn_in_child([&]() { enforce_oracle_parity(condition, context_msg); });
}

// ============================================================================
// Test 8: Fail-Closed Negative Tests Across All 5 Oracle Dimensions & Budget
// ============================================================================
void test_fail_closed_negative_cases() {
  std::cout << "[SCG-08 GATE 1] Running negative tests: deliberate 5-D oracle corruptions "
               "and budget violation fail closed...\n";

  // Construct valid baseline B=64 inputs and expected oracle outputs without consuming parent trial budget
  constexpr size_t B = 64;
  const auto& spec = get_accumulator_spec(B);
  ExactBigIntOracle val_max = ExactBigIntOracle::from_power_of_two(B - 1);
  val_max = ExactBigIntOracle::sub(val_max, ExactBigIntOracle(1));
  ExactBigIntOracle val_min = ExactBigIntOracle::from_power_of_two(B - 1);
  val_min.negative = true;
  ExactBigIntOracle expected_sum = ExactBigIntOracle::add(val_max, val_max);
  ExactBigIntOracle expected_prod = ExactBigIntOracle::mul(val_max, val_max);

  auto ta = TernaryMultiLimb<2>::from_oracle(val_max);
  auto tb = TernaryMultiLimb<2>::from_oracle(val_max);
  auto ta_trits = ta.to_trits();
  auto tb_trits = tb.to_trits();
  auto t_sum = TernaryMultiLimb<2>::from_oracle(expected_sum);
  auto t_sum_trits = t_sum.to_trits();
  auto t_prod = TernaryMultiLimb<3>::from_oracle(expected_prod);
  auto t_prod_trits = t_prod.to_trits();

  auto ba = BinaryMultiLimb<1>::from_oracle(val_max);
  auto bb = BinaryMultiLimb<1>::from_oracle(val_max);
  auto b_sum = BinaryMultiLimb<2>::from_oracle(expected_sum);
  auto b_prod = BinaryMultiLimb<2>::from_oracle(expected_prod);

  // Confirm uncorrupted baseline passes all 5 dimensions
  auto valid_rep = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
      B, val_max, val_max, ta, tb, t_sum, t_prod, ba, bb, b_sum, b_prod, expected_sum,
      expected_prod, true, true);
  SCG08_REQUIRE(valid_rep.all_passed(), "Baseline valid oracle report must pass all 5 dimensions");

  // 1. Negative Test — Dimension 1 (Numerical Equality Corruption): corrupt low limb of b_sum
  {
    auto b_sum_corrupt = b_sum;
    b_sum_corrupt.limbs[0] ^= 1ULL;
    auto rep = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod, t_prod_trits,
        ba, bb, b_sum_corrupt, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep.numerical_equality,
                  "Corrupted binary sum must fail numerical_equality");
    SCG08_REQUIRE(!rep.all_passed(), "Corrupted numerical_equality must fail all_passed()");
    expect_fail_closed_in_child(rep.all_passed(), "Deliberate numerical equality corruption");
  }

  // 2. Negative Test — Dimension 2 (Overflow Classification Corruption):
  // (a) Misclassify overflowing sum (val_max + val_max) as non-overflowing
  // (b) Supply out-of-domain input operand (2^63 > 2^63 - 1)
  // (c) Verify exact negative product bound -2^(B-1)*(2^(B-1)-1) is accepted while
  //     -2^(B-1)*(2^(B-1)-1) - 1 (which is still > -2^(2B-2)) is rejected
  {
    auto rep_misclass = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod, t_prod_trits,
        ba, bb, b_sum, b_prod, expected_sum, expected_prod, false, true);
    SCG08_REQUIRE(!rep_misclass.overflow_classification,
                  "Misclassified sum overflow must fail overflow_classification");
    SCG08_REQUIRE(!rep_misclass.all_passed(),
                  "Misclassified overflow must fail all_passed()");
    expect_fail_closed_in_child(rep_misclass.all_passed(),
                                "Deliberate overflow classification mismatch");

    ExactBigIntOracle out_of_domain = ExactBigIntOracle::from_power_of_two(B - 1);
    auto rep_ood = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, out_of_domain, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod,
        t_prod_trits, ba, bb, b_sum, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_ood.overflow_classification,
                  "Out-of-domain operand must fail overflow_classification");
    SCG08_REQUIRE(!rep_ood.all_passed(), "Out-of-domain operand must fail all_passed()");

    // Exact negative product bound: -2^(B-1) * (2^(B-1) - 1) = val_min * val_max
    ExactBigIntOracle exact_min_prod = ExactBigIntOracle::mul(val_min, val_max);
    ExactBigIntOracle sum_max_min = ExactBigIntOracle::add(val_max, val_min);
    auto tb_min = TernaryMultiLimb<2>::from_oracle(val_min);
    auto bb_min = BinaryMultiLimb<1>::from_oracle(val_min);
    auto t_sum_mm = TernaryMultiLimb<2>::from_oracle(sum_max_min);
    auto b_sum_mm = BinaryMultiLimb<2>::from_oracle(sum_max_min);
    auto t_prod_exact_min = TernaryMultiLimb<3>::from_oracle(exact_min_prod);
    auto b_prod_exact_min = BinaryMultiLimb<2>::from_oracle(exact_min_prod);

    auto rep_exact_min_prod = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_min, ta, tb_min, t_sum_mm, t_prod_exact_min, ba, bb_min, b_sum_mm,
        b_prod_exact_min, sum_max_min, exact_min_prod, false, true);
    SCG08_REQUIRE(rep_exact_min_prod.overflow_classification && rep_exact_min_prod.all_passed(),
                  "Exact negative product bound -2^(B-1)*(2^(B-1)-1) must pass validation");

    // 1 below exact negative product bound: -2^(B-1)*(2^(B-1)-1) - 1 (still > -2^(2B-2))
    ExactBigIntOracle below_min_prod =
        ExactBigIntOracle::sub(exact_min_prod, ExactBigIntOracle(1));
    auto t_prod_below_min = TernaryMultiLimb<3>::from_oracle(below_min_prod);
    auto b_prod_below_min = BinaryMultiLimb<2>::from_oracle(below_min_prod);
    auto rep_below_min_prod = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_min, ta, tb_min, t_sum_mm, t_prod_below_min, ba, bb_min, b_sum_mm,
        b_prod_below_min, sum_max_min, below_min_prod, false, true);
    SCG08_REQUIRE(!rep_below_min_prod.overflow_classification,
                  "Product below exact negative bound -2^(B-1)*(2^(B-1)-1) must fail overflow_classification");
    SCG08_REQUIRE(!rep_below_min_prod.all_passed(),
                  "Product below exact negative bound must fail all_passed()");
    expect_fail_closed_in_child(
        rep_below_min_prod.all_passed(),
        "Deliberate negative product bound violation below -2^(B-1)*(2^(B-1)-1)");
  }

  // 3. Negative Test — Dimension 3 (Canonical Output Digits & Stored Trytes Corruption):
  // (a) Inject non-canonical trit (+2) into output t_sum_trits
  // (b) Inject non-canonical tryte (+14) into output t_sum limb storage and exercise the
  //     actual corrupted-object verification and decoding paths in subprocesses without
  //     reaching TRYTE_TO_TRITS[14 + 13] or ADD_TABLE[14 + 13]
  {
    auto t_sum_trits_corrupt = t_sum_trits;
    t_sum_trits_corrupt[0] = 2;
    auto rep_trit = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits_corrupt, t_prod,
        t_prod_trits, ba, bb, b_sum, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_trit.canonical_digits,
                  "Non-canonical output trit (+2) must fail canonical_digits");
    SCG08_REQUIRE(!rep_trit.all_passed(),
                  "Non-canonical output trit must fail all_passed()");
    expect_fail_closed_in_child(rep_trit.all_passed(),
                                "Deliberate non-canonical output trit corruption");

    auto t_sum_tryte_corrupt = t_sum;
    int8_t bad_tryte = 14;
    std::memcpy(&t_sum_tryte_corrupt.limbs[0], &bad_tryte, 1);

    // Exercise the actual object-level verification path (NOT passing pre-extracted t_sum_trits)
    auto rep_tryte = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, tb, t_sum_tryte_corrupt, t_prod, ba, bb, b_sum, b_prod,
        expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_tryte.canonical_digits,
                  "Non-canonical stored tryte (+14) must fail canonical_digits");
    SCG08_REQUIRE(!rep_tryte.all_passed(),
                  "Non-canonical stored tryte must fail all_passed()");

    // Subprocess 3b-i: Full object-level verification path on corrupted TernaryMultiLimb (+14)
    expect_fail_closed_fn_in_child([&]() {
      auto rep = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
          B, val_max, val_max, ta, tb, t_sum_tryte_corrupt, t_prod, ba, bb, b_sum, b_prod,
          expected_sum, expected_prod, true, true);
      enforce_oracle_parity(rep.canonical_digits, "Dimension 3 (canonical_digits) failed");
      enforce_oracle_parity(rep.all_passed(), "OracleValidationReport::all_passed() failed");
    });

    // Subprocess 3b-ii: Direct to_trits() decoding on corrupted TernaryMultiLimb (+14)
    // Must reject before reaching limbs[l] * unit_limb -> TRYTE_TO_TRITS[14 + 13]
    expect_fail_closed_fn_in_child([&]() {
      auto decoded = t_sum_tryte_corrupt.to_trits();
      (void)decoded;
    });

    // Subprocess 3b-iii: Direct add_ternary() on corrupted TernaryMultiLimb (+14)
    // Must reject before reaching addc() -> ADD_TABLE[14 + 13]
    expect_fail_closed_fn_in_child([&]() {
      OperationAccounting acc_corrupt;
      auto res = ArithmeticWorkHarness::add_ternary<2, 2>(t_sum_tryte_corrupt, tb, acc_corrupt);
      (void)res;
    });

    // Subprocess 3b-iv: Direct mul_ternary() on corrupted TernaryMultiLimb (+14)
    // Must reject before reaching to_trits() -> TRYTE_TO_TRITS[14 + 13]
    expect_fail_closed_fn_in_child([&]() {
      OperationAccounting acc_corrupt;
      auto res = ArithmeticWorkHarness::mul_ternary<2, 3>(t_sum_tryte_corrupt, tb, acc_corrupt);
      (void)res;
    });
  }

  // 4. Negative Test — Dimension 4 (Sign Correctness Corruption):
  // (a) Flip binary output sign on positive sum
  // (b) Construct negative zero (-0) in binary output
  // (c) Negate all trits of ternary product output
  {
    auto b_sum_bad_sign = b_sum;
    b_sum_bad_sign.is_negative = true;
    auto rep_bsign = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod, t_prod_trits,
        ba, bb, b_sum_bad_sign, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_bsign.sign_handling,
                  "Inverted binary output sign must fail sign_handling");
    SCG08_REQUIRE(!rep_bsign.all_passed(), "Inverted binary sign must fail all_passed()");
    expect_fail_closed_in_child(rep_bsign.all_passed(), "Deliberate sign handling corruption");

    ExactBigIntOracle val_zero(0);
    auto b_zero_neg = BinaryMultiLimb<2>::from_oracle(val_zero);
    b_zero_neg.is_negative = true;
    SCG08_REQUIRE(!verify_binary_sign(b_zero_neg.limbs, b_zero_neg.is_negative, val_zero),
                  "Negative zero (-0) in binary limbs must be rejected by verify_binary_sign");

    auto t_prod_trits_neg = t_prod_trits;
    for (int8_t& d : t_prod_trits_neg) d = static_cast<int8_t>(-d);
    auto rep_tsign = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod,
        t_prod_trits_neg, ba, bb, b_sum, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_tsign.sign_handling,
                  "Negated ternary product trits must fail sign_handling");
    SCG08_REQUIRE(!rep_tsign.all_passed(), "Negated ternary product must fail all_passed()");
  }

  // 5. Negative Test — Dimension 5 (Truncation Corruption):
  // (a) Truncate high 64-bit limb of binary product (b_prod.limbs[1] = 0)
  // (b) Truncate high-order ternary trits (>= spec.operand_trits) of ternary product
  {
    auto b_prod_trunc = b_prod;
    b_prod_trunc.limbs[1] = 0;
    auto rep_btrunc = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod, t_prod_trits,
        ba, bb, b_sum, b_prod_trunc, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_btrunc.absence_of_truncation,
                  "Truncated high binary product limb must fail absence_of_truncation");
    SCG08_REQUIRE(!rep_btrunc.all_passed(), "Truncated binary product must fail all_passed()");
    expect_fail_closed_in_child(rep_btrunc.all_passed(), "Deliberate binary limb truncation");

    auto t_prod_trits_trunc = t_prod_trits;
    for (size_t i = spec.operand_trits; i < t_prod_trits_trunc.size(); ++i) {
      t_prod_trits_trunc[i] = 0;
    }
    auto rep_ttrunc = evaluate_oracle_validation_report<2, 2, 3, 1, 2, 2>(
        B, val_max, val_max, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod,
        t_prod_trits_trunc, ba, bb, b_sum, b_prod, expected_sum, expected_prod, true, true);
    SCG08_REQUIRE(!rep_ttrunc.absence_of_truncation,
                  "Truncated high ternary product trits must fail absence_of_truncation");
    SCG08_REQUIRE(!rep_ttrunc.all_passed(), "Truncated ternary product must fail all_passed()");
    expect_fail_closed_in_child(rep_ttrunc.all_passed(), "Deliberate ternary trit truncation");
  }

  // 6. Negative Test — Cumulative Trial Budget Violation:
  pid_t pid_budget = fork();
  SCG08_REQUIRE(pid_budget >= 0, "Fork failed for trial budget negative test");
  if (pid_budget == 0) {
    TrialBudgetEnforcer::instance().reset_for_test(1000);
    TrialBudgetEnforcer::instance().record_trial();
    std::exit(0);
  }
  int status_budget = 0;
  waitpid(pid_budget, &status_budget, 0);
  SCG08_REQUIRE(WIFEXITED(status_budget) && WEXITSTATUS(status_budget) == 1,
                "Cumulative trial budget exceeded must exit fail-closed with code 1");

  std::cout << "  Negative tests passed: all 5 oracle dimensions and budget overflow fail closed.\n";
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
