// experimental/ternaryos/mmu/page_table.cpp

#include "page_table.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace t81::ternaryos::mmu {

uint8_t PageTable::vpn_trit_at(uint64_t vpn, unsigned depth) noexcept {
  return trit_at(vpn, kVpnTrits - 1U - depth);
}

PageTable::Node* PageTable::ensure_leaf(uint64_t vpn) {
  if (!root_) {
    root_ = std::make_unique<Node>();
  }

  Node* node = root_.get();
  for (unsigned depth = 0; depth < kVpnTrits; ++depth) {
    const uint8_t branch = vpn_trit_at(vpn, depth);
    if (!node->children[branch]) {
      node->children[branch] = std::make_unique<Node>();
    }
    node = node->children[branch].get();
  }
  return node;
}

const PageTable::Node* PageTable::find_leaf(uint64_t vpn) const noexcept {
  const Node* node = root_.get();
  for (unsigned depth = 0; depth < kVpnTrits && node; ++depth) {
    node = node->children[vpn_trit_at(vpn, depth)].get();
  }
  return node;
}

bool PageTable::prune_leaf(Node* node, uint64_t vpn, unsigned depth) {
  if (depth == kVpnTrits) {
    node->entry.reset();
  } else {
    const uint8_t branch = vpn_trit_at(vpn, depth);
    auto& child = node->children[branch];
    if (!child) {
      return false;
    }
    const bool child_empty = prune_leaf(child.get(), vpn, depth + 1);
    if (child_empty) {
      child.reset();
    }
  }

  if (node->entry.has_value()) {
    return false;
  }
  return std::all_of(node->children.begin(), node->children.end(),
                     [](const auto& child) { return !child; });
}

std::size_t PageTable::count_nodes(const Node* node) {
  if (!node) {
    return 0;
  }
  std::size_t total = 1;
  for (const auto& child : node->children) {
    total += count_nodes(child.get());
  }
  return total;
}

void PageTable::accumulate_stats(const Node* node, unsigned depth,
                                 PageTableStats& stats) {
  if (!node) {
    return;
  }

  ++stats.radix_nodes;
  stats.max_depth = std::max(stats.max_depth, depth);

  bool has_child = false;
  for (const auto& child : node->children) {
    if (child) {
      has_child = true;
      accumulate_stats(child.get(), depth + 1, stats);
    }
  }
  if (has_child) {
    ++stats.branch_nodes;
  }
  if (node->entry.has_value()) {
    ++stats.leaf_entries;
  }
}

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
             uint64_t tva, uint32_t owner_pid, PagePermissions perms) {
  if (!tva_valid(tva)) return false;

  uint64_t vpn = tva_vpn(tva);
  if (pt.entries_.count(vpn)) return false;  // already mapped

  auto phys = alloc.alloc_page(owner_pid);
  if (!phys.has_value()) return false;       // OOM

  auto* leaf = pt.ensure_leaf(vpn);
  leaf->entry = PageTableEntry{
    .phys_base = *phys,
    .owner_pid = owner_pid,
    .readable = perms.readable,
    .writable = perms.writable,
    .executable = perms.executable,
  };
  pt.entries_[vpn] = *leaf->entry;
  return true;
}

// ─── mmu_translate ───────────────────────────────────────────────────────────

std::optional<uint64_t> mmu_translate(const PageTable& pt, uint64_t tva) {
  return mmu_translate_checked(pt, tva, MmuAccessMode::Read).phys_addr;
}

MmuAccessResult mmu_translate_checked(const PageTable& pt, uint64_t tva,
                                      MmuAccessMode mode) {
  if (!tva_valid(tva)) {
    return {.phys_addr = std::nullopt, .fault = MmuFault::InvalidTva};
  }

  uint64_t vpn    = tva_vpn(tva);
  uint64_t offset = tva_offset(tva);

  const auto* leaf = pt.find_leaf(vpn);
  if (!leaf || !leaf->entry.has_value()) {
    return {.phys_addr = std::nullopt, .fault = MmuFault::Unmapped};
  }

  const auto& entry = *leaf->entry;
  const bool allowed =
      (mode == MmuAccessMode::Read && entry.readable) ||
      (mode == MmuAccessMode::Write && entry.writable) ||
      (mode == MmuAccessMode::Execute && entry.executable);
  if (!allowed) {
    return {.phys_addr = std::nullopt, .fault = MmuFault::PermissionDenied};
  }

  return {.phys_addr = entry.phys_base + offset, .fault = MmuFault::None};
}

// ─── mmu_unmap ───────────────────────────────────────────────────────────────

bool mmu_unmap(PageTable& pt, TernaryPageAllocator& alloc, uint64_t tva) {
  if (!tva_valid(tva)) return false;

  uint64_t vpn = tva_vpn(tva);
  auto it = pt.entries_.find(vpn);
  if (it == pt.entries_.end()) return false;

  bool freed = alloc.free_page(it->second.phys_base);
  pt.entries_.erase(it);
  if (pt.root_ && PageTable::prune_leaf(pt.root_.get(), vpn, 0)) {
    pt.root_.reset();
  }
  return freed;
}

// ─── page_table_dump ─────────────────────────────────────────────────────────

std::string page_table_dump(const PageTable& pt) {
  std::ostringstream ss;
  ss << "PageTable (" << pt.size() << " entries, radix_nodes="
     << PageTable::count_nodes(pt.root_.get()) << "):\n";
  for (const auto& [vpn, entry] : pt.entries()) {
    ss << "  VPN 0x" << std::hex << std::setw(12) << std::setfill('0') << vpn
       << " → phys 0x" << std::setw(12) << entry.phys_base
       << " (pid=" << std::dec << entry.owner_pid
       << ", perms="
       << (entry.readable ? 'r' : '-')
       << (entry.writable ? 'w' : '-')
       << (entry.executable ? 'x' : '-') << ")\n";
  }
  return ss.str();
}

PageTableStats page_table_stats(const PageTable& pt) {
  PageTableStats stats;
  stats.mapped_entries = pt.size();
  PageTable::accumulate_stats(pt.root_.get(), 0, stats);
  return stats;
}

std::string page_table_trace(const PageTable& pt, uint64_t tva) {
  std::ostringstream ss;
  ss << "PageTableTrace(tva=" << tva << ")";
  if (!tva_valid(tva)) {
    ss << " invalid\n";
    return ss.str();
  }

  const uint64_t vpn = tva_vpn(tva);
  const auto* node = pt.root_.get();
  ss << " vpn=" << vpn << " path=";
  for (unsigned depth = 0; depth < PageTable::kVpnTrits; ++depth) {
    const auto branch = PageTable::vpn_trit_at(vpn, depth);
    ss << static_cast<unsigned>(branch);
    if (!node || !node->children[branch]) {
      ss << " miss@" << depth << "\n";
      return ss.str();
    }
    node = node->children[branch].get();
  }

  if (node && node->entry.has_value()) {
    ss << " hit phys=0x" << std::hex << node->entry->phys_base
       << std::dec << " pid=" << node->entry->owner_pid
       << " perms="
       << (node->entry->readable ? 'r' : '-')
       << (node->entry->writable ? 'w' : '-')
       << (node->entry->executable ? 'x' : '-') << "\n";
  } else {
    ss << " empty\n";
  }
  return ss.str();
}

}  // namespace t81::ternaryos::mmu
