#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "exact_oracle_adapter.hpp"
#include "t81/types/T81Limb.hpp"

namespace scg08 {

// ============================================================================
// Tier 2: Arithmetic Work and Operation Instrumentation
//
// Stipulated Weighted Kernel Proxy:
// W_ops = OP_ALU_ADD_SUB + 3*OP_ALU_MUL + OP_TABLE_LOOKUP + OP_BRANCH_COND +
// OP_NORM_PASS
//
// Modeling Notice:
// - W_ops is a stipulated kernel-level event proxy, NOT an unbiased measure of
//   total computational work or physical hardware cost.
// - The coefficient 3 on OP_ALU_MUL is a stipulated modeling choice, not an
//   empirically universal hardware cost ratio.
//
// Non-aggregating diagnostic counters (excluded from W_ops):
// - Stage markers: diagnostic_convolution_ops, diagnostic_reduction_ops
// - Normalization internals: norm_alu_add_sub, norm_alu_mul, norm_table_lookup,
//   norm_branch_cond
// - Representation conversion / unpacking / repacking:
//   conv_limb_unpacks, conv_limb_repacks, conv_tryte_decodes, conv_tryte_encodes
// - Memory traffic: bytes_read, bytes_written, mem_loads, mem_stores
// ============================================================================

struct OperationAccounting {
  uint64_t op_alu_add_sub{0};
  uint64_t op_alu_mul{0};
  uint64_t op_table_lookup{0};
  uint64_t op_branch_cond{0};
  uint64_t op_norm_pass{0};

  uint64_t diagnostic_convolution_ops{0};
  uint64_t diagnostic_reduction_ops{0};

  uint64_t norm_alu_add_sub{0};
  uint64_t norm_alu_mul{0};
  uint64_t norm_table_lookup{0};
  uint64_t norm_branch_cond{0};

  uint64_t conv_limb_unpacks{0};
  uint64_t conv_limb_repacks{0};
  uint64_t conv_tryte_decodes{0};
  uint64_t conv_tryte_encodes{0};

  uint64_t bytes_read{0};
  uint64_t bytes_written{0};
  uint64_t mem_loads{0};
  uint64_t mem_stores{0};

  [[nodiscard]] uint64_t compute_w_ops() const noexcept {
    return op_alu_add_sub + (3 * op_alu_mul) + op_table_lookup + op_branch_cond + op_norm_pass;
  }

  void reset() noexcept { *this = OperationAccounting{}; }
};

class TrialBudgetEnforcer {
public:
  static constexpr size_t GATE1_MAX_CUMULATIVE_TRIALS = 1000;

private:
  size_t cumulative_trials_{0};

public:
  static TrialBudgetEnforcer& instance() noexcept {
    static TrialBudgetEnforcer instance_;
    return instance_;
  }

  void record_trial() {
    cumulative_trials_++;
    if (cumulative_trials_ > GATE1_MAX_CUMULATIVE_TRIALS) {
      std::cerr << "\nFATAL: GATE 1 CUMULATIVE TRIAL BUDGET EXCEEDED (" << cumulative_trials_
                << " > " << GATE1_MAX_CUMULATIVE_TRIALS
                << "). Terminating execution fail-closed.\n";
      std::exit(1);
    }
  }

  [[nodiscard]] size_t get_cumulative_trials() const noexcept { return cumulative_trials_; }

  void reset_for_test(size_t initial = 0) noexcept { cumulative_trials_ = initial; }
};

struct AccumulatorSpec {
  size_t bit_width;
  size_t operand_trits;
  size_t add_trits_min;
  size_t mul_trits_min;
  size_t binary_limbs;       // 64-bit limbs
  size_t ternary_limbs_op;   // 27-trit limbs for operand
  size_t ternary_limbs_add;  // 27-trit limbs for addition accumulator
  size_t ternary_limbs_mul;  // 27-trit limbs for multiplication accumulator
};

inline constexpr AccumulatorSpec SPEC_B64 = {64, 41, 42, 81, 1, 2, 2, 3};
inline constexpr AccumulatorSpec SPEC_B128 = {128, 81, 82, 161, 2, 3, 4, 6};
inline constexpr AccumulatorSpec SPEC_B256 = {256, 162, 163, 323, 4, 6, 7, 12};
inline constexpr AccumulatorSpec SPEC_B512 = {512, 324, 324, 646, 8, 12, 12, 24};
inline constexpr AccumulatorSpec SPEC_B1024 = {1024, 647, 647, 1292, 16, 24, 24, 48};

inline const AccumulatorSpec& get_accumulator_spec(size_t bit_width) {
  switch (bit_width) {
    case 64:
      return SPEC_B64;
    case 128:
      return SPEC_B128;
    case 256:
      return SPEC_B256;
    case 512:
      return SPEC_B512;
    case 1024:
      return SPEC_B1024;
    default:
      std::cerr << "Unsupported bit width: " << bit_width << "\n";
      std::exit(1);
  }
}

template <size_t NumLimbs>
struct TernaryMultiLimb {
  static constexpr size_t LIMBS = NumLimbs;
  static constexpr size_t TRITS = NumLimbs * 27;
  std::array<t81::core::T81Limb27, NumLimbs> limbs{};

  // Inspect the 9 stored trytes of each trivially copyable T81Limb27 without
  // modifying production headers, verifying each tryte is in [-13, +13].
  [[nodiscard]] bool has_canonical_trytes() const noexcept {
    static_assert(std::is_trivially_copyable_v<t81::core::T81Limb27>,
                  "T81Limb27 must be trivially copyable for tryte inspection");
    static_assert(sizeof(t81::core::T81Limb27) >= t81::core::T81Limb27::TRYTES,
                  "T81Limb27 size must cover TRYTES");
    for (size_t l = 0; l < NumLimbs; ++l) {
      std::array<int8_t, t81::core::T81Limb27::TRYTES> raw_trytes{};
      std::memcpy(raw_trytes.data(), &limbs[l], raw_trytes.size());
      for (int8_t tr : raw_trytes) {
        if (tr < -13 || tr > 13) return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::vector<int8_t> to_trits() const {
    enforce_oracle_parity(
        has_canonical_trytes(),
        "Non-canonical stored tryte detected in TernaryMultiLimb::to_trits() before TRYTE_TO_TRITS lookup");
    std::vector<int8_t> trits;
    trits.reserve(TRITS);
    std::array<int8_t, t81::core::T81Limb::TRITS> one_buffer{};
    one_buffer[0] = 1;
    auto unit_limb = t81::core::T81Limb27::from_trits_window(one_buffer, 0);

    for (size_t l = 0; l < NumLimbs; ++l) {
      auto l54 = limbs[l] * unit_limb;
      auto all54 = l54.to_trits();
      for (size_t t = 0; t < 27; ++t) {
        trits.push_back(all54[t]);
      }
    }
    return trits;
  }

  static TernaryMultiLimb from_trits(std::span<const int8_t> trits) {
    enforce_oracle_parity(
        trits.empty() || are_trits_in_canonical_range(trits),
        "Non-canonical trit detected in TernaryMultiLimb::from_trits() before from_trits_window");
    TernaryMultiLimb res;
    std::array<int8_t, t81::core::T81Limb::TRITS> buffer{};
    for (size_t l = 0; l < NumLimbs; ++l) {
      buffer.fill(0);
      size_t start = l * 27;
      for (size_t i = 0; i < 27 && (start + i) < trits.size(); ++i) {
        buffer[i] = trits[start + i];
      }
      res.limbs[l] = t81::core::T81Limb27::from_trits_window(buffer, 0);
    }
    return res;
  }

  static TernaryMultiLimb from_oracle(const ExactBigIntOracle& val) {
    auto full_trits = oracle_to_canonical_trits<TRITS>(val);
    return from_trits(full_trits);
  }
};

template <size_t NumLimbs>
struct BinaryMultiLimb {
  static constexpr size_t LIMBS = NumLimbs;
  static constexpr size_t BITS = NumLimbs * 64;
  std::array<uint64_t, NumLimbs> limbs{};
  bool is_negative{false};

  static BinaryMultiLimb from_oracle(const ExactBigIntOracle& val) {
    BinaryMultiLimb res;
    res.is_negative = val.negative;
    ExactBigIntOracle mag = val;
    mag.negative = false;

    for (size_t i = 0; i < NumLimbs; ++i) {
      if (mag.is_zero()) break;
      uint64_t word = 0;
      for (size_t bit = 0; bit < 64; ++bit) {
        uint64_t bit_val = mag.limbs.empty() ? 0 : (mag.limbs[0] % 2);
        if (bit_val) {
          word |= (1ULL << bit);
        }
        std::uint64_t rem = 0;
        for (size_t j = mag.limbs.size(); j-- > 0;) {
          std::uint64_t cur_val = mag.limbs[j] + rem * ExactBigIntOracle::BASE;
          mag.limbs[j] = static_cast<std::uint32_t>(cur_val / 2);
          rem = cur_val % 2;
        }
        mag.trim();
      }
      res.limbs[i] = word;
    }
    return res;
  }

  [[nodiscard]] ExactBigIntOracle to_oracle() const {
    return binary_limbs_to_oracle(limbs, is_negative);
  }
};

// Independent 64-bit binary limb helpers avoiding compiler warnings
inline int cmp_binary_limbs(std::span<const uint64_t> a, std::span<const uint64_t> b,
                            OperationAccounting& acc) noexcept {
  size_t n = std::max(a.size(), b.size());
  for (size_t i = n; i-- > 0;) {
    uint64_t va = i < a.size() ? a[i] : 0;
    uint64_t vb = i < b.size() ? b[i] : 0;
    acc.op_branch_cond++;  // Condition evaluation: va != vb
    if (va != vb) {
      acc.op_branch_cond++;  // Condition evaluation: va < vb
      return va < vb ? -1 : 1;
    }
  }
  return 0;
}

// 64x64 -> 128-bit multiplication using 32-bit halves.
// Accounts for 4 primitive 32x32 multiplications (u0*v0, u1*v0, u0*v1, u1*v1)
// in op_alu_mul and 4 helper 64-bit additions (u1*v0 + (w0>>32), w1 += u0*v1,
// w2 += (w1>>32), u1*v1 + w2) in op_alu_add_sub. Half-word bit shifts and
// masks are excluded from W_ops under the kernel accounting boundary.
inline void mul64(uint64_t u, uint64_t v, uint64_t& hi, uint64_t& lo,
                  OperationAccounting& acc) noexcept {
  uint64_t u0 = u & 0xFFFFFFFFULL;
  uint64_t u1 = u >> 32;
  uint64_t v0 = v & 0xFFFFFFFFULL;
  uint64_t v1 = v >> 32;

  uint64_t w0 = u0 * v0;
  acc.op_alu_mul++;  // Mul 1: u0 * v0
  uint64_t t = u1 * v0 + (w0 >> 32);
  acc.op_alu_mul++;      // Mul 2: u1 * v0
  acc.op_alu_add_sub++;  // Add 1: (u1 * v0) + (w0 >> 32)
  uint64_t w1 = t & 0xFFFFFFFFULL;
  uint64_t w2 = t >> 32;
  w1 += u0 * v1;
  acc.op_alu_mul++;      // Mul 3: u0 * v1
  acc.op_alu_add_sub++;  // Add 2: w1 + (u0 * v1)
  w2 += (w1 >> 32);
  acc.op_alu_add_sub++;  // Add 3: w2 + (w1 >> 32)
  w1 &= 0xFFFFFFFFULL;

  hi = u1 * v1 + w2;
  acc.op_alu_mul++;      // Mul 4: u1 * v1
  acc.op_alu_add_sub++;  // Add 4: (u1 * v1) + w2
  lo = (w1 << 32) | (w0 & 0xFFFFFFFFULL);
}

class ArithmeticWorkHarness {
public:
  template <size_t InLimbs, size_t OutLimbs>
  static TernaryMultiLimb<OutLimbs> add_ternary(const TernaryMultiLimb<InLimbs>& a,
                                                const TernaryMultiLimb<InLimbs>& b,
                                                OperationAccounting& acc) {
    enforce_oracle_parity(
        a.has_canonical_trytes() && b.has_canonical_trytes(),
        "Non-canonical stored tryte detected in add_ternary() input operand before ADD_TABLE lookup");
    TrialBudgetEnforcer::instance().record_trial();

    TernaryMultiLimb<OutLimbs> res;
    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(t81::core::T81Limb27);

    int8_t carry_in = 0;
    std::array<int8_t, t81::core::T81Limb::TRITS> c_buf{};

    for (size_t i = 0; i < InLimbs; ++i) {
      auto [sum_ab, c1] = a.limbs[i].addc(b.limbs[i]);
      acc.op_alu_add_sub++;  // Primary limb addc() call
      // Each T81Limb27::addc() executes 38 named-table lookups:
      // 9 in detail::ADD_TABLE + 20 in detail::COMPOSITION_TABLE (8 + 7 + 5) + 9 in
      // detail::CARRY_FROM_ZERO (8 carries + 1 carry_out).
      // Excludes index-offset arithmetic (+13, +1), base-3 map_id polynomial encoding,
      // local sum_vals[i][carries[i]+1] reads, and fixed-trip loop control.
      acc.op_table_lookup += 38;

      acc.op_branch_cond++;  // Condition evaluation: carry_in != 0
      if (carry_in != 0) {
        c_buf.fill(0);
        c_buf[0] = carry_in;
        auto c_limb = t81::core::T81Limb27::from_trits_window(c_buf, 0);
        acc.conv_limb_repacks++;
        acc.conv_tryte_encodes += t81::core::T81Limb27::TRYTES;
        auto [sum_c, c2] = sum_ab.addc(c_limb);
        acc.op_alu_add_sub++;  // Carry-in limb addc() call
        acc.op_table_lookup += 38;
        res.limbs[i] = sum_c;
        carry_in = static_cast<int8_t>(c1 + c2);
        acc.op_alu_add_sub++;  // Scalar carry combination: c1 + c2
      } else {
        res.limbs[i] = sum_ab;
        carry_in = c1;
      }
    }

    for (size_t i = InLimbs; i < OutLimbs; ++i) {
      acc.op_branch_cond++;  // Condition evaluation: carry_in != 0
      if (carry_in != 0) {
        c_buf.fill(0);
        c_buf[0] = carry_in;
        auto c_limb = t81::core::T81Limb27::from_trits_window(c_buf, 0);
        acc.conv_limb_repacks++;
        acc.conv_tryte_encodes += t81::core::T81Limb27::TRYTES;
        // Extension limb placement is a representation pack + data copy (not an addc() addition).
        res.limbs[i] = c_limb;
        carry_in = 0;
      } else {
        res.limbs[i] = t81::core::T81Limb27{};
      }
    }

    acc.mem_stores += OutLimbs;
    acc.bytes_written += OutLimbs * sizeof(t81::core::T81Limb27);
    return res;
  }

  template <size_t InLimbs, size_t OutLimbs>
  static TernaryMultiLimb<OutLimbs> mul_ternary(const TernaryMultiLimb<InLimbs>& a,
                                                const TernaryMultiLimb<InLimbs>& b,
                                                OperationAccounting& acc) {
    enforce_oracle_parity(
        a.has_canonical_trytes() && b.has_canonical_trytes(),
        "Non-canonical stored tryte detected in mul_ternary() input operand before to_trits() decoding");
    TrialBudgetEnforcer::instance().record_trial();

    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(t81::core::T81Limb27);

    constexpr size_t ACCUM_TRITS = OutLimbs * 27;
    std::vector<int64_t> accum(ACCUM_TRITS, 0);

    // Entry representation unpacking (excluded from W_ops; recorded in conv_* diagnostics):
    // Each to_trits() call packs 1 temporary unit_limb via T81Limb27::from_trits_window
    // (+1 conv_limb_repacks, +9 encode_tryte) and per limb executes limbs[l] * unit_limb
    // (18 TRYTE_TO_TRITS lookups + 18 encode_tryte in T81Limb54::from_trits)
    // followed by l54.to_trits() (18 decode_tryte).
    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();
    acc.conv_limb_unpacks += InLimbs * 2;
    acc.conv_limb_repacks += 2;  // 1 temporary unit_limb via from_trits_window per to_trits() call
    acc.conv_tryte_decodes += InLimbs * 2 * (2 * t81::core::T81Limb27::TRYTES +
                                             t81::core::T81Limb54::TRYTES);
    acc.conv_tryte_encodes += 2 * t81::core::T81Limb27::TRYTES +
                              InLimbs * 2 * t81::core::T81Limb54::TRYTES;

    acc.diagnostic_convolution_ops++;

    for (size_t i = 0; i < a_trits.size(); ++i) {
      int8_t ai = a_trits[i];
      acc.op_branch_cond++;  // Condition evaluation: ai == 0
      if (ai == 0) continue;
      for (size_t j = 0; j < b_trits.size(); ++j) {
        int8_t bj = b_trits[j];
        acc.op_branch_cond++;  // Condition evaluation: bj == 0
        if (bj == 0) continue;
        acc.op_branch_cond++;  // Condition evaluation: i + j < ACCUM_TRITS
        if (i + j < ACCUM_TRITS) {
          accum[i + j] += static_cast<int64_t>(ai) * bj;
          acc.op_alu_mul++;
          acc.op_alu_add_sub++;
        }
      }
    }

    acc.op_norm_pass++;
    acc.diagnostic_reduction_ops++;

    int64_t carry = 0;
    std::vector<int8_t> norm_trits(ACCUM_TRITS, 0);
    for (size_t i = 0; i < ACCUM_TRITS; ++i) {
      int64_t sum = accum[i] + carry;
      acc.norm_alu_add_sub++;

      int64_t rem = (sum + 1) % 3;
      acc.norm_alu_add_sub++;
      if (rem < 0) {
        rem += 3;
        acc.norm_alu_add_sub++;
      }
      rem -= 1;
      acc.norm_alu_add_sub++;
      carry = (sum - rem) / 3;
      acc.norm_alu_add_sub++;
      acc.norm_branch_cond++;

      norm_trits[i] = static_cast<int8_t>(rem);
    }

    // Exit representation repacking (excluded from W_ops; recorded in conv_* diagnostics):
    auto res = TernaryMultiLimb<OutLimbs>::from_trits(norm_trits);
    acc.conv_limb_repacks += OutLimbs;
    acc.conv_tryte_encodes += OutLimbs * t81::core::T81Limb27::TRYTES;
    acc.mem_stores += OutLimbs;
    acc.bytes_written += OutLimbs * sizeof(t81::core::T81Limb27);
    return res;
  }

  // Pure independent binary multi-precision addition (independent of ExactBigIntOracle)
  template <size_t InLimbs, size_t OutLimbs>
  static BinaryMultiLimb<OutLimbs> add_binary(const BinaryMultiLimb<InLimbs>& a,
                                              const BinaryMultiLimb<InLimbs>& b,
                                              OperationAccounting& acc) {
    TrialBudgetEnforcer::instance().record_trial();

    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(uint64_t);

    BinaryMultiLimb<OutLimbs> res;

    acc.op_branch_cond++;  // Condition evaluation: a.is_negative == b.is_negative
    if (a.is_negative == b.is_negative) {
      res.is_negative = a.is_negative;
      uint64_t carry = 0;
      for (size_t i = 0; i < InLimbs; ++i) {
        uint64_t sum_ab = a.limbs[i] + b.limbs[i];
        acc.op_alu_add_sub++;  // Primitive addition 1: a.limbs[i] + b.limbs[i]
        uint64_t sum = sum_ab + carry;
        acc.op_alu_add_sub++;  // Primitive addition 2: sum_ab + carry

        acc.op_branch_cond++;  // Condition evaluation: carry != 0
        uint64_t new_carry = 0;
        if (carry != 0) {
          acc.op_branch_cond++;  // Carry predicate evaluation: sum <= a.limbs[i]
          new_carry = (sum <= a.limbs[i]) ? 1 : 0;
        } else {
          acc.op_branch_cond++;  // Carry predicate evaluation: sum < a.limbs[i]
          new_carry = (sum < a.limbs[i]) ? 1 : 0;
        }
        res.limbs[i] = sum;
        carry = new_carry;
      }
      for (size_t i = InLimbs; i < OutLimbs; ++i) {
        // Extension limb propagation is a pure data copy (0 arithmetic operations).
        res.limbs[i] = carry;
        carry = 0;
      }
    } else {
      int c = cmp_binary_limbs(a.limbs, b.limbs, acc);
      acc.op_branch_cond++;  // Condition evaluation: c == 0
      if (c == 0) {
        res.is_negative = false;
      } else {
        acc.op_branch_cond++;  // Condition evaluation: c > 0
        bool a_larger = (c > 0);
        const auto& larger = a_larger ? a : b;
        const auto& smaller = a_larger ? b : a;
        res.is_negative = larger.is_negative;
        uint64_t borrow = 0;
        for (size_t i = 0; i < InLimbs; ++i) {
          uint64_t sub = smaller.limbs[i] + borrow;
          acc.op_alu_add_sub++;  // sub = smaller.limbs[i] + borrow
          acc.op_branch_cond++;  // Borrow predicate evaluation: sub < smaller.limbs[i]
          uint64_t new_borrow = 1;
          if (!(sub < smaller.limbs[i])) {
            acc.op_branch_cond++;  // Short-circuit borrow predicate: larger.limbs[i] < sub
            new_borrow = (larger.limbs[i] < sub) ? 1 : 0;
          }
          res.limbs[i] = larger.limbs[i] - sub;
          borrow = new_borrow;
          acc.op_alu_add_sub++;  // res = larger.limbs[i] - sub
        }
        for (size_t i = InLimbs; i < OutLimbs; ++i) {
          res.limbs[i] = 0;
        }
      }
    }

    acc.mem_stores += OutLimbs;
    acc.bytes_written += OutLimbs * sizeof(uint64_t);
    return res;
  }

  // Pure independent binary multi-precision convolution multiplication (independent of
  // ExactBigIntOracle)
  template <size_t InLimbs, size_t OutLimbs>
  static BinaryMultiLimb<OutLimbs> mul_binary(const BinaryMultiLimb<InLimbs>& a,
                                              const BinaryMultiLimb<InLimbs>& b,
                                              OperationAccounting& acc) {
    TrialBudgetEnforcer::instance().record_trial();

    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(uint64_t);

    BinaryMultiLimb<OutLimbs> res;
    res.is_negative = (a.is_negative != b.is_negative);

    acc.diagnostic_convolution_ops++;

    for (size_t i = 0; i < InLimbs; ++i) {
      uint64_t ai = a.limbs[i];
      acc.op_branch_cond++;  // Condition evaluation: ai == 0
      if (ai == 0) continue;
      uint64_t carry = 0;
      for (size_t j = 0; j < InLimbs; ++j) {
        uint64_t bj = b.limbs[j];
        acc.op_branch_cond++;  // Condition evaluation: i + j >= OutLimbs
        if (i + j >= OutLimbs) break;
        uint64_t hi = 0, lo = 0;
        mul64(ai, bj, hi, lo, acc);

        // Add lo + carry + res.limbs[i+j] (4 additions in caller + 4 in mul64 = 8 per inner step)
        uint64_t cur = res.limbs[i + j];
        uint64_t sum1 = cur + lo;
        acc.op_branch_cond++;  // Condition evaluation: sum1 < cur
        uint64_t c1 = (sum1 < cur) ? 1 : 0;
        uint64_t sum2 = sum1 + carry;
        acc.op_branch_cond++;  // Condition evaluation: sum2 < sum1
        uint64_t c2 = (sum2 < sum1) ? 1 : 0;
        res.limbs[i + j] = sum2;
        carry = hi + c1 + c2;
        acc.op_alu_add_sub +=
            4;  // sum1 = cur + lo; sum2 = sum1 + carry; carry = hi + c1; carry += c2
      }
      for (size_t k = i + InLimbs; k < OutLimbs; ++k) {
        acc.op_branch_cond++;  // Condition evaluation: carry != 0
        if (carry == 0) break;
        uint64_t cur = res.limbs[k];
        uint64_t sum = cur + carry;
        acc.op_branch_cond++;  // Condition evaluation: sum < cur
        carry = (sum < cur) ? 1 : 0;
        res.limbs[k] = sum;
        acc.op_alu_add_sub++;
      }
    }

    bool all_zero = true;
    for (size_t i = 0; i < OutLimbs; ++i) {
      acc.op_branch_cond++;  // Condition evaluation: res.limbs[i] != 0
      if (res.limbs[i] != 0) {
        all_zero = false;
        break;
      }
    }
    acc.op_branch_cond++;  // Condition evaluation: all_zero
    if (all_zero) res.is_negative = false;

    acc.mem_stores += OutLimbs;
    acc.bytes_written += OutLimbs * sizeof(uint64_t);
    return res;
  }
};

// 5-Dimensional Oracle Validation Evaluator for a multi-precision band operation pair
template <size_t T_In, size_t T_Add, size_t T_Mul, size_t B_In, size_t B_Add, size_t B_Mul>
inline OracleValidationReport evaluate_oracle_validation_report(
    size_t bit_width, const ExactBigIntOracle& oa, const ExactBigIntOracle& ob,
    const TernaryMultiLimb<T_In>& ta, std::span<const int8_t> ta_trits,
    const TernaryMultiLimb<T_In>& tb, std::span<const int8_t> tb_trits,
    const TernaryMultiLimb<T_Add>& t_sum, std::span<const int8_t> t_sum_trits,
    const TernaryMultiLimb<T_Mul>& t_prod, std::span<const int8_t> t_prod_trits,
    const BinaryMultiLimb<B_In>& ba, const BinaryMultiLimb<B_In>& bb,
    const BinaryMultiLimb<B_Add>& b_sum, const BinaryMultiLimb<B_Mul>& b_prod,
    const ExactBigIntOracle& expected_oa_sum, const ExactBigIntOracle& expected_oa_prod,
    bool expected_sum_overflow, bool expected_prod_overflow) {
  OracleValidationReport rep{};
  const auto& spec = get_accumulator_spec(bit_width);

  bool all_trytes_canonical = ta.has_canonical_trytes() && tb.has_canonical_trytes() &&
                              t_sum.has_canonical_trytes() && t_prod.has_canonical_trytes();
  bool trit_sizes_valid = (ta_trits.size() == T_In * 27) && (tb_trits.size() == T_In * 27) &&
                          (t_sum_trits.size() == T_Add * 27) && (t_prod_trits.size() == T_Mul * 27);

  ExactBigIntOracle ta_oracle = trits_to_oracle(ta_trits);
  ExactBigIntOracle tb_oracle = trits_to_oracle(tb_trits);
  ExactBigIntOracle computed_t_sum_oracle = trits_to_oracle(t_sum_trits);
  ExactBigIntOracle computed_t_prod_oracle = trits_to_oracle(t_prod_trits);

  ExactBigIntOracle ba_oracle = ba.to_oracle();
  ExactBigIntOracle bb_oracle = bb.to_oracle();
  ExactBigIntOracle computed_b_sum_oracle = b_sum.to_oracle();
  ExactBigIntOracle computed_b_prod_oracle = b_prod.to_oracle();

  // Dimension 1: Exact numerical equality across converted inputs and computed outputs
  rep.numerical_equality =
      all_trytes_canonical && trit_sizes_valid &&
      (ta_oracle == oa) && (tb_oracle == ob) && (ba_oracle == oa) && (bb_oracle == ob) &&
      (computed_t_sum_oracle == expected_oa_sum) && (computed_t_prod_oracle == expected_oa_prod) &&
      (computed_b_sum_oracle == expected_oa_sum) && (computed_b_prod_oracle == expected_oa_prod);

  // Dimension 2: Two-sided signed overflow classification against [-2^(B-1), 2^(B-1)-1]
  // for both input operands and computed addition/multiplication results, plus exact
  // accumulator bounds: [-2^B, 2^B - 2] for addition and [-2^(B-1)*(2^(B-1)-1), 2^(2B-2)] for multiplication
  bool inputs_in_domain = is_in_signed_b_domain(oa, bit_width) &&
                          is_in_signed_b_domain(ob, bit_width) &&
                          is_in_signed_b_domain(ta_oracle, bit_width) &&
                          is_in_signed_b_domain(tb_oracle, bit_width) &&
                          is_in_signed_b_domain(ba_oracle, bit_width) &&
                          is_in_signed_b_domain(bb_oracle, bit_width);

  bool oracle_sum_overflow = !is_in_signed_b_domain(expected_oa_sum, bit_width);
  bool oracle_prod_overflow = !is_in_signed_b_domain(expected_oa_prod, bit_width);
  bool t_sum_overflow = !is_in_signed_b_domain(computed_t_sum_oracle, bit_width);
  bool b_sum_overflow = !is_in_signed_b_domain(computed_b_sum_oracle, bit_width);
  bool t_prod_overflow = !is_in_signed_b_domain(computed_t_prod_oracle, bit_width);
  bool b_prod_overflow = !is_in_signed_b_domain(computed_b_prod_oracle, bit_width);

  ExactBigIntOracle max_add_mag = ExactBigIntOracle::from_power_of_two(bit_width);
  ExactBigIntOracle min_add_bound = max_add_mag;
  min_add_bound.negative = true;
  ExactBigIntOracle max_add_bound = ExactBigIntOracle::sub(max_add_mag, ExactBigIntOracle(2));

  ExactBigIntOracle half_domain = ExactBigIntOracle::from_power_of_two(bit_width - 1);
  ExactBigIntOracle max_operand = ExactBigIntOracle::sub(half_domain, ExactBigIntOracle(1));
  ExactBigIntOracle max_prod_bound = ExactBigIntOracle::from_power_of_two(2 * bit_width - 2);
  ExactBigIntOracle min_prod_bound = ExactBigIntOracle::mul(half_domain, max_operand);
  min_prod_bound.negative = true;

  bool sum_in_accum_bounds =
      (expected_oa_sum >= min_add_bound && expected_oa_sum <= max_add_bound) &&
      (computed_t_sum_oracle >= min_add_bound && computed_t_sum_oracle <= max_add_bound) &&
      (computed_b_sum_oracle >= min_add_bound && computed_b_sum_oracle <= max_add_bound);
  bool prod_in_accum_bounds =
      (expected_oa_prod >= min_prod_bound && expected_oa_prod <= max_prod_bound) &&
      (computed_t_prod_oracle >= min_prod_bound && computed_t_prod_oracle <= max_prod_bound) &&
      (computed_b_prod_oracle >= min_prod_bound && computed_b_prod_oracle <= max_prod_bound);

  rep.overflow_classification = inputs_in_domain &&
                                (oracle_sum_overflow == expected_sum_overflow) &&
                                (t_sum_overflow == expected_sum_overflow) &&
                                (b_sum_overflow == expected_sum_overflow) &&
                                (oracle_prod_overflow == expected_prod_overflow) &&
                                (t_prod_overflow == expected_prod_overflow) &&
                                (b_prod_overflow == expected_prod_overflow) &&
                                sum_in_accum_bounds && prod_in_accum_bounds;

  // Dimension 3: Canonical balanced-ternary digits and stored trytes on inputs and outputs
  rep.canonical_digits =
      ta.has_canonical_trytes() && tb.has_canonical_trytes() &&
      t_sum.has_canonical_trytes() && t_prod.has_canonical_trytes() &&
      are_trits_canonical_for_oracle<T_In * 27>(ta_trits, oa) &&
      are_trits_canonical_for_oracle<T_In * 27>(tb_trits, ob) &&
      are_trits_canonical_for_oracle<T_Add * 27>(t_sum_trits, expected_oa_sum) &&
      are_trits_canonical_for_oracle<T_Mul * 27>(t_prod_trits, expected_oa_prod);

  // Dimension 4: Sign correctness across oracle, ternary leading non-zero trits, and binary limbs
  bool oracle_signs_valid =
      !(oa.is_zero() && oa.negative) && !(ob.is_zero() && ob.negative) &&
      !(expected_oa_sum.is_zero() && expected_oa_sum.negative) &&
      !(expected_oa_prod.is_zero() && expected_oa_prod.negative) &&
      (oa.negative ? (oa < ExactBigIntOracle(0)) : (oa >= ExactBigIntOracle(0))) &&
      (ob.negative ? (ob < ExactBigIntOracle(0)) : (ob >= ExactBigIntOracle(0)));

  rep.sign_handling =
      oracle_signs_valid && verify_ternary_sign(ta_trits, oa) &&
      verify_ternary_sign(tb_trits, ob) && verify_ternary_sign(t_sum_trits, expected_oa_sum) &&
      verify_ternary_sign(t_prod_trits, expected_oa_prod) &&
      verify_binary_sign(ba.limbs, ba.is_negative, oa) &&
      verify_binary_sign(bb.limbs, bb.is_negative, ob) &&
      verify_binary_sign(b_sum.limbs, b_sum.is_negative, expected_oa_sum) &&
      verify_binary_sign(b_prod.limbs, b_prod.is_negative, expected_oa_prod);

  // Dimension 5: Absence of truncation (accumulator capacity sufficiency, zero high-order
  // residual, and exact preservation of high-order extension trits and binary limbs)
  bool capacity_ok = (T_In * 27 >= spec.operand_trits) && (T_Add * 27 >= spec.add_trits_min) &&
                     (T_Mul * 27 >= spec.mul_trits_min) && (B_In * 64 >= bit_width) &&
                     (B_Add * 64 >= bit_width + 1) && (B_Mul * 64 >= 2 * bit_width);

  bool no_residual = oracle_fits_in_trits(oa, spec.operand_trits) &&
                     oracle_fits_in_trits(ob, spec.operand_trits) &&
                     oracle_fits_in_trits(expected_oa_sum, spec.add_trits_min) &&
                     oracle_fits_in_trits(expected_oa_prod, spec.mul_trits_min) &&
                     oracle_fits_in_binary_limbs(expected_oa_sum, B_Add) &&
                     oracle_fits_in_binary_limbs(expected_oa_prod, B_Mul);

  auto expected_b_sum = BinaryMultiLimb<B_Add>::from_oracle(expected_oa_sum);
  auto expected_b_prod = BinaryMultiLimb<B_Mul>::from_oracle(expected_oa_prod);
  auto expected_t_sum_trits = oracle_to_canonical_trits<T_Add * 27>(expected_oa_sum);
  auto expected_t_prod_trits = oracle_to_canonical_trits<T_Mul * 27>(expected_oa_prod);

  bool high_trits_preserved = (t_sum_trits.size() == T_Add * 27) &&
                              (t_prod_trits.size() == T_Mul * 27);
  if (high_trits_preserved) {
    for (size_t i = spec.operand_trits; i < T_Add * 27; ++i) {
      if (t_sum_trits[i] != expected_t_sum_trits[i]) high_trits_preserved = false;
    }
    for (size_t i = spec.add_trits_min; i < T_Add * 27; ++i) {
      if (t_sum_trits[i] != 0) high_trits_preserved = false;
    }
    for (size_t i = spec.operand_trits; i < T_Mul * 27; ++i) {
      if (t_prod_trits[i] != expected_t_prod_trits[i]) high_trits_preserved = false;
    }
    for (size_t i = spec.mul_trits_min; i < T_Mul * 27; ++i) {
      if (t_prod_trits[i] != 0) high_trits_preserved = false;
    }
  }

  bool high_binary_limbs_preserved =
      (b_sum.limbs == expected_b_sum.limbs) && (b_prod.limbs == expected_b_prod.limbs);

  rep.absence_of_truncation =
      all_trytes_canonical && trit_sizes_valid && capacity_ok && no_residual &&
      high_trits_preserved && high_binary_limbs_preserved &&
      (computed_t_sum_oracle == expected_oa_sum) && (computed_t_prod_oracle == expected_oa_prod) &&
      (computed_b_sum_oracle == expected_oa_sum) && (computed_b_prod_oracle == expected_oa_prod);

  return rep;
}

// Object-level 5-Dimensional Oracle Validation Evaluator:
// Validates stored trytes (has_canonical_trytes()) BEFORE calling to_trits() so a malformed
// stored tryte (e.g. +14) can never reach TRYTE_TO_TRITS[tryte + 13].
template <size_t T_In, size_t T_Add, size_t T_Mul, size_t B_In, size_t B_Add, size_t B_Mul>
inline OracleValidationReport evaluate_oracle_validation_report(
    size_t bit_width, const ExactBigIntOracle& oa, const ExactBigIntOracle& ob,
    const TernaryMultiLimb<T_In>& ta, const TernaryMultiLimb<T_In>& tb,
    const TernaryMultiLimb<T_Add>& t_sum, const TernaryMultiLimb<T_Mul>& t_prod,
    const BinaryMultiLimb<B_In>& ba, const BinaryMultiLimb<B_In>& bb,
    const BinaryMultiLimb<B_Add>& b_sum, const BinaryMultiLimb<B_Mul>& b_prod,
    const ExactBigIntOracle& expected_oa_sum, const ExactBigIntOracle& expected_oa_prod,
    bool expected_sum_overflow, bool expected_prod_overflow) {
  std::vector<int8_t> ta_trits = ta.has_canonical_trytes() ? ta.to_trits() : std::vector<int8_t>{};
  std::vector<int8_t> tb_trits = tb.has_canonical_trytes() ? tb.to_trits() : std::vector<int8_t>{};
  std::vector<int8_t> t_sum_trits =
      t_sum.has_canonical_trytes() ? t_sum.to_trits() : std::vector<int8_t>{};
  std::vector<int8_t> t_prod_trits =
      t_prod.has_canonical_trytes() ? t_prod.to_trits() : std::vector<int8_t>{};

  return evaluate_oracle_validation_report<T_In, T_Add, T_Mul, B_In, B_Add, B_Mul>(
      bit_width, oa, ob, ta, ta_trits, tb, tb_trits, t_sum, t_sum_trits, t_prod, t_prod_trits, ba,
      bb, b_sum, b_prod, expected_oa_sum, expected_oa_prod, expected_sum_overflow,
      expected_prod_overflow);
}

}  // namespace scg08
