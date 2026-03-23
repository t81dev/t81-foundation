#pragma once

// experimental/ternaryos/dev/block_device.hpp
//
// Abstract block device interface for TernOS Phase 4.
// One logical block = one CanonBlock = 729 trytes (3^6).
// RFC-00B2 §2.

#include <array>
#include <cstdint>
#include <string>

#include "t81/canonfs/canon_types.hpp"

namespace t81::ternaryos::dev {

/// One logical block: 729 bytes (one byte per tryte).
inline constexpr std::size_t kBlockSize = t81::canonfs::CanonBlock::kTryteCount;

using BlockData = std::array<uint8_t, kBlockSize>;

/// Metadata returned by IBlockDevice::info().
struct BlockDeviceInfo {
  uint64_t    total_blocks{0};
  std::size_t block_size_bytes{kBlockSize};
  bool        read_only{false};
  std::string device_id;
};

// ─── IBlockDevice ─────────────────────────────────────────────────────────────

/**
 * @brief Abstract block device interface.
 *
 * All reads/writes are in kBlockSize-byte (729-byte) units addressed by
 * zero-based Logical Block Address (LBA).
 *
 * Thread safety: not thread-safe (single-core cooperative model, Phase 4).
 */
class IBlockDevice {
public:
  virtual ~IBlockDevice() = default;

  /// Device metadata.
  virtual BlockDeviceInfo info() const noexcept = 0;

  /// Total number of addressable blocks.
  virtual uint64_t block_count() const noexcept = 0;

  /// Read block at `lba` into `out`.
  /// Returns false if lba >= block_count().
  virtual bool read_block(uint64_t lba, BlockData& out) const = 0;

  /// Write `data` to block at `lba`.
  /// Returns false if lba >= block_count().
  virtual bool write_block(uint64_t lba, const BlockData& data) = 0;

  /// Durably persist all written blocks (fsync equivalent).
  /// Returns false if the underlying flush fails.
  virtual bool flush() = 0;
};

}  // namespace t81::ternaryos::dev
