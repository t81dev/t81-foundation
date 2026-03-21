#pragma once

// experimental/ternaryos/dev/hosted_block_dev.hpp
//
// File-backed in-memory block device for TernOS hosted simulation.
// RFC-00B2 §2.3.
//
// Usage:
//   auto dev = HostedBlockDev::create(64);        // 64-block in-memory device
//   dev.write_block(0, data);
//   dev.save("/tmp/ternos.blk");                  // flush to file (simulated NVMe)
//   auto dev2 = HostedBlockDev::load("/tmp/ternos.blk");
//   dev2.read_block(0, out);                      // survives "reboot"

#include "block_device.hpp"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace t81::ternaryos::dev {

class HostedBlockDev final : public IBlockDevice {
public:
  /// Create an in-memory device with `n_blocks` zero-initialised blocks.
  explicit HostedBlockDev(uint64_t n_blocks, std::string device_id = "hosted");

  // ── IBlockDevice ────────────────────────────────────────────────────────────
  BlockDeviceInfo info() const noexcept override;
  uint64_t block_count() const noexcept override { return blocks_.size(); }
  bool read_block (uint64_t lba, BlockData& out) const override;
  bool write_block(uint64_t lba, const BlockData& data) override;
  bool flush() override;  ///< Persists to backing file if one was set.

  // ── Persistence ─────────────────────────────────────────────────────────────

  /// Write all blocks to `path` (raw binary: header + block data).
  /// Format: magic[4] + n_blocks[8 LE] + blocks[n * kBlockSize]
  bool save(const std::string& path) const;

  /// Load a previously saved device from `path`.
  /// Returns nullopt if the file is missing, corrupt, or wrong magic.
  static std::optional<HostedBlockDev> load(const std::string& path);

  /// Set a backing-file path so that flush() automatically calls save().
  void set_backing_file(std::string path) { backing_file_ = std::move(path); }

private:
  std::vector<BlockData> blocks_;
  std::string            device_id_;
  std::string            backing_file_;

  static constexpr uint8_t kMagic[4] = {'H', 'B', 'D', '1'};
};

}  // namespace t81::ternaryos::dev
