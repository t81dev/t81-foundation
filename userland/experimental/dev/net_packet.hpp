#pragma once

// experimental/ternaryos/dev/net_packet.hpp
//
// Ternary Ethernet packet wrapper for TernOS Phase 4.
// RFC-00B2 §5.
//
// Bridges binary IEEE 802.3 Ethernet headers to ternary payloads.
// trit_payload.size() must be a multiple of 3 (ternary word alignment).
// content_ref is the CanonHash of the trit_payload bytes for Axion audit.

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "t81/canonfs/canon_types.hpp"
#include "t81/tracing/canonhash.hpp"

namespace t81::ternaryos::dev {

/**
 * @brief Ternary Ethernet packet.
 *
 * The binary Ethernet header fields (dst_mac, src_mac, ethertype) are
 * preserved verbatim for compatibility with existing NIC hardware.
 * The payload is a balanced-ternary sequence; each element is in {-1,0,+1}.
 */
struct TernaryEthernetPacket {
  std::array<uint8_t, 6> dst_mac{};
  std::array<uint8_t, 6> src_mac{};
  uint16_t               ethertype{0x0081};  ///< 0x0081 = T81 experimental
  std::vector<int8_t>    trit_payload;        ///< values in {-1,0,+1}
  t81::canonfs::CanonRef content_ref;         ///< hash of trit_payload

  /// Validate trit_payload:
  ///   - size must be a multiple of 3
  ///   - every element must be in {-1, 0, +1}
  bool valid() const noexcept {
    if (trit_payload.size() % 3 != 0) return false;
    for (auto t : trit_payload) {
      if (t < -1 || t > 1) return false;
    }
    return true;
  }

  /// Build a packet and compute content_ref from trit_payload.
  /// Returns nullopt if !valid().
  static std::optional<TernaryEthernetPacket> build(
      std::array<uint8_t,6> dst,
      std::array<uint8_t,6> src,
      uint16_t              etype,
      std::vector<int8_t>   payload) {
    TernaryEthernetPacket pkt;
    pkt.dst_mac      = dst;
    pkt.src_mac      = src;
    pkt.ethertype    = etype;
    pkt.trit_payload = std::move(payload);
    if (!pkt.valid()) return std::nullopt;

    // Compute CanonRef over the raw trit bytes (cast to uint8_t for hashing).
    std::vector<uint8_t> raw(pkt.trit_payload.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
      raw[i] = static_cast<uint8_t>(pkt.trit_payload[i] & 0xFF);
    }
    pkt.content_ref = t81::canonfs::CanonRef{
        t81::canonfs::CanonHash{t81::hash::hash_bytes(raw)}};
    return pkt;
  }

  /// Payload size in ternary words (groups of 3 trits).
  std::size_t trit_word_count() const noexcept {
    return trit_payload.size() / 3;
  }

  /// Serialize to a binary Ethernet-like frame:
  ///   dst[6] + src[6] + ethertype[2 BE] + trit_count[2 BE] + payload bytes
  std::optional<std::vector<uint8_t>> to_frame() const {
    if (!valid()) return std::nullopt;
    if (trit_payload.size() > 0xFFFFu) return std::nullopt;

    std::vector<uint8_t> frame;
    frame.reserve(16 + trit_payload.size());
    frame.insert(frame.end(), dst_mac.begin(), dst_mac.end());
    frame.insert(frame.end(), src_mac.begin(), src_mac.end());
    frame.push_back(static_cast<uint8_t>((ethertype >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(ethertype & 0xFF));

    const uint16_t trit_count = static_cast<uint16_t>(trit_payload.size());
    frame.push_back(static_cast<uint8_t>((trit_count >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(trit_count & 0xFF));

    for (int8_t t : trit_payload) {
      frame.push_back(static_cast<uint8_t>(t + 1));  // {-1,0,+1} -> {0,1,2}
    }
    return frame;
  }

  /// Parse a binary Ethernet-like frame produced by to_frame().
  static std::optional<TernaryEthernetPacket> from_frame(
      const std::vector<uint8_t>& frame) {
    if (frame.size() < 16) return std::nullopt;

    const uint16_t trit_count =
        static_cast<uint16_t>((static_cast<uint16_t>(frame[14]) << 8) | frame[15]);
    if (frame.size() != static_cast<std::size_t>(16 + trit_count)) {
      return std::nullopt;
    }

    TernaryEthernetPacket pkt;
    std::copy_n(frame.begin(), 6, pkt.dst_mac.begin());
    std::copy_n(frame.begin() + 6, 6, pkt.src_mac.begin());
    pkt.ethertype =
        static_cast<uint16_t>((static_cast<uint16_t>(frame[12]) << 8) | frame[13]);

    pkt.trit_payload.reserve(trit_count);
    for (std::size_t i = 16; i < frame.size(); ++i) {
      const uint8_t raw = frame[i];
      if (raw > 2) return std::nullopt;
      pkt.trit_payload.push_back(static_cast<int8_t>(raw) - 1);
    }
    if (!pkt.valid()) return std::nullopt;

    std::vector<uint8_t> raw(pkt.trit_payload.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
      raw[i] = static_cast<uint8_t>(pkt.trit_payload[i] & 0xFF);
    }
    pkt.content_ref = t81::canonfs::CanonRef{
        t81::canonfs::CanonHash{t81::hash::hash_bytes(raw)}};
    return pkt;
  }
};

}  // namespace t81::ternaryos::dev
