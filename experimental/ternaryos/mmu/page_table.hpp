#pragma once

// experimental/ternaryos/mmu/page_table.hpp
//
// Ternary MMU — Phase 2 flat page table and translate/map/unmap API.
// RFC-00B1 §3, §4.

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "tva.hpp"
#include "ternary_page_alloc.hpp"

namespace t81::ternaryos::mmu {

// ─── Page Table ──────────────────────────────────────────────────────────────

/**
 * @brief Phase 2 page table: flat VPN → physical page base map.
 *
 * Each entry records the physical page base address and the owning PID.
 * Phase 3 will replace this with a 3-ary radix trie; the mmu_* API is
 * unchanged so callers need not be updated.
 */
struct PageTableEntry {
  uint64_t phys_base{0};   ///< Physical byte address of the mapped page
  uint32_t owner_pid{0};   ///< Hanoi PID that owns this mapping (0 = kernel)
};

class PageTable {
public:
  // Number of mapped pages.
  std::size_t size() const noexcept { return entries_.size(); }
  bool empty()       const noexcept { return entries_.empty(); }

  // Internal map exposed read-only for diagnostics.
  const std::unordered_map<uint64_t, PageTableEntry>& entries() const noexcept {
    return entries_;
  }

private:
  friend bool mmu_map(PageTable&, TernaryPageAllocator&,
                      uint64_t, uint32_t);
  friend std::optional<uint64_t> mmu_translate(const PageTable&, uint64_t);
  friend bool mmu_unmap(PageTable&, TernaryPageAllocator&, uint64_t);

  std::unordered_map<uint64_t, PageTableEntry> entries_;  // keyed by VPN
};

// ─── MMU Operations (RFC-00B1 §4) ────────────────────────────────────────────

/**
 * @brief Map the virtual page containing `tva` to a new physical page.
 *
 * Allocates one page from `alloc` and inserts VPN → phys_base into `pt`.
 *
 * @return false if the VPN is already mapped, or the allocator is OOM.
 */
bool mmu_map(PageTable& pt, TernaryPageAllocator& alloc,
             uint64_t tva, uint32_t owner_pid = 0);

/**
 * @brief Translate a TVA to a physical byte address.
 *
 * Looks up VPN in `pt`, then adds the page offset.
 *
 * @return Physical address, or nullopt if the VPN is not mapped or the TVA
 *         exceeds kMaxTva.
 */
std::optional<uint64_t> mmu_translate(const PageTable& pt, uint64_t tva);

/**
 * @brief Unmap the virtual page containing `tva` and free its physical page.
 *
 * @return false if the VPN is not mapped or free_page fails.
 */
bool mmu_unmap(PageTable& pt, TernaryPageAllocator& alloc, uint64_t tva);

/**
 * @brief Human-readable dump of the page table for diagnostics.
 */
std::string page_table_dump(const PageTable& pt);

}  // namespace t81::ternaryos::mmu
