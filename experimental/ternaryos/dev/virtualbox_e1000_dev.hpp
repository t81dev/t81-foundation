#pragma once

// experimental/ternaryos/dev/virtualbox_e1000_dev.hpp
//
// VirtualBox-first E1000 network adapter scaffold for TernOS Phase 4.
// This is still a hosted simulation wrapper, but it fixes the NIC-facing
// boundary in code so the VirtualBox promotion path is explicit.

#include "net_packet.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::dev {

struct VBoxE1000Info {
  uint64_t abar_base{0xF0200000ULL};
  uint64_t abar_span_bytes{0x20000ULL};
  uint8_t  irq{11};
  std::array<uint8_t, 6> mac{{0x08, 0x00, 0x27, 0x81, 0x00, 0x01}};
  bool     link_up{true};
};

class VirtualBoxE1000Dev final {
public:
  explicit VirtualBoxE1000Dev(std::string device_id = "vbox-e10000");

  const std::string& device_id() const noexcept { return device_id_; }
  const VBoxE1000Info& e1000_info() const noexcept { return info_; }

  std::size_t tx_frames() const noexcept { return tx_frames_; }
  std::size_t rx_frames() const noexcept { return rx_frames_; }
  std::size_t tx_bytes() const noexcept { return tx_bytes_; }
  std::size_t rx_bytes() const noexcept { return rx_bytes_; }
  std::size_t pending_tx_frames() const noexcept { return tx_queue_.size(); }
  std::size_t pending_rx_frames() const noexcept { return rx_queue_.size(); }

  std::optional<std::vector<uint8_t>> send_packet(const TernaryEthernetPacket& pkt);
  bool inject_frame(std::vector<uint8_t> frame);
  std::optional<TernaryEthernetPacket> receive_packet();

private:
  VBoxE1000Info                         info_{};
  std::string                           device_id_;
  std::vector<std::vector<uint8_t>>     tx_queue_;
  std::vector<std::vector<uint8_t>>     rx_queue_;
  std::size_t                           tx_frames_{0};
  std::size_t                           rx_frames_{0};
  std::size_t                           tx_bytes_{0};
  std::size_t                           rx_bytes_{0};
};

}  // namespace t81::ternaryos::dev
