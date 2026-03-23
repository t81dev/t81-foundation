// experimental/ternaryos/dev/virtualbox_e1000_dev.cpp

#include "virtualbox_e1000_dev.hpp"

#include <utility>

namespace t81::ternaryos::dev {

VirtualBoxE1000Dev::VirtualBoxE1000Dev(std::string device_id)
    : device_id_(std::move(device_id)) {}

std::optional<std::vector<uint8_t>> VirtualBoxE1000Dev::send_packet(
    const TernaryEthernetPacket& pkt) {
  auto frame = pkt.to_frame();
  if (!frame.has_value()) return std::nullopt;

  tx_bytes_ += frame->size();
  ++tx_frames_;
  tx_queue_.push_back(*frame);
  return frame;
}

bool VirtualBoxE1000Dev::inject_frame(std::vector<uint8_t> frame) {
  auto parsed = TernaryEthernetPacket::from_frame(frame);
  if (!parsed.has_value()) return false;

  rx_bytes_ += frame.size();
  ++rx_frames_;
  rx_queue_.push_back(std::move(frame));
  return true;
}

std::optional<TernaryEthernetPacket> VirtualBoxE1000Dev::receive_packet() {
  if (rx_queue_.empty()) return std::nullopt;

  auto frame = std::move(rx_queue_.front());
  rx_queue_.erase(rx_queue_.begin());
  return TernaryEthernetPacket::from_frame(frame);
}

}  // namespace t81::ternaryos::dev
