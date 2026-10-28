// Standalone H-CONV-01 research; standard library + extracted scalar arithmetic.
#include <istream>
#include "fixed_extracted.hpp"
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

using Fixed = t81::core::detail::DFixed;
using Storage = Fixed::Storage;
using Wide = t81::T81Int<384>;

// Copied from tensor.hpp's private helper, with no tensor construction.
static Fixed fixed_from_int64_(std::int64_t value) {
  Storage storage(value);
  storage <<= Fixed::kFractionalTrits;
  return Fixed(storage);
}

// Existing algorithm: body faithfully extracted from canonical_fixed_from_float_.
[[gnu::noinline]] static std::optional<Fixed> repeated(float value) {
  if (!std::isfinite(value)) return std::nullopt;
  std::uint32_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  const bool negative = (bits >> 31U) != 0U;
  const std::uint32_t exponent = (bits >> 23U) & 0xFFU;
  const std::uint32_t fraction = bits & 0x7FFFFFU;
  if (exponent == 0U && fraction == 0U) return Fixed::zero();
  std::uint64_t mantissa = 0;
  int binary_exponent = 0;
  if (exponent == 0U) {
    mantissa = fraction;
    binary_exponent = -149;
  } else {
    mantissa = (1ULL << 23U) | static_cast<std::uint64_t>(fraction);
    binary_exponent = static_cast<int>(exponent) - 150;
  }
  Fixed out = fixed_from_int64_(static_cast<std::int64_t>(mantissa));
  const Fixed two(2);
  if (binary_exponent > 0) {
    for (int i = 0; i < binary_exponent; ++i) out = out * two;
  } else if (binary_exponent < 0) {
    for (int i = 0; i < -binary_exponent; ++i) out = out / two;
  }
  if (negative) out = -out;
  return out;
}

// One alternative: direct integer scaling with a single final quotient.
// The positive-exponent multiply branch is deliberately the same as B.
[[gnu::noinline]] static std::optional<Fixed> direct(float value) {
  const auto bits = std::bit_cast<std::uint32_t>(value);
  const unsigned exp = (bits >> 23) & 255;
  const unsigned fraction = bits & 0x7fffff;
  if (exp == 255) return std::nullopt;
  if (exp == 0 && fraction == 0) return Fixed::zero();
  const auto mantissa = static_cast<std::int64_t>(exp ? fraction + (1U << 23) : fraction);
  const int e = exp ? static_cast<int>(exp)-150 : -149;
  Fixed out = fixed_from_int64_(mantissa);
  if (e < 0) {
    Wide denominator(1);
    // All setup charged to C; no cached denominators or lookup tables.
    for (int i=0; i<-e; ++i) denominator += denominator;
    Wide quotient = Wide(out.v) / denominator;
    out = Fixed(Storage(quotient));
  } else if (e > 0) {
    const Fixed two(2);
    for (int i=0; i<e; ++i) out = out * two;
  }
  if (bits >> 31) out = -out;
  return out;
}

static std::string packed(const std::optional<Fixed>& value) {
  if (!value) return "nonfinite";
  std::ostringstream out;
  for (auto byte : value->v.raw_data())
    out << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
  return out.str();
}

int main(int argc, char** argv) {
  try {
    if (argc < 3) throw std::runtime_error("native check|bench corpus.hex [schedule.txt]");
    std::ifstream file(argv[2]);
    if (!file) throw std::runtime_error("Missing corpus");
    std::vector<std::uint32_t> bits;
    std::string line;
    while (std::getline(file,line)) bits.push_back(static_cast<std::uint32_t>(std::stoul(line,nullptr,16)));
    if (std::string(argv[1]) == "check") {
      for (auto b : bits) {
        float x = std::bit_cast<float>(b);
        auto a = repeated(x), c = direct(x);
        std::cout << std::hex << std::setfill('0') << std::setw(8) << b << ' '
                  << packed(a) << ' ' << packed(c) << '\n' << std::flush;
      }
      return 0;
    }
    if (std::string(argv[1]) != "bench" || argc != 4) throw std::runtime_error("Invalid operation");
    auto batch = [&](char method) {
      std::uint64_t digest = 1469598103934665603ULL;
      for (auto b : bits) {
        float x = std::bit_cast<float>(b);
        auto value = method=='B' ? repeated(x) : direct(x);
        if (value) for (auto byte : value->v.raw_data()) digest = (digest ^ byte)*1099511628211ULL;
        else digest = (digest ^ 255)*1099511628211ULL;
      }
      return digest;
    };
    // Both algorithms consume the entire identical corpus once before timing.
    const auto warm_b = batch('B'), warm_c = batch('C');
    if (warm_b != warm_c) throw std::runtime_error("Warmup digest mismatch");
    std::cout << "warmup " << warm_b << ' ' << warm_c << '\n' << std::flush;
    std::ifstream schedule(argv[3]);
    if (!schedule) throw std::runtime_error("Missing schedule");
    std::size_t pair=0;
    while (std::getline(schedule,line)) {
      for (char method : line) {
        if (method!='B' && method!='C') throw std::runtime_error("Bad schedule");
        const auto begin = std::chrono::steady_clock::now();
        const auto digest = batch(method);
        const auto end = std::chrono::steady_clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
        if (digest != warm_b || ns <= 0) throw std::runtime_error("Timed result/clock mismatch");
        std::cout << pair << ' ' << method << ' ' << ns << ' ' << bits.size() << ' ' << digest << '\n' << std::flush;
      }
      ++pair;
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
