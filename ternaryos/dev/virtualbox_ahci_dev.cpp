// experimental/ternaryos/dev/virtualbox_ahci_dev.cpp

#include "virtualbox_ahci_dev.hpp"

namespace t81::ternaryos::dev {

VirtualBoxAhciDev::VirtualBoxAhciDev(IBlockDevice& backing, std::string device_id)
    : backing_(backing), device_id_(std::move(device_id)) {}

BlockDeviceInfo VirtualBoxAhciDev::info() const noexcept {
  auto meta = backing_.info();
  meta.device_id = device_id_;
  return meta;
}

uint64_t VirtualBoxAhciDev::block_count() const noexcept {
  return backing_.block_count();
}

bool VirtualBoxAhciDev::read_block(uint64_t lba, BlockData& out) const {
  if (!backing_.read_block(lba, out)) return false;
  ++read_ops_;
  return true;
}

bool VirtualBoxAhciDev::write_block(uint64_t lba, const BlockData& data) {
  if (!backing_.write_block(lba, data)) return false;
  ++write_ops_;
  return true;
}

bool VirtualBoxAhciDev::flush() {
  if (!backing_.flush()) return false;
  ++flush_ops_;
  return true;
}

}  // namespace t81::ternaryos::dev
