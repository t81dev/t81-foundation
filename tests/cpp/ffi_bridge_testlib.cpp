#include <cstdint>
#include <cstring>

namespace t81 {
namespace ffi {

struct FFIByteSpanResult {
  const std::uint8_t* data;
  std::uint32_t size;
};

struct FFIStringListResult {
  const char* const* items;
  std::uint32_t count;
};

}  // namespace ffi
}  // namespace t81

extern "C" std::int64_t t81_ffi_add_i64(std::int64_t lhs, std::int64_t rhs) { return lhs + rhs; }

extern "C" std::int64_t t81_ffi_strlen_bridge(const char* text) {
  return static_cast<std::int64_t>(text ? std::strlen(text) : 0);
}

extern "C" const char* t81_ffi_hello_bridge() { return "bridge-ok"; }

extern "C" double t81_ffi_half_double(double value) { return value / 2.0; }

extern "C" double t81_ffi_fixed_double() { return 1.5; }

extern "C" std::int64_t t81_ffi_bytes_checksum(const std::uint8_t* data, std::uint32_t size) {
  std::int64_t sum = 0;
  for (std::uint32_t i = 0; i < size; ++i) {
    sum += static_cast<std::int64_t>(data[i]);
  }
  return sum;
}

extern "C" t81::ffi::FFIByteSpanResult t81_ffi_bytes_result_bridge() {
  static const std::uint8_t payload[] = {0x41, 0x00, 0x42, 0x7f};
  t81::ffi::FFIByteSpanResult result{};
  result.data = payload;
  result.size = static_cast<std::uint32_t>(sizeof(payload));
  return result;
}

extern "C" std::int64_t t81_ffi_mix_i64_strlen(std::int64_t base, const char* text) {
  return base + static_cast<std::int64_t>(text ? std::strlen(text) : 0);
}

extern "C" t81::ffi::FFIStringListResult t81_ffi_pair_strings_bridge() {
  static const char* payload[] = {"alpha", "beta"};
  t81::ffi::FFIStringListResult result{};
  result.items = payload;
  result.count = 2;
  return result;
}

extern "C" std::int64_t t81_ffi_string_list_total_len(const char* const* items,
                                                      std::uint32_t count) {
  std::int64_t total = 0;
  for (std::uint32_t i = 0; i < count; ++i) {
    total += static_cast<std::int64_t>(items && items[i] ? std::strlen(items[i]) : 0);
  }
  return total;
}

extern "C" std::int64_t t81_ffi_quarantined_probe() { return 99; }
