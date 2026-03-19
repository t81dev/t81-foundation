#pragma once

// experimental/ternaryos/mmu/ternary_page_alloc.hpp
//
// Ternary page allocator — Phase 1 / Phase 2 bridge.
//
// Consumes the HAL BootContext memory map and manages a flat bitmap of
// 3¹⁰ = 59,049-tryte pages. Each page is tracked with a TernaryPageState
// trit: FREE(-1), RESERVED(0), or ALLOCATED(+1) — matching balanced ternary
// semantics from the T81 type system.
//
// This is the lowest-level allocator in TernOS. The full Ternary MMU (Phase 2
// / RFC-00B1) will build virtual address translation on top of this.

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "../hal/hal.hpp"

namespace t81::ternaryos::mmu {

// ─── Balanced ternary page state ─────────────────────────────────────────────

/**
 * @brief Balanced-ternary page state.
 *   FREE     (-1) — available for allocation
 *   RESERVED ( 0) — mapped but not user-allocatable (firmware, MMIO, HAL stack)
 *   ALLOCATED(+1) — in use by a TISC process or kernel component
 */
enum class TernaryPageState : int8_t {
  Free      = -1,
  Reserved  =  0,
  Allocated = +1,
};

// ─── Page descriptor ─────────────────────────────────────────────────────────

/**
 * @brief Descriptor for a single 3¹⁰-tryte physical page.
 */
struct PageDescriptor {
  uint64_t         phys_base;   ///< Physical byte address of this page's start
  TernaryPageState state{TernaryPageState::Reserved};
  uint32_t         owner_pid{0};  ///< 0 = kernel; non-zero = Hanoi PID
};

// ─── Allocator ───────────────────────────────────────────────────────────────

/**
 * @brief Physical page allocator over the HAL memory map.
 *
 * Lifecycle:
 *   1. Construct with the HAL BootContext memory map.
 *   2. Writable regions are carved into FREE pages.
 *      Non-writable / non-executable regions are marked RESERVED.
 *   3. alloc_page() / free_page() manage individual pages.
 *   4. alloc_contiguous(n) finds a run of n consecutive FREE pages.
 *
 * Thread safety: not thread-safe in Phase 1 (single-core boot context).
 * Phase 3 (scheduler) will wrap this with a spinlock.
 */
class TernaryPageAllocator {
public:
  static constexpr uint64_t kPageSize = hal::MemoryRegion::kTernaryPageSize;

  /**
   * @brief Construct from the HAL memory map.
   *
   * Writable regions produce FREE pages.
   * Read-only / executable-only regions produce RESERVED pages.
   */
  explicit TernaryPageAllocator(
      const std::vector<hal::MemoryRegion>& memory_map);

  // ── Allocation ─────────────────────────────────────────────────────────────

  /**
   * @brief Allocate one FREE page.
   * @param owner_pid  Hanoi PID to associate with the page (0 = kernel).
   * @return Physical page base address, or nullopt if OOM.
   */
  std::optional<uint64_t> alloc_page(uint32_t owner_pid = 0);

  /**
   * @brief Allocate n physically contiguous FREE pages.
   * @return Physical base of the first page, or nullopt if no run of n exists.
   */
  std::optional<uint64_t> alloc_contiguous(std::size_t n,
                                            uint32_t owner_pid = 0);

  /**
   * @brief Free a previously allocated page by its physical base address.
   * @return true on success; false if the address was not ALLOCATED.
   */
  bool free_page(uint64_t phys_base);

  // ── Introspection ──────────────────────────────────────────────────────────

  std::size_t total_pages()     const noexcept { return pages_.size(); }
  std::size_t free_pages()      const noexcept;
  std::size_t reserved_pages()  const noexcept;
  std::size_t allocated_pages() const noexcept;

  /**
   * @brief Human-readable summary for hal_log / diagnostics.
   */
  std::string stats_string() const;

  /**
   * @brief Read-only view of the full page table (for tests and dumps).
   */
  std::span<const PageDescriptor> page_table() const noexcept {
    return {pages_.data(), pages_.size()};
  }

private:
  std::vector<PageDescriptor> pages_;

  // Returns pointer to descriptor for phys_base, or nullptr if not found.
  PageDescriptor* find_page(uint64_t phys_base) noexcept;
};

}  // namespace t81::ternaryos::mmu
