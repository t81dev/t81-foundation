#pragma once

// experimental/ternaryos/mmu/page_table.hpp
//
// Ternary MMU — Phase 2 flat page table and translate/map/unmap API.
// RFC-00B1 §3, §4.

#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "tva.hpp"
#include "ternary_page_alloc.hpp"

namespace t81::ternaryos::mmu {

// ─── Page Table ──────────────────────────────────────────────────────────────

/**
 * @brief Phase 3 page table: ternary radix tree over the 20-trit VPN.
 *
 * Each entry records the physical page base address and the owning PID. The
 * public MMU API is unchanged; callers still map/translate/unmap using TVAs.
 * A flat VPN cache is retained only for diagnostics and compatibility.
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
  static constexpr unsigned kVpnTrits = 20;

  friend bool mmu_map(PageTable&, TernaryPageAllocator&,
                      uint64_t, uint32_t);
  friend std::optional<uint64_t> mmu_translate(const PageTable&, uint64_t);
  friend bool mmu_unmap(PageTable&, TernaryPageAllocator&, uint64_t);
  friend std::string page_table_dump(const PageTable&);

  struct Node {
    std::array<std::unique_ptr<Node>, 3> children{};
    std::optional<PageTableEntry> entry{};
  };

  static uint8_t vpn_trit_at(uint64_t vpn, unsigned depth) noexcept;
  Node* ensure_leaf(uint64_t vpn);
  const Node* find_leaf(uint64_t vpn) const noexcept;
  static bool prune_leaf(Node* node, uint64_t vpn, unsigned depth);
  static std::size_t count_nodes(const Node* node);

  std::unique_ptr<Node> root_;
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
