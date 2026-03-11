// experimental/ternaryos/dev/canon_store.cpp

#include "canon_store.hpp"

#include <cstring>
#include <iterator>

namespace t81::ternaryos::dev {

namespace {

constexpr uint8_t kRootIndexMagic[4] = {'C', 'S', 'T', '1'};
constexpr uint8_t kOverflowIndexMagic[4] = {'C', 'S', 'I', '1'};
constexpr std::size_t kRootEntryCountOffset = 4;
constexpr std::size_t kRootEntriesOffset = 8;
constexpr std::size_t kRootOverflowCountOffset = 688;
constexpr std::size_t kOverflowEntryCountOffset = 4;
constexpr std::size_t kOverflowEntriesOffset = 8;
constexpr std::size_t kEntrySpanBytes = 40;

bool has_magic(const BlockData& block, const uint8_t (&magic)[4]) {
  return block[0] == magic[0] && block[1] == magic[1] &&
         block[2] == magic[2] && block[3] == magic[3];
}

std::size_t parse_index_entries(const BlockData& block,
                                std::size_t offset,
                                std::size_t entry_count,
                                uint64_t device_block_count,
                                std::map<t81::canonfs::CanonHash, uint64_t>& index,
                                uint64_t& next_lba) {
  std::size_t off = offset;
  std::size_t parsed = 0;
  for (std::size_t i = 0; i < entry_count && off + kEntrySpanBytes <= kBlockSize; ++i) {
    t81::canonfs::CanonHash hash;
    uint64_t lba = 0;
    std::memcpy(hash.h.bytes.data(), block.data() + off, 32);
    std::memcpy(&lba, block.data() + off + 32, 8);
    off += kEntrySpanBytes;
    ++parsed;
    if (lba == 0 || lba >= device_block_count) continue;
    index[hash] = lba;
    if (lba >= next_lba) next_lba = lba + 1;
  }
  return parsed;
}

}  // namespace

// ─── Constructor ─────────────────────────────────────────────────────────────

CanonStore::CanonStore(IBlockDevice& dev) : dev_(dev) {}

std::size_t CanonStore::overflow_index_blocks_for(std::size_t entry_count) noexcept {
  if (entry_count <= kIndexEntriesPerBlock) return 0;
  const std::size_t remaining = entry_count - kIndexEntriesPerBlock;
  return (remaining + kIndexEntriesPerBlock - 1) / kIndexEntriesPerBlock;
}

uint64_t CanonStore::metadata_tail_start(uint64_t block_count,
                                         std::size_t entry_count) noexcept {
  const auto overflow_blocks = overflow_index_blocks_for(entry_count);
  if (block_count <= overflow_blocks) return 0;
  return block_count - overflow_blocks;
}

// ─── put ─────────────────────────────────────────────────────────────────────

std::optional<t81::canonfs::CanonRef> CanonStore::put(
    const t81::canonfs::CanonBlock& block) {
  // Deduplicate: if already stored, return existing ref.
  auto h = block.hash();
  if (index_.count(h)) {
    return t81::canonfs::CanonRef{h};
  }

  // Check device capacity.
  const auto next_entry_count = index_.size() + 1;
  const auto data_limit = metadata_tail_start(dev_.block_count(), next_entry_count);
  if (data_limit == 0 || next_lba_ >= data_limit) return std::nullopt;

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
//   bytes  8–687 : up to 17 entries (hash[32] + lba[8] = 40 bytes each)
//   bytes 688–695: overflow index block count (uint64_t LE)
// Additional overflow index blocks, when present, are stored contiguously
// from the tail of the device downward and use magic "CSI1".

bool CanonStore::flush() {
  if (dev_.block_count() == 0) return false;
  const auto overflow_blocks = overflow_index_blocks_for(index_.size());
  const auto data_limit = metadata_tail_start(dev_.block_count(), index_.size());
  if (data_limit == 0 || next_lba_ > data_limit) return false;

  BlockData existing_root{};
  std::size_t previous_overflow_blocks = 0;
  if (dev_.read_block(0, existing_root) && has_magic(existing_root, kRootIndexMagic)) {
    std::memcpy(&previous_overflow_blocks,
                existing_root.data() + kRootOverflowCountOffset,
                sizeof(previous_overflow_blocks));
    if (previous_overflow_blocks > dev_.block_count() - 1) {
      previous_overflow_blocks = 0;
    }
  }

  BlockData hdr{};
  // Magic
  std::memcpy(hdr.data(), kRootIndexMagic, 4);
  // Entry count
  uint32_t count = static_cast<uint32_t>(index_.size());
  std::memcpy(hdr.data() + kRootEntryCountOffset, &count, sizeof(count));
  std::memcpy(hdr.data() + kRootOverflowCountOffset,
              &overflow_blocks,
              sizeof(overflow_blocks));

  std::size_t root_entries_written = 0;
  std::size_t off = kRootEntriesOffset;
  auto it = index_.cbegin();
  for (; it != index_.cend() && root_entries_written < kIndexEntriesPerBlock;
       ++it, ++root_entries_written) {
    std::memcpy(hdr.data() + off, it->first.h.bytes.data(), 32);
    std::memcpy(hdr.data() + off + 32, &it->second, 8);
    off += kEntrySpanBytes;
  }

  if (!dev_.write_block(0, hdr)) return false;

  for (std::size_t block_index = 0; block_index < overflow_blocks; ++block_index) {
    BlockData overflow{};
    std::memcpy(overflow.data(), kOverflowIndexMagic, 4);

    const auto remaining = static_cast<std::size_t>(std::distance(it, index_.cend()));
    const uint32_t block_entry_count = static_cast<uint32_t>(
        std::min(remaining, kIndexEntriesPerBlock));
    std::memcpy(overflow.data() + kOverflowEntryCountOffset,
                &block_entry_count,
                sizeof(block_entry_count));

    std::size_t overflow_off = kOverflowEntriesOffset;
    for (uint32_t written = 0; written < block_entry_count; ++written, ++it) {
      std::memcpy(overflow.data() + overflow_off, it->first.h.bytes.data(), 32);
      std::memcpy(overflow.data() + overflow_off + 32, &it->second, 8);
      overflow_off += kEntrySpanBytes;
    }

    const uint64_t lba = dev_.block_count() - overflow_blocks + block_index;
    if (!dev_.write_block(lba, overflow)) return false;
  }

  if (previous_overflow_blocks > overflow_blocks) {
    BlockData zero{};
    for (std::size_t block_index = 0; block_index < previous_overflow_blocks - overflow_blocks; ++block_index) {
      const uint64_t lba = dev_.block_count() - previous_overflow_blocks + block_index;
      if (!dev_.write_block(lba, zero)) return false;
    }
  }

  return dev_.flush();
}

// ─── rebuild_index ───────────────────────────────────────────────────────────

std::size_t CanonStore::rebuild_index() {
  index_.clear();
  next_lba_ = 1;
  uint64_t fallback_scan_limit = dev_.block_count();

  // Try to read the index header first (fast path).
  if (dev_.block_count() > 0) {
    BlockData hdr{};
    if (dev_.read_block(0, hdr) && has_magic(hdr, kRootIndexMagic)) {
      uint32_t count = 0;
      std::size_t overflow_blocks = 0;
      std::memcpy(&count, hdr.data() + kRootEntryCountOffset, sizeof(count));
      std::memcpy(&overflow_blocks,
                  hdr.data() + kRootOverflowCountOffset,
                  sizeof(overflow_blocks));
      if (overflow_blocks <= dev_.block_count() - 1) {
        fallback_scan_limit = dev_.block_count() - overflow_blocks;
      }

      const auto expected_overflow_blocks =
          overflow_index_blocks_for(static_cast<std::size_t>(count));
      if (overflow_blocks == expected_overflow_blocks) {
        std::size_t parsed_total = parse_index_entries(
            hdr,
            kRootEntriesOffset,
            std::min<std::size_t>(count, kIndexEntriesPerBlock),
            dev_.block_count(),
            index_,
            next_lba_);

        bool valid_overflow = true;
        std::size_t remaining = count > parsed_total ? count - parsed_total : 0;
        for (std::size_t block_index = 0; block_index < overflow_blocks && valid_overflow; ++block_index) {
          const uint64_t lba = dev_.block_count() - overflow_blocks + block_index;
          BlockData overflow{};
          if (!dev_.read_block(lba, overflow) || !has_magic(overflow, kOverflowIndexMagic)) {
            valid_overflow = false;
            break;
          }

          uint32_t block_entry_count = 0;
          std::memcpy(&block_entry_count,
                      overflow.data() + kOverflowEntryCountOffset,
                      sizeof(block_entry_count));
          const auto expected_entries =
              static_cast<uint32_t>(std::min<std::size_t>(remaining, kIndexEntriesPerBlock));
          if (block_entry_count != expected_entries) {
            valid_overflow = false;
            break;
          }

          parsed_total += parse_index_entries(
              overflow,
              kOverflowEntriesOffset,
              block_entry_count,
              dev_.block_count(),
              index_,
              next_lba_);
          remaining -= block_entry_count;
        }

        if (valid_overflow && parsed_total == count) {
          return index_.size();
        }
      }
      index_.clear();
      next_lba_ = 1;
    }
  }

  // Fallback: scan all data blocks (LBA 1..metadata_tail_start-1), re-hash each.
  for (uint64_t lba = 1; lba < fallback_scan_limit; ++lba) {
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
