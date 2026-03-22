/**
 * @file string.hpp
 * @brief Standard string utilities and conversions.
 */
#pragma once

#include <sstream>
#include <vector>
#include "t81/bigint.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81String.hpp"

namespace t81::text {

using String = t81::T81String;

// String manipulation
inline ::std::vector<String> split(const String& s, char delimiter) {
  ::std::vector<String> result;
  ::std::string_view sv = s.sv();
  size_t start = 0;
  size_t end = sv.find(delimiter);
  while (end != ::std::string_view::npos) {
    result.emplace_back(sv.substr(start, end - start));
    start = end + 1;
    end = sv.find(delimiter, start);
  }
  result.emplace_back(sv.substr(start));
  return result;
}

inline String join(const ::std::vector<String>& parts, const String& delimiter) {
  if (parts.empty()) return String("");
  String result = parts[0];
  for (size_t i = 1; i < parts.size(); ++i) {
    result += delimiter;
    result += parts[i];
  }
  return result;
}

// Conversions
template <typename T>
String to_string(const T& val) {
  // Fallback to stream operator
  ::std::ostringstream oss;
  oss << val;
  return String(::std::string_view(oss.str()));
}

// Specializations/Overloads
template <size_t N>
String to_string(const T81Int<N>& val) {
  return String(::std::string_view(val.to_string()));
}

// Float formatting — native ternary-to-decimal (no host float dependency).
//
// Value model: sign * mantissa_int * 3^k,  k = exp - (M - 1)
//   mantissa_int  = the balanced-ternary integer stored in the M mantissa trits
//   exp           = T81Float::exponent()  (raw balanced-ternary exponent field)
//
// For k >= 0 the result is an exact integer computed via ::t81::T81BigInt.
// For k <  0 the result has a fractional part; exact decimal digits are
// extracted by repeated multiply-by-10 / divide-by-denominator long division,
// which terminates when the remainder is zero (exact representation) or after
// kMaxFracDigits digits (truncation for irrational-in-decimal values).
template <size_t M, size_t E>
String to_string(const T81Float<M, E>& val) {
  if (val.is_nae()) return String("NAE");
  if (val.is_inf()) return String(val.is_negative() ? "-INF" : "INF");
  if (val.is_zero()) return String("0");

  const bool negative = val.is_negative();
  const std::int64_t exp = val.exponent();
  // k = exp - (M - 1): power of 3 that scales the mantissa integer to the
  // actual value.  Negative k means the value has a fractional part.
  const std::int64_t k = exp - static_cast<std::int64_t>(M) + 1;

  // Mantissa as an arbitrary-precision integer (always non-negative for
  // normalised T81Float; sign is separate).
  ::t81::T81BigInt mantissa_big(val.mantissa());

  ::std::string result;
  if (negative) result += '-';

  if (k >= 0) {
    // Pure-integer value: mantissa_int * 3^k
    ::t81::T81BigInt int_val =
        mantissa_big * ::t81::T81BigInt::pow(::t81::T81BigInt(3), ::t81::T81BigInt(k));
    result += int_val.to_decimal_string();
  } else {
    // Mixed integer+fractional value.
    // denominator = 3^(-k); integer_part = mantissa_big / denominator;
    // remainder   = mantissa_big % denominator
    ::t81::T81BigInt denominator =
        ::t81::T81BigInt::pow(::t81::T81BigInt(3), ::t81::T81BigInt(-k));
    auto [integer_part, remainder] =
        ::t81::T81BigInt::div_mod(mantissa_big, denominator);

    result += integer_part.to_decimal_string();

    if (!remainder.is_zero()) {
      result += '.';
      // Exact decimal digit extraction.  For M=72 the maximum number of
      // fractional decimal digits needed is ceil(71 * log10(3)) == 34;
      // kMaxFracDigits = 40 gives a small guard margin.
      constexpr int kMaxFracDigits = 40;
      for (int i = 0; i < kMaxFracDigits && !remainder.is_zero(); ++i) {
        remainder = remainder * ::t81::T81BigInt(10);
        auto [digit, new_rem] = ::t81::T81BigInt::div_mod(remainder, denominator);
        result += static_cast<char>('0' + static_cast<int>(digit.to_int64()));
        remainder = new_rem;
      }
    }
  }

  return String(::std::string_view(result));
}

}  // namespace t81::text
