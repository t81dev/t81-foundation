// experimental/ternaryos/mmu/page_table.cpp

#include "page_table.hpp"

#include <sstream>
#include <iomanip>

namespace t81::ternaryos::mmu {

// ─── tva_to_string ───────────────────────────────────────────────────────────

std::string tva_to_string(uint64_t tva) {
  if (tva == 0) return "0";
  // Extract up to 30 base-3 digits, then reverse.
  char buf[31];
  int  pos = 0;
  uint64_t v = tva;
  while (v > 0 && pos < 30) {
    buf[pos++] = static_cast<char>('0' + (v % 3));
    v /= 3;
  }
  // Reverse
  for (int i = 0, j = pos - 1; i < j; ++i, --j) std::swap(buf[i], buf[j]);
  return std::string(buf, static_cast<std::size_t>(pos));
}

// ─── mmu_map ─────────────────────────────────────────────────────────────────

bool mmu_map(PageTable& pt, TernaryPageAllocator& alloc,
             uint64_t tva, uint32_t owner_pid) {
  if (!tva_valid(tva)) return false;

  uint64_t vpn = tva_vpn(tva);
  if (pt.entries_.count(vpn)) return false;  // already mapped

  auto phys = alloc.alloc_page(owner_pid);
  if (!phys.has_value()) return false;       // OOM

  pt.entries_[vpn] = PageTableEntry{*phys, owner_pid};
  return true;
}

// ─── mmu_translate ───────────────────────────────────────────────────────────

std::optional<uint64_t> mmu_translate(const PageTable& pt, uint64_t tva) {
  if (!tva_valid(tva)) return std::nullopt;

  uint64_t vpn    = tva_vpn(tva);
  uint64_t offset = tva_offset(tva);

  auto it = pt.entries_.find(vpn);
  if (it == pt.entries_.end()) return std::nullopt;

  return it->second.phys_base + offset;
}

// ─── mmu_unmap ───────────────────────────────────────────────────────────────

bool mmu_unmap(PageTable& pt, TernaryPageAllocator& alloc, uint64_t tva) {
  if (!tva_valid(tva)) return false;

  uint64_t vpn = tva_vpn(tva);
  auto it = pt.entries_.find(vpn);
  if (it == pt.entries_.end()) return false;

  bool freed = alloc.free_page(it->second.phys_base);
  pt.entries_.erase(it);
  return freed;
}

// ─── page_table_dump ─────────────────────────────────────────────────────────

std::string page_table_dump(const PageTable& pt) {
  std::ostringstream ss;
  ss << "PageTable (" << pt.size() << " entries):\n";
  for (const auto& [vpn, entry] : pt.entries()) {
    ss << "  VPN 0x" << std::hex << std::setw(12) << std::setfill('0') << vpn
       << " → phys 0x" << std::setw(12) << entry.phys_base
       << " (pid=" << std::dec << entry.owner_pid << ")\n";
  }
  return ss.str();
}

}  // namespace t81::ternaryos::mmu
