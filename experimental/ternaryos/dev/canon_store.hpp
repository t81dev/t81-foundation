#pragma once

// experimental/ternaryos/dev/canon_store.hpp
//
// Content-addressed CanonBlock store backed by an IBlockDevice.
// RFC-00B2 §3.
//
// put(block)  → hash → deduplicate → allocate LBA → write → return CanonRef
// get(ref)    → look up LBA → read → verify hash → return CanonBlock
// flush()     → persist index to LBA 0 → IBlockDevice::flush()
// rebuild()   → scan data blocks, re-hash, repopulate in-memory index

#include "block_device.hpp"

#include <map>
#include <optional>

#include "t81/canonfs/canon_types.hpp"

namespace t81::ternaryos::dev {

/**
 * @brief Content-addressed CanonBlock store layered on an IBlockDevice.
 *
 * LBA 0 is reserved for the index header (RFC-00B2 §3.2).
 * Data blocks start at LBA 1.  Maximum index entries per single header
 * block: 17 (Phase 4 cap; chained index pages planned for Phase 5).
 */
class CanonStore {
public:
  inline static constexpr std::size_t kMaxIndexEntries = 17;

  explicit CanonStore(IBlockDevice& dev);

  // ── Write path ──────────────────────────────────────────────────────────────

  /// Store a CanonBlock.  If identical content is already present,
  /// returns the existing CanonRef (deduplication).
  /// Returns nullopt if the device is full or flush fails.
  std::optional<t81::canonfs::CanonRef> put(const t81::canonfs::CanonBlock& block);

  // ── Read path ───────────────────────────────────────────────────────────────

  /// Retrieve a block by CanonRef.
  /// Verifies the block's hash after reading; returns nullopt on mismatch.
  std::optional<t81::canonfs::CanonBlock> get(const t81::canonfs::CanonRef& ref) const;

  /// True iff the store contains at least one block with this hash.
  bool contains(const t81::canonfs::CanonRef& ref) const;

  // ── Persistence ─────────────────────────────────────────────────────────────

  /// Write the in-memory index to LBA 0, then call IBlockDevice::flush().
  bool flush();

  /// Scan all allocated data blocks, re-hash each, rebuild the index.
  /// Call this after loading a device from disk (post-"reboot").
  std::size_t rebuild_index();

  /// Number of stored blocks (unique hashes).
  std::size_t size() const noexcept { return index_.size(); }

  /// Number of LBAs consumed (always == size() + 1 for the header).
  uint64_t next_lba() const noexcept { return next_lba_; }

private:
  IBlockDevice& dev_;
  std::map<t81::canonfs::CanonHash, uint64_t> index_;  ///< hash → LBA
  uint64_t next_lba_{1};  ///< next free data LBA (LBA 0 = index header)
};

}  // namespace t81::ternaryos::dev
