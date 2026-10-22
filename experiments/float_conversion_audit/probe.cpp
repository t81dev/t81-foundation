// Audit adapter: calls the unchanged production conversion, then diagnoses the
// legacy host intermediates. No host-derived value is a reference expectation.
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <string>
#include "t81/types/T81Float.hpp"

int main() {
  static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);
  std::locale::global(std::locale::classic());
  std::cerr << "compiler=" <<
#ifdef __clang__
      "Clang " << __clang_version__
#elif defined(__GNUC__)
      "GCC " << __VERSION__
#else
      "unknown"
#endif
            << ";long_double_digits=" << std::numeric_limits<long double>::digits
            << ";long_double_max_exponent=" << std::numeric_limits<long double>::max_exponent
            << '\n';
  std::string token;
  while (std::cin >> token) {
    std::uint64_t bits = 0;
    auto parsed = std::from_chars(token.data(), token.data() + token.size(), bits, 16);
    if (token.size() != 16 || parsed.ec != std::errc{} ||
        parsed.ptr != token.data() + token.size()) {
      std::cerr << "invalid binary64 hex input\n";
      return 2;
    }
    const double value = std::bit_cast<double>(bits);
    const auto actual = t81::v1::T81Float<27, 9>::from_double(value).to_canonical_string();
    std::cout << token << '\t' << actual;
    if (std::isfinite(value) && value != 0.0) {
      const long double magnitude = std::fabs(static_cast<long double>(value));
      const auto exponent =
          static_cast<std::int64_t>(std::floor(std::log(magnitude) / std::log(3.0L))) + 1;
      // Same usual arithmetic conversions as production: unsigned subtraction.
      const auto buggy_scale = std::size_t{26} - exponent;
      const auto signed_scale = std::int64_t{26} - exponent;
      const auto power = std::pow(3.0L, static_cast<long double>(buggy_scale));
      const auto signed_power = std::pow(3.0L, static_cast<long double>(signed_scale));
      std::cout << '\t' << exponent << '\t' << buggy_scale << '\t' << signed_scale << '\t'
                << std::isfinite(power) << '\t' << std::isfinite(signed_power) << '\t'
                << std::isfinite(magnitude * signed_power);
    } else {
      std::cout << "\tNA\tNA\tNA\tNA\tNA\tNA";
    }
    std::cout << '\n';
  }
}
