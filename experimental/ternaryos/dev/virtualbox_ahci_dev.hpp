#pragma once

// experimental/ternaryos/dev/virtualbox_ahci_dev.hpp
//
// VirtualBox-first AHCI block device scaffold for TernOS Phase 4.
//
// This adapter wraps an existing IBlockDevice and exposes the first supported
// VirtualBox storage profile as an AHCI-shaped boundary. The underlying storage
// remains hosted for now; the purpose is to move the codebase from "generic
// hosted block file" toward "VBox AHCI guest device".

#include "block_device.hpp"

#include <cstdint>
#include <string>

namespace t81::ternaryos::dev {

struct VBoxAhciInfo {
  uint64_t abar_base{0xF0400000ULL};
  uint64_t abar_span_bytes{0x2000ULL};
  uint8_t  irq{19};
  uint8_t  port_count{1};
  bool     bootable{true};
};

class VirtualBoxAhciDev final : public IBlockDevice {
public:
  explicit VirtualBoxAhciDev(IBlockDevice& backing,
                             std::string device_id = "vbox-ahci0");

  BlockDeviceInfo info() const noexcept override;
  uint64_t block_count() const noexcept override;
  bool read_block(uint64_t lba, BlockData& out) const override;
  bool write_block(uint64_t lba, const BlockData& data) override;
  bool flush() override;

  const VBoxAhciInfo& ahci_info() const noexcept { return ahci_; }
  uint64_t read_ops() const noexcept { return read_ops_; }
  uint64_t write_ops() const noexcept { return write_ops_; }
  uint64_t flush_ops() const noexcept { return flush_ops_; }

private:
  IBlockDevice& backing_;
  std::string   device_id_;
  VBoxAhciInfo  ahci_{};
  mutable uint64_t read_ops_{0};
  uint64_t write_ops_{0};
  uint64_t flush_ops_{0};
};

}  // namespace t81::ternaryos::dev
