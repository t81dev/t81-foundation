#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "t81/codec/trit_packing.hpp"
#include "t81/tritwise/profiling.hpp"
#include "t81/types/Result.hpp"
#include "t81/types/T81Int.hpp"

namespace t81::swar::kernel {
void t_not(const uint8_t* src, uint8_t* dst, size_t n);
void t_and(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n);
void t_or(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n);
}  // namespace t81::swar::kernel

namespace t81::experimental {

class ComputeTritVector;

class PackedTritVector {
public:
  // Factory methods
  static Result<PackedTritVector> from_compute(const ComputeTritVector& other);

  static Result<PackedTritVector> from_trits(const std::vector<int8_t>& trits) {
    std::vector<Trit> t_vec;
    t_vec.reserve(trits.size());
    for (auto t : trits) {
      if (t < -1 || t > 1) {
        return Result<PackedTritVector>::failure(T81Symbol::intern("INVALID_TRIT"),
                                                 T81String("Input trit value must be -1, 0, or 1"),
                                                 T81Symbol::intern("PackedTritVector"));
      }
      t_vec.push_back(static_cast<Trit>(t));
    }
    auto res = t81::codec::trit_packing::pack_pt5(t_vec);
    if (res.is_err()) {
      return Result<PackedTritVector>(res.error());
    }
    return Result<PackedTritVector>::success(PackedTritVector(res.value(), trits.size()));
  }

  static Result<PackedTritVector> from_packed(const std::vector<uint8_t>& packed,
                                              size_t trit_count) {
    auto res = t81::codec::trit_packing::unpack_pt5(packed, trit_count);
    if (res.is_err()) {
      return Result<PackedTritVector>(res.error());
    }
    return Result<PackedTritVector>::success(PackedTritVector(packed, trit_count));
  }

  // Accessors
  size_t size() const { return count_; }
  const std::vector<uint8_t>& packed_data() const { return packed_; }

  Result<std::vector<int8_t>> to_trits() const {
    auto res = t81::codec::trit_packing::unpack_pt5(packed_, count_);
    if (res.is_err()) {
      return Result<std::vector<int8_t>>(res.error());
    }
    std::vector<int8_t> out;
    out.reserve(count_);
    for (auto t : res.value()) {
      out.push_back(static_cast<int8_t>(t));
    }
    return Result<std::vector<int8_t>>::success(out);
  }

  // Scalar logic (source of truth)
  static int8_t scalar_not(int8_t t) { return -t; }

  static int8_t scalar_and(int8_t a, int8_t b) { return std::min(a, b); }

  static int8_t scalar_or(int8_t a, int8_t b) { return std::max(a, b); }

  // TXor semantics: Defined as (a - b) wrapped, per core/vm/vm.cpp implementation of Opcode::TXor.
  // Note: This operation is non-commutative (Difference), despite the name "Xor".
  static int8_t scalar_xor(int8_t a, int8_t b) {
    int result = a - b;
    if (result > 1) return -1;
    if (result < -1) return 1;
    return static_cast<int8_t>(result);
  }

  // Packed operations (unpack-operate-repack)
  Result<PackedTritVector> t_not() const {
    auto trits_res = to_trits();
    if (trits_res.is_err()) return Result<PackedTritVector>(trits_res.error());

    std::vector<int8_t> result_trits;
    result_trits.reserve(count_);
    for (auto t : trits_res.value()) {
      result_trits.push_back(scalar_not(t));
    }
    return from_trits(result_trits);
  }

  Result<PackedTritVector> t_and(const PackedTritVector& other) const {
    if (count_ != other.count_) {
      return Result<PackedTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("PackedTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<PackedTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<PackedTritVector>(rhs_res.error());

    std::vector<int8_t> result_trits;
    result_trits.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      result_trits.push_back(scalar_and(lhs_res.value()[i], rhs_res.value()[i]));
    }
    return from_trits(result_trits);
  }

  Result<PackedTritVector> t_or(const PackedTritVector& other) const {
    if (count_ != other.count_) {
      return Result<PackedTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("PackedTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<PackedTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<PackedTritVector>(rhs_res.error());

    std::vector<int8_t> result_trits;
    result_trits.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      result_trits.push_back(scalar_or(lhs_res.value()[i], rhs_res.value()[i]));
    }
    return from_trits(result_trits);
  }

  Result<PackedTritVector> t_xor(const PackedTritVector& other) const {
    if (count_ != other.count_) {
      return Result<PackedTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("PackedTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<PackedTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<PackedTritVector>(rhs_res.error());

    std::vector<int8_t> result_trits;
    result_trits.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      result_trits.push_back(scalar_xor(lhs_res.value()[i], rhs_res.value()[i]));
    }
    return from_trits(result_trits);
  }

private:
  PackedTritVector(std::vector<uint8_t> packed, size_t count)
      : packed_(std::move(packed)), count_(count) {}

  std::vector<uint8_t> packed_;
  size_t count_;
};

// Phase 2A: Compute-Friendly Representation Prototype
// Uses 2-bit packing (4 trits per byte) for faster access than PT-5.
// Mapping: 0->00, 1->01, -1->11 (Invalid: 10)
class ComputeTritVector {
public:
  static Result<ComputeTritVector> from_trits(const std::vector<int8_t>& trits) {
    std::vector<uint8_t> data;
    size_t packed_len = bytes_for_trits(trits.size());
    data.reserve(packed_len);

    for (size_t i = 0; i < trits.size(); i += 4) {
      uint8_t byte = 0;
      for (size_t j = 0; j < 4; ++j) {
        if (i + j < trits.size()) {
          int8_t t = trits[i + j];
          if (t < -1 || t > 1) {
            return Result<ComputeTritVector>::failure(
                T81Symbol::intern("INVALID_TRIT"),
                T81String("Input trit value must be -1, 0, or 1"),
                T81Symbol::intern("ComputeTritVector"));
          }
          // Map: 0->00, 1->01, -1->11.
          uint8_t val = 0;
          if (t == 0)
            val = 0;  // 00
          else if (t == 1)
            val = 1;  // 01
          else if (t == -1)
            val = 3;  // 11

          byte |= (val << (j * 2));
        }
      }
      data.push_back(byte);
    }
    return Result<ComputeTritVector>::success(ComputeTritVector(std::move(data), trits.size()));
  }

  static Result<ComputeTritVector> from_phase1(const PackedTritVector& other) {
    auto trits_res = other.to_trits();
    if (trits_res.is_err()) return Result<ComputeTritVector>(trits_res.error());
    return from_trits(trits_res.value());
  }

  static Result<ComputeTritVector> from_pt5(const PackedTritVector& other) {
    return from_phase1(other);
  }

  static Result<ComputeTritVector> from_packed(const std::vector<uint8_t>& packed,
                                               size_t trit_count) {
    if (packed.size() != bytes_for_trits(trit_count)) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("INVALID_PACKED_LENGTH"),
          T81String("Packed byte length does not match trit count"),
          T81Symbol::intern("ComputeTritVector"));
    }

    for (size_t i = 0; i < trit_count; ++i) {
      size_t byte_idx = i / 4;
      size_t bit_idx = (i % 4) * 2;
      uint8_t val = (packed[byte_idx] >> bit_idx) & 0x03;
      if (val == 2) {
        return Result<ComputeTritVector>::failure(T81Symbol::intern("INVALID_PACKED_DATA"),
                                                  T81String("Encountered invalid 2-bit pattern"),
                                                  T81Symbol::intern("ComputeTritVector"));
      }
    }

    if (trit_count % 4 != 0 && !packed.empty()) {
      const uint8_t valid_mask = static_cast<uint8_t>((1u << ((trit_count % 4) * 2)) - 1u);
      if ((packed.back() & static_cast<uint8_t>(~valid_mask)) != 0) {
        return Result<ComputeTritVector>::failure(
            T81Symbol::intern("INVALID_TAIL_PADDING"),
            T81String("Unused trailing packed bits must be zero"),
            T81Symbol::intern("ComputeTritVector"));
      }
    }

    return Result<ComputeTritVector>::success(ComputeTritVector(packed, trit_count));
  }

  size_t size() const { return count_; }
  const std::vector<uint8_t>& data() const { return data_; }
  // Non-const data access for in-place benchmarks/tests that need raw pointers,
  // though generally discouraged in public API.
  std::vector<uint8_t>& data_mut() { return data_; }

  Result<std::vector<int8_t>> to_trits() const {
    std::vector<int8_t> out;
    out.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      size_t byte_idx = i / 4;
      size_t bit_idx = (i % 4) * 2;
      uint8_t byte = data_[byte_idx];
      uint8_t val = (byte >> bit_idx) & 0x03;

      int8_t t = 0;
      if (val == 0)
        t = 0;
      else if (val == 1)
        t = 1;
      else if (val == 3)
        t = -1;
      else {
        return Result<std::vector<int8_t>>::failure(T81Symbol::intern("INVALID_PACKED_DATA"),
                                                    T81String("Encountered invalid 2-bit pattern"),
                                                    T81Symbol::intern("ComputeTritVector"));
      }
      out.push_back(t);
    }
    return Result<std::vector<int8_t>>::success(out);
  }

  Result<ComputeTritVector> t_not_ref() const {
    auto trits_res = to_trits();
    if (trits_res.is_err()) return Result<ComputeTritVector>(trits_res.error());
    std::vector<int8_t> trits = trits_res.value();
    for (auto& t : trits) t = PackedTritVector::scalar_not(t);
    return from_trits(trits);
  }

  Result<ComputeTritVector> t_and_ref(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<ComputeTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<ComputeTritVector>(rhs_res.error());

    auto& lhs = lhs_res.value();
    auto& rhs = rhs_res.value();
    std::vector<int8_t> res;
    res.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      res.push_back(PackedTritVector::scalar_and(lhs[i], rhs[i]));
    }
    return from_trits(res);
  }

  Result<ComputeTritVector> t_or_ref(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<ComputeTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<ComputeTritVector>(rhs_res.error());

    auto& lhs = lhs_res.value();
    auto& rhs = rhs_res.value();
    std::vector<int8_t> res;
    res.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      res.push_back(PackedTritVector::scalar_or(lhs[i], rhs[i]));
    }
    return from_trits(res);
  }

  Result<ComputeTritVector> t_xor_ref(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    auto lhs_res = to_trits();
    if (lhs_res.is_err()) return Result<ComputeTritVector>(lhs_res.error());
    auto rhs_res = other.to_trits();
    if (rhs_res.is_err()) return Result<ComputeTritVector>(rhs_res.error());

    auto& lhs = lhs_res.value();
    auto& rhs = rhs_res.value();
    std::vector<int8_t> res;
    res.reserve(count_);
    for (size_t i = 0; i < count_; ++i) {
      res.push_back(PackedTritVector::scalar_xor(lhs[i], rhs[i]));
    }
    return from_trits(res);
  }

  // Phase 2B: Direct operations using LUT
  Result<ComputeTritVector> t_not_lut() const {
    const auto& luts = LUTs::get();
    std::vector<uint8_t> res_data;
    res_data.reserve(data_.size());

    for (size_t i = 0; i < data_.size(); ++i) {
      res_data.push_back(luts.op_not[data_[i]]);
    }

    if (count_ % 4 != 0 && !res_data.empty()) {
      mask_trailing(res_data.back(), count_ % 4);
    }
    T81_PROFILE_RECORD("TXor", data_.size());

    return Result<ComputeTritVector>::success(ComputeTritVector(std::move(res_data), count_));
  }

  Result<ComputeTritVector> t_and_lut(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    const auto& luts = LUTs::get();
    std::vector<uint8_t> res_data;
    res_data.reserve(data_.size());

    for (size_t i = 0; i < data_.size(); ++i) {
      res_data.push_back(luts.op_and[data_[i]][other.data_[i]]);
    }

    if (count_ % 4 != 0 && !res_data.empty()) {
      mask_trailing(res_data.back(), count_ % 4);
    }

    return Result<ComputeTritVector>::success(ComputeTritVector(std::move(res_data), count_));
  }

  Result<ComputeTritVector> t_or_lut(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    const auto& luts = LUTs::get();
    std::vector<uint8_t> res_data;
    res_data.reserve(data_.size());

    for (size_t i = 0; i < data_.size(); ++i) {
      res_data.push_back(luts.op_or[data_[i]][other.data_[i]]);
    }

    if (count_ % 4 != 0 && !res_data.empty()) {
      mask_trailing(res_data.back(), count_ % 4);
    }

    return Result<ComputeTritVector>::success(ComputeTritVector(std::move(res_data), count_));
  }

  // Phase 2B+ / 2C: TXor remains on LUT path for safety (non-commutative, complex logic)
  Result<ComputeTritVector> t_xor_lut(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    const auto& luts = LUTs::get();
    std::vector<uint8_t> res_data;
    res_data.reserve(data_.size());

    for (size_t i = 0; i < data_.size(); ++i) {
      res_data.push_back(luts.op_xor[data_[i]][other.data_[i]]);
    }

    if (count_ % 4 != 0 && !res_data.empty()) {
      mask_trailing(res_data.back(), count_ % 4);
    }

    return Result<ComputeTritVector>::success(ComputeTritVector(std::move(res_data), count_));
  }

  // Phase 2C: SWAR Implementations (Exposed for verification/benchmarking)
  [[deprecated("Use t81::swar::t_not_swar instead")]] Result<ComputeTritVector> t_not_swar() const {
    ComputeTritVector res = *this;
    kernel_not_swar(data_.data(), res.data_.data(), data_.size());
    if (count_ % 4 != 0 && !res.data_.empty()) {
      mask_trailing(res.data_.back(), count_ % 4);
    }
    return Result<ComputeTritVector>::success(std::move(res));
  }

  [[deprecated("Use t81::swar::t_and_swar instead")]] Result<ComputeTritVector> t_and_swar(
      const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    ComputeTritVector res = *this;
    kernel_and_swar(data_.data(), other.data_.data(), res.data_.data(), data_.size());
    if (count_ % 4 != 0 && !res.data_.empty()) {
      mask_trailing(res.data_.back(), count_ % 4);
    }
    return Result<ComputeTritVector>::success(std::move(res));
  }

  [[deprecated("Use t81::swar::t_or_swar instead")]] Result<ComputeTritVector> t_or_swar(
      const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    ComputeTritVector res = *this;
    kernel_or_swar(data_.data(), other.data_.data(), res.data_.data(), data_.size());
    if (count_ % 4 != 0 && !res.data_.empty()) {
      mask_trailing(res.data_.back(), count_ % 4);
    }
    return Result<ComputeTritVector>::success(std::move(res));
  }

  // Phase 2D: Zero-Allocation / In-Place APIs

  Result<bool> t_not_inplace() {
    kernel_not(data_.data(), data_.data(), data_.size());
    if (count_ % 4 != 0 && !data_.empty()) {
      mask_trailing(data_.back(), count_ % 4);
    }
    return Result<bool>::success(true);
  }

  Result<bool> t_and_inplace(const ComputeTritVector& other) {
    if (count_ != other.count_) {
      return Result<bool>::failure(T81Symbol::intern("LENGTH_MISMATCH"),
                                   T81String("Vectors must have same length for binary operation"),
                                   T81Symbol::intern("ComputeTritVector"));
    }
    kernel_and(data_.data(), other.data_.data(), data_.data(), data_.size());
    if (count_ % 4 != 0 && !data_.empty()) {
      mask_trailing(data_.back(), count_ % 4);
    }
    return Result<bool>::success(true);
  }

  Result<bool> t_or_inplace(const ComputeTritVector& other) {
    if (count_ != other.count_) {
      return Result<bool>::failure(T81Symbol::intern("LENGTH_MISMATCH"),
                                   T81String("Vectors must have same length for binary operation"),
                                   T81Symbol::intern("ComputeTritVector"));
    }
    kernel_or(data_.data(), other.data_.data(), data_.data(), data_.size());
    if (count_ % 4 != 0 && !data_.empty()) {
      mask_trailing(data_.back(), count_ % 4);
    }
    return Result<bool>::success(true);
  }

  Result<bool> t_xor_inplace(const ComputeTritVector& other) {
    if (count_ != other.count_) {
      return Result<bool>::failure(T81Symbol::intern("LENGTH_MISMATCH"),
                                   T81String("Vectors must have same length for binary operation"),
                                   T81Symbol::intern("ComputeTritVector"));
    }
    const auto& luts = LUTs::get();
    for (size_t i = 0; i < data_.size(); ++i) {
      data_[i] = luts.op_xor[data_[i]][other.data_[i]];
    }
    if (count_ % 4 != 0 && !data_.empty()) {
      mask_trailing(data_.back(), count_ % 4);
    }
    T81_PROFILE_RECORD("TXor", data_.size());
    return Result<bool>::success(true);
  }

  // Phase 2C/2D: Public API Wrappers (dispatch to kernels via inplace)

  Result<ComputeTritVector> t_not() const {
    ComputeTritVector res = *this;
    auto r = res.t_not_inplace();
    if (r.is_err()) return Result<ComputeTritVector>(r.error());
    return Result<ComputeTritVector>::success(std::move(res));
  }

  Result<ComputeTritVector> t_and(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    ComputeTritVector res = *this;
    auto r = res.t_and_inplace(other);
    if (r.is_err()) return Result<ComputeTritVector>(r.error());
    return Result<ComputeTritVector>::success(std::move(res));
  }

  Result<ComputeTritVector> t_or(const ComputeTritVector& other) const {
    if (count_ != other.count_) {
      return Result<ComputeTritVector>::failure(
          T81Symbol::intern("LENGTH_MISMATCH"),
          T81String("Vectors must have same length for binary operation"),
          T81Symbol::intern("ComputeTritVector"));
    }
    ComputeTritVector res = *this;
    auto r = res.t_or_inplace(other);
    if (r.is_err()) return Result<ComputeTritVector>(r.error());
    return Result<ComputeTritVector>::success(std::move(res));
  }

  Result<ComputeTritVector> t_xor(const ComputeTritVector& other) const { return t_xor_lut(other); }

private:
  ComputeTritVector(std::vector<uint8_t> data, size_t count)
      : data_(std::move(data)), count_(count) {}

  static size_t bytes_for_trits(size_t trit_count) { return (trit_count + 3) / 4; }

  static void mask_trailing(uint8_t& byte, size_t used_trits) {
    size_t used_bits = used_trits * 2;
    uint8_t mask = (1 << used_bits) - 1;
    byte &= mask;
  }

public:
  // Thresholds for dispatching to SIMD kernels.
  // Below these sizes (in bytes), the overhead of SIMD setup/tail handling
  // outweighs the throughput benefit, so we fall back to SWAR.
  // Determined via benchmarks/BM_PackedTritVector.cpp.
  static constexpr size_t AVX2_THRESHOLD_BYTES = 64;  // ~256 trits (Verified on x86_64)
  static constexpr size_t NEON_THRESHOLD_BYTES = 64;  // Baseline threshold for OR on ARM64
  static constexpr size_t AVX2_TNOT_THRESHOLD_BYTES = AVX2_THRESHOLD_BYTES;
  static constexpr size_t AVX2_TAND_THRESHOLD_BYTES = AVX2_THRESHOLD_BYTES;
  static constexpr size_t AVX2_TOR_THRESHOLD_BYTES = AVX2_THRESHOLD_BYTES;
  static constexpr size_t NEON_TNOT_THRESHOLD_BYTES = std::numeric_limits<size_t>::max();
  static constexpr size_t NEON_TAND_THRESHOLD_BYTES = std::numeric_limits<size_t>::max();
  static constexpr size_t NEON_TOR_THRESHOLD_BYTES = NEON_THRESHOLD_BYTES;

  // Inline helpers for fastpaths
  static inline void op_not_64(const uint8_t* src, uint8_t* dst) {
    uint64_t x;
    std::memcpy(&x, src, 8);
    uint64_t low = x & 0x5555555555555555ULL;
    uint64_t res = x ^ (low << 1);
    std::memcpy(dst, &res, 8);
  }

  static inline void op_and_64(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst) {
    uint64_t a, b;
    std::memcpy(&a, src_a, 8);
    std::memcpy(&b, src_b, 8);
    uint64_t H = (a | b) & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t L_content = (a & b) & 0x5555555555555555ULL;
    uint64_t res = H | (H >> 1) | L_content;
    std::memcpy(dst, &res, 8);
  }

  static inline void op_or_64(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst) {
    uint64_t a, b;
    std::memcpy(&a, src_a, 8);
    std::memcpy(&b, src_b, 8);
    uint64_t h_a = a & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t h_b = b & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t l_a = a & 0x5555555555555555ULL;
    uint64_t l_b = b & 0x5555555555555555ULL;
    uint64_t H = h_a & h_b;
    uint64_t mask = (h_a | h_b) >> 1;
    uint64_t L = (l_a & l_b) | ((l_a | l_b) & ~mask);
    uint64_t res = H | (H >> 1) | L;
    std::memcpy(dst, &res, 8);
  }

  // Fastpaths
  static void fastpath_not_tiny(const uint8_t* src, uint8_t* dst, size_t n) {
    if (n == 8) {
      op_not_64(src, dst);
      return;
    }
    // Fallback for < 8
    for (size_t i = 0; i < n; ++i) {
      uint8_t x = src[i];
      uint8_t low = x & 0x55;
      dst[i] = x ^ (low << 1);
    }
  }

  static void fastpath_not_small(const uint8_t* src, uint8_t* dst, size_t n) {
    if (n == 16) {
      op_not_64(src, dst);
      op_not_64(src + 8, dst + 8);
      return;
    }
    kernel_not_swar(src, dst, n);
  }

  static void fastpath_and_tiny(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t n) {
    if (n == 8) {
      op_and_64(a, b, out);
      return;
    }
    for (size_t i = 0; i < n; ++i) {
      uint8_t val_a = a[i];
      uint8_t val_b = b[i];
      uint8_t H = (val_a | val_b) & 0xAA;
      uint8_t L_content = (val_a & val_b) & 0x55;
      out[i] = H | (H >> 1) | L_content;
    }
  }

  static void fastpath_and_small(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t n) {
    if (n == 16) {
      op_and_64(a, b, out);
      op_and_64(a + 8, b + 8, out + 8);
      return;
    }
    kernel_and_swar(a, b, out, n);
  }

  static void fastpath_or_tiny(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t n) {
    if (n == 8) {
      op_or_64(a, b, out);
      return;
    }
    for (size_t i = 0; i < n; ++i) {
      uint8_t val_a = a[i];
      uint8_t val_b = b[i];
      uint8_t h_a = val_a & 0xAA;
      uint8_t h_b = val_b & 0xAA;
      uint8_t l_a = val_a & 0x55;
      uint8_t l_b = val_b & 0x55;
      uint8_t H = h_a & h_b;
      uint8_t mask = (h_a | h_b) >> 1;
      uint8_t L = (l_a & l_b) | ((l_a | l_b) & ~mask);
      out[i] = H | (H >> 1) | L;
    }
  }

  static void fastpath_or_small(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t n) {
    if (n == 16) {
      op_or_64(a, b, out);
      op_or_64(a + 8, b + 8, out + 8);
      return;
    }
    kernel_or_swar(a, b, out, n);
  }

  // Kernel Dispatch Layer
  static void kernel_not(const uint8_t* in, uint8_t* out, size_t len) {
    if (len <= 8) {
      fastpath_not_tiny(in, out, len);
      return;
    }
    if (len <= 16) {
      fastpath_not_small(in, out, len);
      return;
    }
    T81_PROFILE_RECORD("TNot", len);
#if defined(__x86_64__) && defined(__AVX2__)
    if (len >= AVX2_TNOT_THRESHOLD_BYTES) {
      kernel_not_avx2(in, out, len);
      return;
    }
#elif defined(__aarch64__) && defined(__ARM_NEON)
    if (len >= NEON_TNOT_THRESHOLD_BYTES) {
      kernel_not_neon(in, out, len);
      return;
    }
#endif
    kernel_not_swar(in, out, len);
  }

  static void kernel_and(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t len) {
    if (len <= 8) {
      fastpath_and_tiny(a, b, out, len);
      return;
    }
    if (len <= 16) {
      fastpath_and_small(a, b, out, len);
      return;
    }
    T81_PROFILE_RECORD("TAnd", len);
#if defined(__x86_64__) && defined(__AVX2__)
    if (len >= AVX2_TAND_THRESHOLD_BYTES) {
      kernel_and_avx2(a, b, out, len);
      return;
    }
#elif defined(__aarch64__) && defined(__ARM_NEON)
    if (len >= NEON_TAND_THRESHOLD_BYTES) {
      kernel_and_neon(a, b, out, len);
      return;
    }
#endif
    kernel_and_swar(a, b, out, len);
  }

  static void kernel_or(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t len) {
    if (len <= 8) {
      fastpath_or_tiny(a, b, out, len);
      return;
    }
    if (len <= 16) {
      fastpath_or_small(a, b, out, len);
      return;
    }
    T81_PROFILE_RECORD("TOr", len);
#if defined(__x86_64__) && defined(__AVX2__)
    if (len >= AVX2_TOR_THRESHOLD_BYTES) {
      kernel_or_avx2(a, b, out, len);
      return;
    }
#elif defined(__aarch64__) && defined(__ARM_NEON)
    if (len >= NEON_TOR_THRESHOLD_BYTES) {
      kernel_or_neon(a, b, out, len);
      return;
    }
#endif
    kernel_or_swar(a, b, out, len);
  }

  // SWAR Kernels
  static void kernel_not_swar(const uint8_t* src, uint8_t* dst, size_t n) {
    t81::swar::kernel::t_not(src, dst, n);
  }

  static void kernel_and_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    t81::swar::kernel::t_and(src_a, src_b, dst, n);
  }

  static void kernel_or_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    t81::swar::kernel::t_or(src_a, src_b, dst, n);
  }

#if defined(__aarch64__) && defined(__ARM_NEON)
  static void kernel_not_neon(const uint8_t* src, uint8_t* dst, size_t n) {
    size_t i = 0;
    uint8x16_t mask55 = vdupq_n_u8(0x55);
    for (; i + 16 <= n; i += 16) {
      uint8x16_t x = vld1q_u8(src + i);
      uint8x16_t low = vandq_u8(x, mask55);
      uint8x16_t low_sh = vshlq_n_u8(low, 1);
      uint8x16_t res = veorq_u8(x, low_sh);
      vst1q_u8(dst + i, res);
    }
    if (i < n) {
      kernel_not_swar(src + i, dst + i, n - i);
    }
  }

  static void kernel_and_neon(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    size_t i = 0;
    uint8x16_t maskAA = vdupq_n_u8(0xAA);
    uint8x16_t mask55 = vdupq_n_u8(0x55);
    for (; i + 16 <= n; i += 16) {
      uint8x16_t va = vld1q_u8(src_a + i);
      uint8x16_t vb = vld1q_u8(src_b + i);

      uint8x16_t a_or_b = vorrq_u8(va, vb);
      uint8x16_t H = vandq_u8(a_or_b, maskAA);

      uint8x16_t a_and_b = vandq_u8(va, vb);
      uint8x16_t L_content = vandq_u8(a_and_b, mask55);

      uint8x16_t H_shr = vshrq_n_u8(H, 1);
      uint8x16_t res = vorrq_u8(H, H_shr);
      res = vorrq_u8(res, L_content);
      vst1q_u8(dst + i, res);
    }
    if (i < n) {
      kernel_and_swar(src_a + i, src_b + i, dst + i, n - i);
    }
  }

  static void kernel_or_neon(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    size_t i = 0;
    uint8x16_t maskAA = vdupq_n_u8(0xAA);
    uint8x16_t mask55 = vdupq_n_u8(0x55);
    for (; i + 16 <= n; i += 16) {
      uint8x16_t va = vld1q_u8(src_a + i);
      uint8x16_t vb = vld1q_u8(src_b + i);

      uint8x16_t h_a = vandq_u8(va, maskAA);
      uint8x16_t h_b = vandq_u8(vb, maskAA);
      uint8x16_t l_a = vandq_u8(va, mask55);
      uint8x16_t l_b = vandq_u8(vb, mask55);

      uint8x16_t H = vandq_u8(h_a, h_b);
      uint8x16_t h_or = vorrq_u8(h_a, h_b);
      uint8x16_t mask = vshrq_n_u8(h_or, 1);

      uint8x16_t l_and = vandq_u8(l_a, l_b);
      uint8x16_t l_or = vorrq_u8(l_a, l_b);

      // L = (l_a & l_b) | ((l_a | l_b) & ~mask)
      // vbicq_u8(a, b) -> a & ~b
      uint8x16_t L_part2 = vbicq_u8(l_or, mask);
      uint8x16_t L = vorrq_u8(l_and, L_part2);

      uint8x16_t H_shr = vshrq_n_u8(H, 1);
      uint8x16_t res = vorrq_u8(H, H_shr);
      res = vorrq_u8(res, L);
      vst1q_u8(dst + i, res);
    }
    if (i < n) {
      kernel_or_swar(src_a + i, src_b + i, dst + i, n - i);
    }
  }
#endif

#if defined(__x86_64__) && defined(__AVX2__)
  static void kernel_not_avx2(const uint8_t* src, uint8_t* dst, size_t n) {
    size_t i = 0;
    __m256i mask55 = _mm256_set1_epi8(0x55);
    for (; i + 32 <= n; i += 32) {
      __m256i x = _mm256_loadu_si256((const __m256i*)(src + i));
      __m256i low = _mm256_and_si256(x, mask55);
      __m256i low_sh = _mm256_slli_epi64(low, 1);
      __m256i res = _mm256_xor_si256(x, low_sh);
      _mm256_storeu_si256((__m256i*)(dst + i), res);
    }
    // Tail
    if (i < n) {
      kernel_not_swar(src + i, dst + i, n - i);
    }
  }

  static void kernel_and_avx2(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    size_t i = 0;
    __m256i maskAA = _mm256_set1_epi8(static_cast<int8_t>(0xAA));
    __m256i mask55 = _mm256_set1_epi8(0x55);
    for (; i + 32 <= n; i += 32) {
      __m256i va = _mm256_loadu_si256((const __m256i*)(src_a + i));
      __m256i vb = _mm256_loadu_si256((const __m256i*)(src_b + i));

      __m256i a_or_b = _mm256_or_si256(va, vb);
      __m256i H = _mm256_and_si256(a_or_b, maskAA);

      __m256i a_and_b = _mm256_and_si256(va, vb);
      __m256i L_content = _mm256_and_si256(a_and_b, mask55);

      __m256i H_shr = _mm256_srli_epi64(H, 1);
      __m256i res = _mm256_or_si256(H, H_shr);
      res = _mm256_or_si256(res, L_content);
      _mm256_storeu_si256((__m256i*)(dst + i), res);
    }
    if (i < n) {
      kernel_and_swar(src_a + i, src_b + i, dst + i, n - i);
    }
  }

  static void kernel_or_avx2(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
    size_t i = 0;
    __m256i maskAA = _mm256_set1_epi8(static_cast<int8_t>(0xAA));
    __m256i mask55 = _mm256_set1_epi8(0x55);
    for (; i + 32 <= n; i += 32) {
      __m256i va = _mm256_loadu_si256((const __m256i*)(src_a + i));
      __m256i vb = _mm256_loadu_si256((const __m256i*)(src_b + i));

      __m256i h_a = _mm256_and_si256(va, maskAA);
      __m256i h_b = _mm256_and_si256(vb, maskAA);
      __m256i l_a = _mm256_and_si256(va, mask55);
      __m256i l_b = _mm256_and_si256(vb, mask55);

      __m256i H = _mm256_and_si256(h_a, h_b);
      __m256i h_or = _mm256_or_si256(h_a, h_b);
      __m256i mask = _mm256_srli_epi64(h_or, 1);

      __m256i l_and = _mm256_and_si256(l_a, l_b);
      __m256i l_or = _mm256_or_si256(l_a, l_b);

      // L = (l_a & l_b) | ((l_a | l_b) & ~mask)
      __m256i L_part2 = _mm256_andnot_si256(mask, l_or);
      __m256i L = _mm256_or_si256(l_and, L_part2);

      __m256i H_shr = _mm256_srli_epi64(H, 1);
      __m256i res = _mm256_or_si256(H, H_shr);
      res = _mm256_or_si256(res, L);
      _mm256_storeu_si256((__m256i*)(dst + i), res);
    }
    if (i < n) {
      kernel_or_swar(src_a + i, src_b + i, dst + i, n - i);
    }
  }
#endif

private:
  struct LUTs {
    uint8_t op_not[256];
    uint8_t op_and[256][256];
    uint8_t op_or[256][256];
    uint8_t op_xor[256][256];

    static const LUTs& get() {
      static LUTs instance;
      return instance;
    }

  private:
    LUTs() {
      for (int i = 0; i < 256; ++i) {
        op_not[i] = generate_unary(i, [](int8_t t) { return PackedTritVector::scalar_not(t); });
        for (int j = 0; j < 256; ++j) {
          op_and[i][j] = generate_binary(
              i, j, [](int8_t a, int8_t b) { return PackedTritVector::scalar_and(a, b); });
          op_or[i][j] = generate_binary(
              i, j, [](int8_t a, int8_t b) { return PackedTritVector::scalar_or(a, b); });
          op_xor[i][j] = generate_binary(
              i, j, [](int8_t a, int8_t b) { return PackedTritVector::scalar_xor(a, b); });
        }
      }
    }

    static uint8_t generate_unary(uint8_t in, int8_t (*op)(int8_t)) {
      uint8_t out = 0;
      for (int lane = 0; lane < 4; ++lane) {
        int8_t t = decode_trit(in, lane);
        if (t == 2) return 0xAA;  // Invalid input -> Invalid output pattern

        int8_t res = op(t);
        encode_trit(out, lane, res);
      }
      return out;
    }

    static uint8_t generate_binary(uint8_t a_in, uint8_t b_in, int8_t (*op)(int8_t, int8_t)) {
      uint8_t out = 0;
      for (int lane = 0; lane < 4; ++lane) {
        int8_t t_a = decode_trit(a_in, lane);
        int8_t t_b = decode_trit(b_in, lane);
        if (t_a == 2 || t_b == 2) return 0xAA;  // Invalid input -> Invalid output pattern

        int8_t res = op(t_a, t_b);
        encode_trit(out, lane, res);
      }
      return out;
    }

    // 00 -> 0, 01 -> 1, 11 -> -1, 10 -> 2 (Invalid)
    static int8_t decode_trit(uint8_t byte, int lane) {
      uint8_t val = (byte >> (lane * 2)) & 0x03;
      if (val == 0) return 0;
      if (val == 1) return 1;
      if (val == 3) return -1;
      return 2;  // Invalid
    }

    static void encode_trit(uint8_t& byte, int lane, int8_t t) {
      uint8_t val = 0;  // 00
      if (t == 1)
        val = 1;  // 01
      else if (t == -1)
        val = 3;  // 11
      // t=0 -> 00
      byte |= (val << (lane * 2));
    }
  };

  std::vector<uint8_t> data_;
  size_t count_;
};

inline Result<PackedTritVector> PackedTritVector::from_compute(const ComputeTritVector& other) {
  auto trits_res = other.to_trits();
  if (trits_res.is_err()) return Result<PackedTritVector>(trits_res.error());
  return from_trits(trits_res.value());
}

}  // namespace t81::experimental

namespace t81 {

using PackedTritVector = experimental::PackedTritVector;
using ComputeTritVector = experimental::ComputeTritVector;

namespace packed {

using PackedTritVector = t81::PackedTritVector;
using ComputeTritVector = t81::ComputeTritVector;

}  // namespace packed

}  // namespace t81

#include "t81/swar/swar.hpp"
