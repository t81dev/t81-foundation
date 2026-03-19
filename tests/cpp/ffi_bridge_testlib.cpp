#include <cstdint>
#include <cstring>

struct t81_ffi_bytes_result {
  const std::uint8_t* data;
  std::uint32_t size;
};

struct t81_ffi_string_list_result {
  const char* const* items;
  std::uint32_t count;
};

extern "C" std::int64_t t81_ffi_add_i64(std::int64_t lhs, std::int64_t rhs) {
  return lhs + rhs;
}

extern "C" std::int64_t t81_ffi_strlen_bridge(const char* text) {
  return static_cast<std::int64_t>(text ? std::strlen(text) : 0);
}

extern "C" const char* t81_ffi_hello_bridge() {
  return "bridge-ok";
}

extern "C" double t81_ffi_half_double(double value) {
  return value / 2.0;
}

extern "C" double t81_ffi_fixed_double() {
  return 1.5;
}

extern "C" std::int64_t t81_ffi_bytes_checksum(const std::uint8_t* data, std::uint32_t size) {
  std::int64_t sum = 0;
  for (std::uint32_t i = 0; i < size; ++i) {
    sum += static_cast<std::int64_t>(data[i]);
  }
  return sum;
}

extern "C" t81_ffi_bytes_result t81_ffi_bytes_result_bridge() {
  static const std::uint8_t payload[] = {0x41, 0x00, 0x42, 0x7f};
  t81_ffi_bytes_result result{};
  result.data = payload;
  result.size = static_cast<std::uint32_t>(sizeof(payload));
  return result;
}

extern "C" std::int64_t t81_ffi_mix_i64_strlen(std::int64_t base, const char* text) {
  return base + static_cast<std::int64_t>(text ? std::strlen(text) : 0);
}

extern "C" t81_ffi_string_list_result t81_ffi_pair_strings_bridge() {
  static const char* payload[] = {"alpha", "beta"};
  t81_ffi_string_list_result result{};
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

extern "C" std::int64_t t81_ffi_quarantined_probe() {
  return 99;
}
