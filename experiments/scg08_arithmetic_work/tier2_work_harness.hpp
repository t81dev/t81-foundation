#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "exact_oracle_adapter.hpp"
#include "t81/types/T81Limb.hpp"

namespace scg08 {

// ============================================================================
// Tier 2: Arithmetic Work and Operation Instrumentation
//
// Accounting revision: RFC-00E1 correction v2 (historical observations unchanged).
// Selected kernel events only; see RFC section 3 for exclusions.
// Frozen Weighted Metric:
// W_ops = OP_ALU_ADD_SUB + 3*OP_ALU_MUL + OP_TABLE_LOOKUP + OP_BRANCH_COND +
// OP_NORM_PASS
//
// Strictly non-aggregating diagnostic counters:
// - OP_CONVOLUTION
// - OP_REDUCTION
//
// Normalization internal primitive operations recorded separately:
// - NORM_ALU_ADD_SUB
// - NORM_ALU_MUL
// - NORM_TABLE_LOOKUP
// - NORM_BRANCH_COND
//
// Memory traffic recorded independently:
// - bytes_read, bytes_written, loads, stores
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

  [[nodiscard]] std::vector<int8_t> to_trits() const {
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

// 64x64 -> 128-bit multiplication using 32-bit halves, accounting for primitive operations
inline void mul64(uint64_t u, uint64_t v, uint64_t& hi, uint64_t& lo,
                  OperationAccounting& acc) noexcept {
  uint64_t u0 = u & 0xFFFFFFFFULL;
  uint64_t u1 = u >> 32;
  uint64_t v0 = v & 0xFFFFFFFFULL;
  uint64_t v1 = v >> 32;

  uint64_t w0 = u0 * v0;
  acc.op_alu_mul++;
  uint64_t t = u1 * v0 + (w0 >> 32);
  acc.op_alu_mul++;
  acc.op_alu_add_sub++;
  uint64_t w1 = t & 0xFFFFFFFFULL;
  uint64_t w2 = t >> 32;
  w1 += u0 * v1;
  acc.op_alu_mul++;
  acc.op_alu_add_sub++;
  w2 += (w1 >> 32);
  acc.op_alu_add_sub++;
  w1 &= 0xFFFFFFFFULL;

  hi = u1 * v1 + w2;
  acc.op_alu_mul++;
  acc.op_alu_add_sub++;
  lo = (w1 << 32) | (w0 & 0xFFFFFFFFULL);
}

class ArithmeticWorkHarness {
public:
  template <size_t InLimbs, size_t OutLimbs>
  static TernaryMultiLimb<OutLimbs> add_ternary(const TernaryMultiLimb<InLimbs>& a,
                                                const TernaryMultiLimb<InLimbs>& b,
                                                OperationAccounting& acc) {
    TrialBudgetEnforcer::instance().record_trial();

    TernaryMultiLimb<OutLimbs> res;
    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(t81::core::T81Limb27);

    int8_t carry_in = 0;
    std::array<int8_t, t81::core::T81Limb::TRITS> c_buf{};

    for (size_t i = 0; i < InLimbs; ++i) {
      auto [sum_ab, c1] = a.limbs[i].addc(b.limbs[i]);
      acc.op_alu_add_sub++;
      // Each T81Limb27::addc() executes 38 table lookups:
      // 9 in detail::ADD_TABLE + 20 in detail::COMPOSITION_TABLE (8 + 7 + 5) + 9 in
      // detail::CARRY_FROM_ZERO (8 carries + 1 carry_out)
      acc.op_table_lookup += 38;

      acc.op_branch_cond++;  // Condition evaluation: carry_in != 0
      if (carry_in != 0) {
        c_buf.fill(0);
        c_buf[0] = carry_in;
        auto c_limb = t81::core::T81Limb27::from_trits_window(c_buf, 0);
        auto [sum_c, c2] = sum_ab.addc(c_limb);
        acc.op_alu_add_sub++;
        acc.op_table_lookup += 38;
        res.limbs[i] = sum_c;
        carry_in = c1 + c2;
        acc.op_alu_add_sub++; // scalar carry combination, separate from addc
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
        res.limbs[i] = c_limb;
        carry_in = 0; // representation copy, no arithmetic event
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
    TrialBudgetEnforcer::instance().record_trial();

    acc.mem_loads += InLimbs * 2;
    acc.bytes_read += InLimbs * 2 * sizeof(t81::core::T81Limb27);

    constexpr size_t ACCUM_TRITS = OutLimbs * 27;
    std::vector<int64_t> accum(ACCUM_TRITS, 0);

    auto a_trits = a.to_trits();
    auto b_trits = b.to_trits();

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

    auto res = TernaryMultiLimb<OutLimbs>::from_trits(norm_trits);
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
        uint64_t sum = a.limbs[i] + b.limbs[i] + carry;
        acc.op_branch_cond++;  // Condition evaluation: carry != 0
        uint64_t new_carry = 0;
        acc.op_branch_cond++; // selected <= or < carry predicate
        if (carry != 0) {
          new_carry = (sum <= a.limbs[i]) ? 1 : 0;
        } else {
          new_carry = (sum < a.limbs[i]) ? 1 : 0;
        }
        res.limbs[i] = sum;
        carry = new_carry;
        acc.op_alu_add_sub += 2; // a + b, then + carry
      }
      for (size_t i = InLimbs; i < OutLimbs; ++i) {
        res.limbs[i] = carry;
        carry = 0; // copy into extension limb, no arithmetic event
      }
    } else {
      int c = cmp_binary_limbs(a.limbs, b.limbs, acc);
      acc.op_branch_cond++;  // Condition evaluation: c == 0
      if (c == 0) {
        res.is_negative = false;
      } else {
        acc.op_branch_cond++;  // Condition evaluation: c > 0
        const auto& larger = (c > 0) ? a : b;
        acc.op_branch_cond++; // second c > 0 selection
        const auto& smaller = (c > 0) ? b : a;
        res.is_negative = larger.is_negative;
        uint64_t borrow = 0;
        for (size_t i = 0; i < InLimbs; ++i) {
          uint64_t sub = smaller.limbs[i] + borrow;
          acc.op_alu_add_sub++;  // sub = smaller.limbs[i] + borrow
          acc.op_branch_cond++;  // Condition evaluation: smaller.limbs[i] + borrow <
                                 // smaller.limbs[i]
          bool b1 = (sub < smaller.limbs[i]); // reuse the already computed sum
          bool b2 = false;
          acc.op_branch_cond++; // !b1 decision
          if (!b1) {
            acc.op_branch_cond++;  // Short-circuit condition evaluation: larger.limbs[i] < sub
            b2 = (larger.limbs[i] < sub);
          }
          acc.op_branch_cond++; // combined borrow decision
          uint64_t new_borrow = (b1 || b2) ? 1 : 0;
          res.limbs[i] = larger.limbs[i] - sub;
          borrow = new_borrow;
          acc.op_alu_add_sub++;  // res = larger.limbs[i] - sub
        }
        for (size_t i = InLimbs; i < OutLimbs; ++i) {
          res.limbs[i] = 0;
        }
      }
    }

    bool all_zero = true;
    for (size_t i = 0; i < OutLimbs; ++i) {
      acc.op_branch_cond++;
      if (res.limbs[i] != 0) { all_zero = false; break; }
    }
    acc.op_branch_cond++;
    if (all_zero) res.is_negative = false;

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
    acc.op_branch_cond++; // sign comparison

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

        // Add lo + carry + res.limbs[i+j]
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

}  // namespace scg08
