#include "t81/codec/metadata.hpp"
#include <cstring>

namespace t81::codec::metadata {

constexpr uint32_t kMagic = 0x43313854;  // 'T81C' in little-endian

std::vector<uint8_t> wrap_encoded_buffer(EncodingType encoding, uint64_t trit_count,
                                         std::span<const uint8_t> payload) {
  std::vector<uint8_t> result(16 + payload.size());
  Header h;
  h.magic = kMagic;
  h.version = 1;
  h.encoding = encoding;
  h.flags = 0;
  h.trit_count = trit_count;

  std::memcpy(result.data(), &h, sizeof(Header));
  std::memcpy(result.data() + 16, payload.data(), payload.size());
  return result;
}

Result<std::pair<Header, std::vector<uint8_t>>> unwrap_encoded_buffer(
    std::span<const uint8_t> buffer) {
  if (buffer.size() < 16) {
    return T81Error(T81Symbol::intern("INSUFFICIENT_DATA"),
                    T81String("Buffer too small for header"));
  }

  Header h;
  std::memcpy(&h, buffer.data(), sizeof(Header));

  if (h.magic != kMagic) {
    return T81Error(T81Symbol::intern("INVALID_MAGIC"), T81String("Invalid metadata magic"));
  }

  if (h.version != 1) {
    return T81Error(T81Symbol::intern("UNSUPPORTED_VERSION"),
                    T81String("Unsupported metadata version"));
  }

  std::vector<uint8_t> payload(buffer.begin() + 16, buffer.end());
  return std::make_pair(h, std::move(payload));
}

}  // namespace t81::codec::metadata
