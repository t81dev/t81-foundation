#pragma once

#include "t81/packed_trit_vector.hpp"
#include "t81/types/Result.hpp"

namespace t81::tritwise {

using t81::ComputeTritVector;
using t81::PackedTritVector;

/**
 * @brief Tritwise AND operation (In-Place).
 *
 * Computes the element-wise ternary AND of two packed vectors.
 * Dispatches to SIMD kernels (AVX2/NEON) for vectors >= 256 trits (64 bytes).
 * Ensures deterministic byte identity across all backends.
 *
 * Storage vs Compute: This API operates on ComputeTritVector (2-bit packed).
 * Use ComputeTritVector::from_pt5() to convert from storage format.
 *
 * @param dst Destination vector (modified in-place).
 * @param src Source vector to AND with destination.
 * @return Result<bool> Success or error.
 */
inline Result<bool> tritwise_and(ComputeTritVector& dst, const ComputeTritVector& src) {
  return dst.t_and_inplace(src);
}

/**
 * @brief Tritwise OR operation (In-Place).
 *
 * Computes the element-wise ternary OR of two packed vectors.
 * Dispatches to SIMD kernels (AVX2/NEON) for vectors >= 256 trits (64 bytes).
 * Ensures deterministic byte identity across all backends.
 *
 * @param dst Destination vector (modified in-place).
 * @param src Source vector to OR with destination.
 * @return Result<bool> Success or error.
 */
inline Result<bool> tritwise_or(ComputeTritVector& dst, const ComputeTritVector& src) {
  return dst.t_or_inplace(src);
}

/**
 * @brief Tritwise NOT operation (In-Place).
 *
 * Computes the element-wise ternary NOT of a packed vector.
 * Dispatches to SIMD kernels (AVX2/NEON) for vectors >= 256 trits (64 bytes).
 * Ensures deterministic byte identity across all backends.
 *
 * @param dst Vector to negate (modified in-place).
 * @return Result<bool> Success or error.
 */
inline Result<bool> tritwise_not(ComputeTritVector& dst) { return dst.t_not_inplace(); }

/**
 * @brief Tritwise XOR operation (In-Place).
 *
 * Computes the element-wise ternary XOR (difference) of two packed vectors.
 * NOTE: This operation MUST route to the LUT fallback path. SIMD optimization
 * is explicitly disabled for TXor due to non-commutative semantics and safety constraints.
 *
 * @param dst Destination vector (modified in-place).
 * @param src Source vector to XOR with destination.
 * @return Result<bool> Success or error.
 */
inline Result<bool> tritwise_xor(ComputeTritVector& dst, const ComputeTritVector& src) {
  return dst.t_xor_inplace(src);
}

// By-value convenience variants

/**
 * @brief Tritwise AND operation (By-Value).
 */
inline Result<ComputeTritVector> tritwise_and(const ComputeTritVector& a,
                                              const ComputeTritVector& b) {
  return a.t_and(b);
}

/**
 * @brief Tritwise OR operation (By-Value).
 */
inline Result<ComputeTritVector> tritwise_or(const ComputeTritVector& a,
                                             const ComputeTritVector& b) {
  return a.t_or(b);
}

/**
 * @brief Tritwise NOT operation (By-Value).
 */
inline Result<ComputeTritVector> tritwise_not(const ComputeTritVector& a) { return a.t_not(); }

/**
 * @brief Tritwise XOR operation (By-Value).
 * Routes to LUT fallback.
 */
inline Result<ComputeTritVector> tritwise_xor(const ComputeTritVector& a,
                                              const ComputeTritVector& b) {
  return a.t_xor(b);  // Uses t_xor_lut internally
}

}  // namespace t81::tritwise
