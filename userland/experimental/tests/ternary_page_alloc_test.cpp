// experimental/ternaryos/tests/ternary_page_alloc_test.cpp
//
// Unit tests for the TernaryPageAllocator.

#include "../mmu/ternary_page_alloc.hpp"

#include <cassert>
#include <cstdio>
#include <optional>

using namespace t81::ternaryos::mmu;
using namespace t81::ternaryos::hal;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
  return cond;
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Build a simple memory map: one writable region of exactly `n_pages` pages.
// Base is kPageSize (59049) so it is already page-aligned — the constructor's
// ceiling-align step is a no-op and the region produces exactly n_pages pages.
static std::vector<MemoryRegion> make_map(std::size_t n_pages,
                                          uint64_t base = TernaryPageAllocator::kPageSize) {
  return {{
    .base_phys  = base,
    .size_bytes = n_pages * TernaryPageAllocator::kPageSize,
    .writable   = true,
    .executable = false,
  }};
}

// ─── Tests ───────────────────────────────────────────────────────────────────

static void test_construction() {
  std::printf("\n[T1] Construction from memory map\n");
  auto alloc = TernaryPageAllocator(make_map(8));
  check(alloc.total_pages()    == 8, "8 total pages");
  check(alloc.free_pages()     == 8, "all pages FREE initially");
  check(alloc.reserved_pages() == 0, "no reserved pages");
  check(alloc.allocated_pages()== 0, "no allocated pages");
}

static void test_reserved_region() {
  std::printf("\n[T2] Read-only region produces RESERVED pages\n");
  std::vector<MemoryRegion> map = {{
    .base_phys  = TernaryPageAllocator::kPageSize,  // page-aligned
    .size_bytes = 4 * TernaryPageAllocator::kPageSize,
    .writable   = false,
    .executable = true,
  }};
  auto alloc = TernaryPageAllocator(map);
  check(alloc.total_pages()    == 4, "4 total pages");
  check(alloc.reserved_pages() == 4, "all 4 pages RESERVED");
  check(alloc.free_pages()     == 0, "no free pages");
  // Allocation from reserved region should fail.
  check(!alloc.alloc_page().has_value(), "alloc_page returns nullopt when all reserved");
}

static void test_alloc_and_free() {
  std::printf("\n[T3] alloc_page / free_page round-trip\n");
  auto alloc = TernaryPageAllocator(make_map(4));

  auto p1 = alloc.alloc_page(/*owner=*/1);
  auto p2 = alloc.alloc_page(/*owner=*/2);
  check(p1.has_value(),           "first alloc succeeds");
  check(p2.has_value(),           "second alloc succeeds");
  check(p1 != p2,                 "distinct physical addresses");
  check(alloc.free_pages() == 2,  "2 pages remaining free");
  check(alloc.allocated_pages() == 2, "2 pages allocated");

  check(alloc.free_page(*p1),     "free_page succeeds for allocated page");
  check(alloc.free_pages() == 3,  "3 free after free_page");
  check(!alloc.free_page(*p1),    "double-free returns false");
}

static void test_oom() {
  std::printf("\n[T4] OOM: exhaust all pages\n");
  constexpr std::size_t kN = 3;
  auto alloc = TernaryPageAllocator(make_map(kN));

  for (std::size_t i = 0; i < kN; ++i) alloc.alloc_page();
  check(alloc.free_pages() == 0, "no free pages after exhaustion");
  auto oom = alloc.alloc_page();
  check(!oom.has_value(),        "alloc_page returns nullopt when OOM");
}

static void test_contiguous_alloc() {
  std::printf("\n[T5] alloc_contiguous\n");
  auto alloc = TernaryPageAllocator(make_map(9));

  auto run = alloc.alloc_contiguous(3);
  check(run.has_value(), "contiguous alloc of 3 succeeds");
  check(alloc.allocated_pages() == 3, "3 pages marked allocated");
  check(alloc.free_pages() == 6, "6 pages still free");

  // Verify physical contiguity.
  if (run.has_value()) {
    uint64_t base = *run;
    const auto& tbl = alloc.page_table();
    // Find the run start.
    bool contiguous = true;
    std::size_t found = 0;
    for (std::size_t i = 0; i < tbl.size(); ++i) {
      if (tbl[i].phys_base == base) {
        for (std::size_t j = 0; j < 3; ++j) {
          if (tbl[i+j].phys_base != base + j * TernaryPageAllocator::kPageSize) {
            contiguous = false; break;
          }
          if (tbl[i+j].state != TernaryPageState::Allocated) {
            contiguous = false; break;
          }
        }
        found = 1;
        break;
      }
    }
    check(found == 1 && contiguous, "3 contiguous pages are physically adjacent and ALLOCATED");
  }
}

static void test_contiguous_oom() {
  std::printf("\n[T6] alloc_contiguous OOM: request larger than available\n");
  auto alloc = TernaryPageAllocator(make_map(2));
  auto r = alloc.alloc_contiguous(3);
  check(!r.has_value(), "contiguous alloc > pool returns nullopt");
}

static void test_fragmentation() {
  std::printf("\n[T7] Fragmented pool blocks contiguous alloc\n");
  // 4 pages: allocate 0 and 2, free them, then try to alloc 3 contiguous.
  auto alloc = TernaryPageAllocator(make_map(4));
  auto p0 = alloc.alloc_page(); // page index 0
  auto p1 = alloc.alloc_page(); // page index 1
  auto p2 = alloc.alloc_page(); // page index 2
  alloc.free_page(*p1);         // free middle → fragmented: free at 1, alloc at 0,2,3 (wait, p3 not alloc'd)
  // State: ALLOC ALLOC FREE ALLOC (p3 never alloc'd)
  // Actually we have 4 pages total; p0,p1,p2 allocated, p3 still free.
  // After freeing p1: ALLOC FREE ALLOC FREE
  (void)p0; (void)p2;
  auto p3 = alloc.alloc_page();  // allocate the last free page
  // Now: ALLOC FREE ALLOC ALLOC — only 1 free page (index 1)
  (void)p3;
  auto r = alloc.alloc_contiguous(2);
  check(!r.has_value(), "contiguous-2 fails in fragmented pool with only 1 free page");
}

static void test_stats_string() {
  std::printf("\n[T8] stats_string is non-empty\n");
  auto alloc = TernaryPageAllocator(make_map(4));
  alloc.alloc_page();
  auto s = alloc.stats_string();
  check(!s.empty(), "stats_string is non-empty");
  std::printf("       %s\n", s.c_str());
}

static void test_page_size_constant() {
  std::printf("\n[T9] kPageSize == 3^10 == 59049\n");
  check(TernaryPageAllocator::kPageSize == 59049, "kPageSize = 59049");
}

static void test_owner_pid_tracking() {
  std::printf("\n[T10] owner_pid is stored correctly\n");
  auto alloc = TernaryPageAllocator(make_map(4));
  auto addr  = alloc.alloc_page(/*owner=*/42);
  check(addr.has_value(), "alloc succeeds");
  if (addr.has_value()) {
    const auto& tbl = alloc.page_table();
    for (const auto& pg : tbl) {
      if (pg.phys_base == *addr) {
        check(pg.owner_pid == 42, "owner_pid == 42 recorded");
        break;
      }
    }
  }
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== TernaryPageAllocator tests ===\n");

  test_construction();
  test_reserved_region();
  test_alloc_and_free();
  test_oom();
  test_contiguous_alloc();
  test_contiguous_oom();
  test_fragmentation();
  test_stats_string();
  test_page_size_constant();
  test_owner_pid_tracking();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
