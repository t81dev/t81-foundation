// experimental/ternaryos/dev/canon_store.cpp

#include "canon_store.hpp"

#include <cstring>

namespace t81::ternaryos::dev {

// ─── Constructor ─────────────────────────────────────────────────────────────

CanonStore::CanonStore(IBlockDevice& dev) : dev_(dev) {}

// ─── put ─────────────────────────────────────────────────────────────────────

std::optional<t81::canonfs::CanonRef> CanonStore::put(
    const t81::canonfs::CanonBlock& block) {
  // Deduplicate: if already stored, return existing ref.
  auto h = block.hash();
  if (index_.count(h)) {
    return t81::canonfs::CanonRef{h};
  }

  // Enforce Phase 4 index cap.
  if (index_.size() >= kMaxIndexEntries) return std::nullopt;

  // Check device capacity.
  if (next_lba_ >= dev_.block_count()) return std::nullopt;

  // Write to device.
  BlockData raw{};
  std::copy(block.trytes.begin(), block.trytes.end(), raw.begin());
  if (!dev_.write_block(next_lba_, raw)) return std::nullopt;

  index_[h] = next_lba_;
  ++next_lba_;
  return t81::canonfs::CanonRef{h};
}

// ─── get ─────────────────────────────────────────────────────────────────────

std::optional<t81::canonfs::CanonBlock> CanonStore::get(
    const t81::canonfs::CanonRef& ref) const {
  auto it = index_.find(ref.hash);
  if (it == index_.end()) return std::nullopt;

  BlockData raw{};
  if (!dev_.read_block(it->second, raw)) return std::nullopt;

  // Reconstruct CanonBlock and verify hash.
  t81::canonfs::CanonBlock block;
  std::copy(raw.begin(), raw.end(), block.trytes.begin());

  if (!(block.hash() == ref.hash)) return std::nullopt;  // corruption detected
  return block;
}

bool CanonStore::contains(const t81::canonfs::CanonRef& ref) const {
  return index_.count(ref.hash) > 0;
}

// ─── flush ───────────────────────────────────────────────────────────────────

// Index header layout at LBA 0 (RFC-00B2 §3.2):
//   bytes  0–3   : magic "CST1"
//   bytes  4–7   : entry_count (uint32_t LE)
//   bytes  8–727 : entries (hash[32] + lba[8] = 40 bytes each; max 17)
//   byte   728   : 0x00 padding

static const uint8_t kIndexMagic[4] = {'C', 'S', 'T', '1'};

bool CanonStore::flush() {
  if (dev_.block_count() == 0) return false;

  BlockData hdr{};
  // Magic
  std::memcpy(hdr.data(), kIndexMagic, 4);
  // Entry count
  uint32_t count = static_cast<uint32_t>(index_.size());
  std::memcpy(hdr.data() + 4, &count, 4);
  // Entries
  std::size_t off = 8;
  for (const auto& [hash, lba] : index_) {
    if (off + 40 > kBlockSize) break;  // shouldn't happen under Phase 4 cap
    std::memcpy(hdr.data() + off, hash.h.bytes.data(), 32);
    std::memcpy(hdr.data() + off + 32, &lba, 8);
    off += 40;
  }

  if (!dev_.write_block(0, hdr)) return false;
  return dev_.flush();
}

// ─── rebuild_index ───────────────────────────────────────────────────────────

std::size_t CanonStore::rebuild_index() {
  index_.clear();
  next_lba_ = 1;

  // Try to read the index header first (fast path).
  if (dev_.block_count() > 0) {
    BlockData hdr{};
    if (dev_.read_block(0, hdr) &&
        hdr[0] == kIndexMagic[0] && hdr[1] == kIndexMagic[1] &&
        hdr[2] == kIndexMagic[2] && hdr[3] == kIndexMagic[3]) {
      uint32_t count = 0;
      std::memcpy(&count, hdr.data() + 4, 4);
      if (count <= kMaxIndexEntries) {
        std::size_t off = 8;
        for (uint32_t i = 0; i < count && off + 40 <= kBlockSize; ++i) {
          t81::canonfs::CanonHash hash;
          uint64_t lba = 0;
          std::memcpy(hash.h.bytes.data(), hdr.data() + off, 32);
          std::memcpy(&lba, hdr.data() + off + 32, 8);
          off += 40;
          if (lba == 0 || lba >= dev_.block_count()) continue;
          index_[hash] = lba;
          if (lba >= next_lba_) next_lba_ = lba + 1;
        }
        return index_.size();
      }
    }
  }

  // Fallback: scan all data blocks (LBA 1..block_count-1), re-hash each.
  for (uint64_t lba = 1; lba < dev_.block_count(); ++lba) {
    BlockData raw{};
    if (!dev_.read_block(lba, raw)) continue;

    // Skip all-zero blocks (empty/unwritten).
    bool all_zero = true;
    for (auto b : raw) { if (b) { all_zero = false; break; } }
    if (all_zero) continue;

    t81::canonfs::CanonBlock block;
    std::copy(raw.begin(), raw.end(), block.trytes.begin());
    auto hash = block.hash();

    if (!index_.count(hash)) {
      index_[hash] = lba;
      if (lba >= next_lba_) next_lba_ = lba + 1;
    }
  }
  return index_.size();
}

}  // namespace t81::ternaryos::dev
