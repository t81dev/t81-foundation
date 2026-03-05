#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include "t81/types/Result.hpp"
#include "t81/types/T81Int.hpp"

namespace t81::codec::base81_balanced {

/**
 * @brief Pack 4 trits into a single balanced Base-81 digit.
 *
 * digit = t0*1 + t1*3 + t2*9 + t3*27
 * Result is in range [-40, 40].
 */
int8_t pack_digit(const Trit trits[4]);

/**
 * @brief Unpack a single balanced Base-81 digit into 4 trits.
 */
void unpack_digit(int8_t digit, Trit trits[4]);

/**
 * @brief Pack a stream of trits into balanced Base-81 digits.
 *
 * @param trits Input trits.
 * @param digits Output digits buffer. Must be large enough (trits.size() + 3) / 4.
 * @return Result<size_t> Number of digits written.
 */
Result<size_t> pack(std::span<const Trit> trits, std::span<int8_t> digits);

/**
 * @brief Unpack balanced Base-81 digits into a stream of trits.
 *
 * @param digits Input digits in range [-40, 40].
 * @param trits Output trits buffer.
 * @param trit_count Number of trits to extract.
 * @return Result<size_t> Number of trits written.
 */
Result<size_t> unpack(std::span<const int8_t> digits, std::span<Trit> trits, size_t trit_count);

/**
 * @brief Helper to pack a vector of trits into a vector of balanced digits.
 */
Result<std::vector<int8_t>> pack_vector(const std::vector<Trit>& trits);

/**
 * @brief Helper to unpack a vector of balanced digits into a vector of trits.
 */
Result<std::vector<Trit>> unpack_vector(const std::vector<int8_t>& digits, size_t trit_count);

/**
 * @brief Convert unbalanced digits (0..80) to balanced digits (-40..40).
 * balanced = unbalanced - 40
 */
void to_balanced(std::span<const uint8_t> unbalanced, std::span<int8_t> balanced);

/**
 * @brief Convert balanced digits (-40..40) to unbalanced digits (0..80).
 * unbalanced = balanced + 40
 */
void to_unbalanced(std::span<const int8_t> balanced, std::span<uint8_t> unbalanced);

}  // namespace t81::codec::base81_balanced
