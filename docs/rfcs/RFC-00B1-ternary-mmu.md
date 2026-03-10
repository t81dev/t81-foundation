# RFC-00B1: Ternary MMU — Virtual Address Translation

**Status:** Draft
**Date:** 2026-03-10
**Author:** @t81dev
**Depends on:** RFC-00B0 (HAL), `experimental/ternaryos/mmu/ternary_page_alloc`
**Blocks:** TernOS Phase 2 (v1.6 gate)

---

## 1. Summary

This RFC defines the virtual address space, page-table structure, and
translate / map / unmap API for the TernOS Ternary Memory Management Unit
(MMU). It resolves the open question from RFC-00B0 §5 regarding mapping 48-bit
binary physical addresses into a trit-addressed virtual space.

---

## 2. Address Space Design

### 2.1 Ternary Virtual Address (TVA)

A TVA is a `uint64_t` interpreted as an unsigned base-3 number. Its structure:

```
  [  upper trits  |  VPN (20 trits)  |  page offset (10 trits)  ]
       unused         virtual page          position within page
```

- **Page offset:** lower 10 trits → values 0..59,048 (3¹⁰ − 1)
  covering exactly one 3¹⁰-tryte page.
- **Virtual page number (VPN):** next 20 trits → 3²⁰ = 3,486,784,401 pages.
- **Total virtual space:** 3³⁰ trytes ≈ 205 TB — sufficient for Phase 2–4.
- **Encoding:** `tva = vpn * kPageSize + offset`, where `kPageSize = 59,049`.
  This is valid because 3³⁰ = 205,891,132,094,649 < 2⁴⁸, fitting in `uint64_t`.

### 2.2 Addressing the binary↔ternary gap

48-bit binary physical addresses do **not** divide cleanly into trit boundaries
(2⁴⁸ = 281,474,976,710,656; 3³⁰ = 205,891,132,094,649 < 2⁴⁸).

**Resolution:** The HAL page allocator carves physical memory into 59,049-byte
pages. A physical page is identified by its `phys_base` (a `uint64_t`). The MMU
stores `phys_base` directly — no binary-to-ternary reencoding of the address
itself is required. The ternary structure lives entirely in the **virtual**
address; the physical side remains a plain byte offset.

This is the "narrow virtual" strategy: the ternary virtual space (3³⁰ ≈ 205 TB)
is smaller than the 48-bit physical space (281 TB), which is acceptable for
Phase 2. RFC-00B2 (if needed) will extend VPN width.

---

## 3. Page Table

### 3.1 Phase 2: Flat hash map

A `std::unordered_map<uint64_t, uint64_t>` mapping VPN → physical page base.
This is sufficient for the Phase 2 hosted prototype and unit-test validation.

### 3.2 Phase 3+: Radix trie (deferred)

A 3-ary trie (branching factor 3) with 10-trit levels matches the natural
ternary page granularity. Deferred to Phase 3; the flat map's API is a strict
subset so migration is non-breaking.

---

## 4. MMU API

```cpp
namespace t81::ternaryos::mmu {

// Map one virtual page (VPN derived from tva) to a physical page.
// Allocates a physical page from TernaryPageAllocator if phys_base is 0.
// Returns false if VPN is already mapped or OOM.
bool mmu_map(PageTable& pt, TernaryPageAllocator& alloc,
             uint64_t tva, uint32_t owner_pid = 0);

// Translate a TVA to a physical byte address.
// Returns nullopt if the VPN is not mapped or the offset is out of range.
std::optional<uint64_t> mmu_translate(const PageTable& pt, uint64_t tva);

// Unmap the virtual page containing tva; frees the physical page.
bool mmu_unmap(PageTable& pt, TernaryPageAllocator& alloc, uint64_t tva);

} // namespace
```

---

## 5. Acceptance Criteria

- [ ] `tva_from_vpn_offset(vpn, offset)` and `tva_vpn(tva)` / `tva_offset(tva)` are inverse operations.
- [ ] `mmu_map` + `mmu_translate` round-trip: translate(map(tva)) returns a valid physical address.
- [ ] `mmu_translate` returns nullopt for unmapped TVA.
- [ ] `mmu_unmap` frees the physical page (page allocator free count increases).
- [ ] Double-map of same VPN returns false.
- [ ] All existing 80 TernOS tests continue to pass.
