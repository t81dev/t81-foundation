// experimental/ternaryos/dev/hosted_block_dev.cpp

#include "hosted_block_dev.hpp"

#include <cstring>
#include <fstream>
#include <iterator>

namespace t81::ternaryos::dev {

HostedBlockDev::HostedBlockDev(uint64_t n_blocks, std::string device_id)
    : blocks_(n_blocks, BlockData{}), device_id_(std::move(device_id)) {}

BlockDeviceInfo HostedBlockDev::info() const noexcept {
  return BlockDeviceInfo{
      .total_blocks    = static_cast<uint64_t>(blocks_.size()),
      .block_size_bytes = kBlockSize,
      .read_only       = false,
      .device_id       = device_id_,
  };
}

bool HostedBlockDev::read_block(uint64_t lba, BlockData& out) const {
  if (lba >= blocks_.size()) return false;
  out = blocks_[lba];
  return true;
}

bool HostedBlockDev::write_block(uint64_t lba, const BlockData& data) {
  if (lba >= blocks_.size()) return false;
  blocks_[lba] = data;
  return true;
}

bool HostedBlockDev::flush() {
  if (backing_file_.empty()) return true;
  return save(backing_file_);
}

bool HostedBlockDev::save(const std::string& path) const {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f.is_open()) return false;

  // Header: magic[4] + n_blocks[8 LE]
  f.write(reinterpret_cast<const char*>(kMagic), 4);
  uint64_t n = static_cast<uint64_t>(blocks_.size());
  f.write(reinterpret_cast<const char*>(&n), sizeof(n));

  // Block data
  for (const auto& blk : blocks_) {
    f.write(reinterpret_cast<const char*>(blk.data()), kBlockSize);
  }
  return f.good();
}

std::optional<HostedBlockDev> HostedBlockDev::load(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) return std::nullopt;

  // Validate magic
  uint8_t magic[4];
  f.read(reinterpret_cast<char*>(magic), 4);
  if (!f.good() ||
      magic[0] != kMagic[0] || magic[1] != kMagic[1] ||
      magic[2] != kMagic[2] || magic[3] != kMagic[3]) {
    return std::nullopt;
  }

  // Read n_blocks
  uint64_t n = 0;
  f.read(reinterpret_cast<char*>(&n), sizeof(n));
  if (!f.good() || n == 0 || n > (1u << 20)) return std::nullopt;

  HostedBlockDev dev(n, path);
  for (uint64_t i = 0; i < n; ++i) {
    f.read(reinterpret_cast<char*>(dev.blocks_[i].data()), kBlockSize);
    if (!f.good()) return std::nullopt;
  }
  return dev;
}

}  // namespace t81::ternaryos::dev
