// experimental/ternaryos/tests/mmu_test.cpp
//
// Unit tests for the Ternary MMU — RFC-00B1 acceptance criteria.

#include "../mmu/page_table.hpp"
#include "../mmu/tva.hpp"

#include <cassert>
#include <cstdio>

using namespace t81::ternaryos::mmu;
using namespace t81::ternaryos::hal;

static int g_pass = 0;
static int g_fail = 0;

static bool check(bool cond, const char* label) {
  if (cond) { std::printf("  PASS  %s\n", label); ++g_pass; }
  else       { std::printf("  FAIL  %s\n", label); ++g_fail; }
  return cond;
}

// ─── Helper ──────────────────────────────────────────────────────────────────

static TernaryPageAllocator make_alloc(std::size_t n_pages = 32) {
  std::vector<MemoryRegion> map = {{
    .base_phys  = kPageSize,   // page-aligned
    .size_bytes = n_pages * kPageSize,
    .writable   = true,
    .executable = false,
  }};
  return TernaryPageAllocator(map);
}

// ─── TVA arithmetic tests ────────────────────────────────────────────────────

static void test_tva_constants() {
  std::printf("\n[M1] TVA constants\n");
  check(kPageSize == 59049,               "kPageSize == 3^10 == 59049");
  check(kMaxVpn   == 3486784400ULL,       "kMaxVpn == 3^20 - 1");
  check(kMaxTva   == 205891132094648ULL,  "kMaxTva == 3^30 - 1");
}

static void test_tva_decomposition() {
  std::printf("\n[M2] TVA decomposition\n");
  // vpn=1, offset=0 → tva = 59049
  uint64_t tva = tva_from_vpn_offset(1, 0);
  check(tva == 59049,          "vpn=1, offset=0 → tva=59049");
  check(tva_vpn(tva) == 1,     "tva_vpn round-trips");
  check(tva_offset(tva) == 0,  "tva_offset == 0");

  // vpn=0, offset=1000 → tva = 1000
  tva = tva_from_vpn_offset(0, 1000);
  check(tva_vpn(tva) == 0,     "vpn=0 from offset-only tva");
  check(tva_offset(tva) == 1000, "offset=1000 round-trips");

  // vpn=5, offset=3 → tva = 5*59049 + 3 = 295248
  tva = tva_from_vpn_offset(5, 3);
  check(tva_vpn(tva) == 5,     "vpn=5 round-trips");
  check(tva_offset(tva) == 3,  "offset=3 round-trips");
}

static void test_tva_validity() {
  std::printf("\n[M3] tva_valid\n");
  check(tva_valid(0),          "tva=0 is valid");
  check(tva_valid(kMaxTva),    "kMaxTva is valid");
  check(!tva_valid(kMaxTva+1), "kMaxTva+1 is invalid");
}

static void test_trit_utilities() {
  std::printf("\n[M4] trit_at / trit_weight\n");
  // 59049 = 3^10: trit 10 = 1, all lower = 0
  check(trit_at(kPageSize, 10) == 1, "trit 10 of kPageSize == 1");
  check(trit_at(kPageSize,  0) == 0, "trit 0 of kPageSize == 0");
  check(trit_weight(0) == 0,         "trit_weight(0) == 0");
  check(trit_weight(kPageSize) == 1, "trit_weight(3^10) == 1");
  // 13 = 1*9 + 1*3 + 1 = 111 in base-3 → weight 3
  check(trit_weight(13) == 3,        "trit_weight(13) == 3");
}

static void test_tva_to_string() {
  std::printf("\n[M5] tva_to_string\n");
  check(tva_to_string(0)  == "0",   "tva_to_string(0) == \"0\"");
  check(tva_to_string(3)  == "10",  "tva_to_string(3) == \"10\"");  // 1*3 + 0
  check(tva_to_string(9)  == "100", "tva_to_string(9) == \"100\""); // 1*9
  check(tva_to_string(13) == "111", "tva_to_string(13) == \"111\"");
}

// ─── MMU operation tests ─────────────────────────────────────────────────────

static void test_map_and_translate() {
  std::printf("\n[M6] mmu_map + mmu_translate round-trip (RFC-00B1 AC)\n");
  auto alloc = make_alloc(8);
  PageTable pt;

  uint64_t tva = tva_from_vpn_offset(3, 100);
  bool mapped = mmu_map(pt, alloc, tva, /*pid=*/1);
  check(mapped,            "mmu_map succeeds");
  check(pt.size() == 1,    "page table has 1 entry");
  check(alloc.allocated_pages() == 1, "1 physical page consumed");

  auto phys = mmu_translate(pt, tva);
  check(phys.has_value(),  "mmu_translate returns a value");

  // Physical address must end with the page offset.
  if (phys.has_value()) {
    check(*phys % kPageSize == 100, "physical offset == TVA offset (100)");
  }
}

static void test_translate_offset_range() {
  std::printf("\n[M7] Offset variation within same mapped page\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  uint64_t base_tva = tva_from_vpn_offset(7, 0);
  mmu_map(pt, alloc, base_tva);

  auto p0 = mmu_translate(pt, tva_from_vpn_offset(7, 0));
  auto p1 = mmu_translate(pt, tva_from_vpn_offset(7, 1));
  auto pm = mmu_translate(pt, tva_from_vpn_offset(7, kPageSize - 1));

  check(p0.has_value() && p1.has_value() && pm.has_value(),
        "offsets 0, 1, max all translate");
  if (p0 && p1 && pm) {
    check(*p1 == *p0 + 1,            "consecutive offsets are contiguous");
    check(*pm == *p0 + kPageSize - 1, "max offset is page_base + kPageSize-1");
  }
}

static void test_unmapped_returns_nullopt() {
  std::printf("\n[M8] Unmapped TVA returns nullopt (RFC-00B1 AC)\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  auto r = mmu_translate(pt, tva_from_vpn_offset(99, 0));
  check(!r.has_value(), "unmapped VPN → nullopt");
}

static void test_invalid_tva_returns_nullopt() {
  std::printf("\n[M9] Invalid TVA (> kMaxTva) returns nullopt\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  mmu_map(pt, alloc, 0);
  auto r = mmu_translate(pt, kMaxTva + 1);
  check(!r.has_value(), "invalid TVA → nullopt");
  check(!mmu_map(pt, alloc, kMaxTva + 1), "mmu_map with invalid TVA returns false");
}

static void test_double_map() {
  std::printf("\n[M10] Double-map same VPN returns false (RFC-00B1 AC)\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  uint64_t tva = tva_from_vpn_offset(2, 0);
  check(mmu_map(pt, alloc, tva),  "first map succeeds");
  check(!mmu_map(pt, alloc, tva), "second map of same VPN returns false");
  check(pt.size() == 1,           "only one entry in table");
  check(alloc.allocated_pages() == 1, "only one physical page consumed");
}

static void test_unmap_frees_physical() {
  std::printf("\n[M11] mmu_unmap frees physical page (RFC-00B1 AC)\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  uint64_t tva = tva_from_vpn_offset(1, 500);

  mmu_map(pt, alloc, tva);
  check(alloc.free_pages() == 3, "3 free pages after map");

  bool ok = mmu_unmap(pt, alloc, tva);
  check(ok,                       "mmu_unmap returns true");
  check(pt.empty(),               "page table empty after unmap");
  check(alloc.free_pages() == 4,  "4 free pages after unmap (physical freed)");

  // After unmap the TVA should no longer translate.
  check(!mmu_translate(pt, tva).has_value(), "unmapped TVA → nullopt after unmap");
}

static void test_unmap_nonexistent() {
  std::printf("\n[M12] mmu_unmap of unmapped VPN returns false\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  check(!mmu_unmap(pt, alloc, tva_from_vpn_offset(5, 0)),
        "unmap of unmapped VPN returns false");
}

static void test_multiple_mappings() {
  std::printf("\n[M13] Multiple independent VPN mappings\n");
  auto alloc = make_alloc(8);
  PageTable pt;

  for (uint64_t vpn = 0; vpn < 5; ++vpn) {
    mmu_map(pt, alloc, tva_from_vpn_offset(vpn, 0), /*pid=*/vpn + 1);
  }
  check(pt.size() == 5,               "5 entries in page table");
  check(alloc.allocated_pages() == 5, "5 physical pages allocated");

  // Each VPN maps to a distinct physical page.
  std::vector<uint64_t> bases;
  bool distinct = true;
  for (uint64_t vpn = 0; vpn < 5; ++vpn) {
    auto p = mmu_translate(pt, tva_from_vpn_offset(vpn, 0));
    if (!p) { distinct = false; break; }
    for (auto b : bases) if (b == *p) { distinct = false; break; }
    bases.push_back(*p);
  }
  check(distinct, "all 5 VPNs map to distinct physical pages");
}

static void test_page_table_dump() {
  std::printf("\n[M14] page_table_dump is non-empty\n");
  auto alloc = make_alloc(4);
  PageTable pt;
  mmu_map(pt, alloc, tva_from_vpn_offset(10, 0), /*pid=*/7);
  auto s = page_table_dump(pt);
  check(!s.empty(), "dump is non-empty");
  check(s.find("radix_nodes=") != std::string::npos,
        "dump includes radix node count");
  std::printf("       %s", s.c_str());
}

static void test_sparse_radix_mappings() {
  std::printf("\n[M15] Sparse VPNs share radix structure without collision\n");
  auto alloc = make_alloc(8);
  PageTable pt;

  const uint64_t near_vpn = 1;
  const uint64_t far_vpn = kMaxVpn - 1;
  check(mmu_map(pt, alloc, tva_from_vpn_offset(near_vpn, 0), /*pid=*/11),
        "map near VPN succeeds");
  check(mmu_map(pt, alloc, tva_from_vpn_offset(far_vpn, 0), /*pid=*/12),
        "map far VPN succeeds");

  const auto near_phys = mmu_translate(pt, tva_from_vpn_offset(near_vpn, 42));
  const auto far_phys = mmu_translate(pt, tva_from_vpn_offset(far_vpn, 42));
  check(near_phys.has_value(), "near VPN translates");
  check(far_phys.has_value(),  "far VPN translates");
  if (near_phys && far_phys) {
    check(*near_phys != *far_phys, "sparse VPNs map to distinct physical pages");
    check(*near_phys % kPageSize == 42, "near VPN preserves offset");
    check(*far_phys % kPageSize == 42,  "far VPN preserves offset");
  }
}

static void test_unmap_prunes_without_affecting_siblings() {
  std::printf("\n[M16] Unmap prunes radix path without harming siblings\n");
  auto alloc = make_alloc(8);
  PageTable pt;

  const uint64_t vpn_a = 3;
  const uint64_t vpn_b = 4;
  check(mmu_map(pt, alloc, tva_from_vpn_offset(vpn_a, 0), /*pid=*/21),
        "map first sibling VPN succeeds");
  check(mmu_map(pt, alloc, tva_from_vpn_offset(vpn_b, 0), /*pid=*/22),
        "map second sibling VPN succeeds");
  check(mmu_unmap(pt, alloc, tva_from_vpn_offset(vpn_a, 0)),
        "unmap first sibling succeeds");
  check(!mmu_translate(pt, tva_from_vpn_offset(vpn_a, 0)).has_value(),
        "unmapped sibling no longer translates");
  check(mmu_translate(pt, tva_from_vpn_offset(vpn_b, 17)).has_value(),
        "remaining sibling still translates");
}

static void test_page_table_stats() {
  std::printf("\n[M17] page_table_stats reflects radix structure\n");
  auto alloc = make_alloc(8);
  PageTable pt;
  check(mmu_map(pt, alloc, tva_from_vpn_offset(1, 0), /*pid=*/31),
        "first stats mapping succeeds");
  check(mmu_map(pt, alloc, tva_from_vpn_offset(2, 0), /*pid=*/32),
        "second stats mapping succeeds");

  const auto stats = page_table_stats(pt);
  check(stats.mapped_entries == 2, "stats count mapped entries");
  check(stats.leaf_entries == 2, "stats count leaf entries");
  check(stats.radix_nodes > stats.leaf_entries, "radix has internal structure");
  check(stats.branch_nodes >= 1, "stats count at least one branch node");
  check(stats.max_depth == 20, "stats report full 20-trit walk depth");
}

static void test_page_table_trace() {
  std::printf("\n[M18] page_table_trace exposes hit and miss paths\n");
  auto alloc = make_alloc(8);
  PageTable pt;
  const uint64_t mapped_tva = tva_from_vpn_offset(5, 7);
  check(mmu_map(pt, alloc, mapped_tva, /*pid=*/41), "trace mapping succeeds");

  const auto hit = page_table_trace(pt, mapped_tva);
  const auto miss = page_table_trace(pt, tva_from_vpn_offset(6, 7));
  const auto invalid = page_table_trace(pt, kMaxTva + 1);

  check(hit.find("hit phys=0x") != std::string::npos, "trace reports hit");
  check(hit.find("pid=41") != std::string::npos, "trace includes owner pid");
  check(miss.find("miss@") != std::string::npos, "trace reports miss depth");
  check(invalid.find("invalid") != std::string::npos, "trace reports invalid TVA");
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Ternary MMU tests (RFC-00B1) ===\n");

  test_tva_constants();
  test_tva_decomposition();
  test_tva_validity();
  test_trit_utilities();
  test_tva_to_string();
  test_map_and_translate();
  test_translate_offset_range();
  test_unmapped_returns_nullopt();
  test_invalid_tva_returns_nullopt();
  test_double_map();
  test_unmap_frees_physical();
  test_unmap_nonexistent();
  test_multiple_mappings();
  test_page_table_dump();
  test_sparse_radix_mappings();
  test_unmap_prunes_without_affecting_siblings();
  test_page_table_stats();
  test_page_table_trace();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return (g_fail == 0) ? 0 : 1;
}
