// experimental/ternaryos/mmu/ternary_page_alloc.cpp

#include "ternary_page_alloc.hpp"

#include <algorithm>
#include <sstream>

namespace t81::ternaryos::mmu {

// ─── Constructor ─────────────────────────────────────────────────────────────

TernaryPageAllocator::TernaryPageAllocator(
    const std::vector<hal::MemoryRegion>& memory_map) {
  for (const auto& region : memory_map) {
    // Align base up to page boundary.
    uint64_t base = ((region.base_phys + kPageSize - 1) / kPageSize) * kPageSize;
    uint64_t end  = region.base_phys + region.size_bytes;

    // Determine default state for this region's pages.
    TernaryPageState default_state =
        region.writable ? TernaryPageState::Free : TernaryPageState::Reserved;

    while (base + kPageSize <= end) {
      pages_.push_back(PageDescriptor{
        .phys_base  = base,
        .state      = default_state,
        .owner_pid  = 0,
      });
      base += kPageSize;
    }
  }

  // Sort by physical address for deterministic alloc_contiguous scan.
  std::sort(pages_.begin(), pages_.end(),
            [](const PageDescriptor& a, const PageDescriptor& b) {
              return a.phys_base < b.phys_base;
            });
}

// ─── Allocation ──────────────────────────────────────────────────────────────

std::optional<uint64_t> TernaryPageAllocator::alloc_page(uint32_t owner_pid) {
  for (auto& pg : pages_) {
    if (pg.state == TernaryPageState::Free) {
      pg.state     = TernaryPageState::Allocated;
      pg.owner_pid = owner_pid;
      return pg.phys_base;
    }
  }
  return std::nullopt;
}

std::optional<uint64_t> TernaryPageAllocator::alloc_contiguous(
    std::size_t n, uint32_t owner_pid) {
  if (n == 0 || n > pages_.size()) return std::nullopt;

  for (std::size_t i = 0; i + n <= pages_.size(); ++i) {
    // Check that the run is physically contiguous and all FREE.
    bool ok = true;
    for (std::size_t j = 0; j < n; ++j) {
      const auto& pg = pages_[i + j];
      if (pg.state != TernaryPageState::Free) { ok = false; break; }
      if (j > 0 && pg.phys_base != pages_[i + j - 1].phys_base + kPageSize) {
        ok = false; break;
      }
    }
    if (!ok) continue;

    // Commit the run.
    for (std::size_t j = 0; j < n; ++j) {
      pages_[i + j].state     = TernaryPageState::Allocated;
      pages_[i + j].owner_pid = owner_pid;
    }
    return pages_[i].phys_base;
  }
  return std::nullopt;
}

bool TernaryPageAllocator::free_page(uint64_t phys_base) {
  PageDescriptor* pg = find_page(phys_base);
  if (!pg || pg->state != TernaryPageState::Allocated) return false;
  pg->state     = TernaryPageState::Free;
  pg->owner_pid = 0;
  return true;
}

// ─── Introspection ───────────────────────────────────────────────────────────

std::size_t TernaryPageAllocator::free_pages() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(pages_.begin(), pages_.end(),
                    [](const PageDescriptor& p) {
                      return p.state == TernaryPageState::Free;
                    }));
}

std::size_t TernaryPageAllocator::reserved_pages() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(pages_.begin(), pages_.end(),
                    [](const PageDescriptor& p) {
                      return p.state == TernaryPageState::Reserved;
                    }));
}

std::size_t TernaryPageAllocator::allocated_pages() const noexcept {
  return static_cast<std::size_t>(
      std::count_if(pages_.begin(), pages_.end(),
                    [](const PageDescriptor& p) {
                      return p.state == TernaryPageState::Allocated;
                    }));
}

std::string TernaryPageAllocator::stats_string() const {
  std::ostringstream ss;
  ss << "TernaryPageAllocator: "
     << total_pages()     << " total, "
     << free_pages()      << " free, "
     << reserved_pages()  << " reserved, "
     << allocated_pages() << " allocated"
     << " (page_size=" << kPageSize << " bytes)";
  return ss.str();
}

// ─── Internal helpers ────────────────────────────────────────────────────────

PageDescriptor* TernaryPageAllocator::find_page(uint64_t phys_base) noexcept {
  // Binary search on sorted pages_.
  auto it = std::lower_bound(
      pages_.begin(), pages_.end(), phys_base,
      [](const PageDescriptor& pg, uint64_t addr) {
        return pg.phys_base < addr;
      });
  if (it == pages_.end() || it->phys_base != phys_base) return nullptr;
  return &*it;
}

}  // namespace t81::ternaryos::mmu
