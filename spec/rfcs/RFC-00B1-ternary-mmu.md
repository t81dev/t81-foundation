# RFC-00B1: Ternary MMU — Virtual Address Translation

**Status:** accepted
**Type:** standards-track
**Applies-To:** Axion MMU, TVA layout, radix page table, MMU fault classification
**Created:** 2026-03-10
**Updated:** 2026-03-11
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

## 2. Motivation

The Axion memory model must stop pretending a flat binary page map is "ternary
enough." TVA layout, radix structure, and explicit fault semantics are required
so the MMU can become a real kernel subsystem rather than just a test helper.

## 3. Proposal

### 3.1 Address Space Design

#### 3.1.1 Ternary Virtual Address (TVA)

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

#### 3.1.2 Addressing the binary↔ternary gap

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

### 3.2 Page Table

#### 3.2.1 Implemented structure: 20-trit radix walk

The current `PageTable` is a ternary radix tree over the 20-trit VPN. Each
level consumes one VPN trit and branches over `{0,1,2}`. After 20 steps, the
leaf stores:

- `phys_base`
- `owner_pid`
- `readable`
- `writable`
- `executable`

This makes the page-table shape match the ternary virtual-address model while
preserving the same external MMU API.

#### 3.2.2 Diagnostics surface

The implementation still exposes a read-only flat VPN cache for diagnostics and
compatibility, but the radix tree is the canonical translation structure.

---

### 3.3 MMU API

```cpp
namespace t81::ternaryos::mmu {

struct PagePermissions {
  bool readable{true};
  bool writable{true};
  bool executable{false};
};

enum class MmuAccessMode { Read, Write, Execute };
enum class MmuFault { None, InvalidTva, Unmapped, PermissionDenied };

struct MmuAccessResult {
  std::optional<uint64_t> phys_addr;
  MmuFault fault{MmuFault::None};
};

// Map one virtual page (VPN derived from tva) to a physical page.
// Allocates a physical page from TernaryPageAllocator if phys_base is 0.
// Returns false if VPN is already mapped or OOM.
bool mmu_map(PageTable& pt, TernaryPageAllocator& alloc,
             uint64_t tva, uint32_t owner_pid = 0,
             PagePermissions perms = {});

// Translate a TVA to a physical byte address.
// Returns nullopt if the VPN is not mapped or the offset is out of range.
std::optional<uint64_t> mmu_translate(const PageTable& pt, uint64_t tva);

// Translate a TVA for a specific access class and classify failures.
MmuAccessResult mmu_translate_checked(const PageTable& pt, uint64_t tva,
                                     MmuAccessMode mode);

// Unmap the virtual page containing tva; frees the physical page.
bool mmu_unmap(PageTable& pt, TernaryPageAllocator& alloc, uint64_t tva);

// Structural diagnostics for the radix tree.
PageTableStats page_table_stats(const PageTable& pt);
std::string page_table_trace(const PageTable& pt, uint64_t tva);

} // namespace
```

---

## 4. Determinism / Safety Considerations

- TVA decoding and radix traversal must be deterministic for identical input addresses.
- Fault classification must distinguish malformed addresses from absent mappings and permission failures.
- Permission bits are part of the trusted memory boundary and should be surfaced in diagnostics.
- The MMU must remain compatible with the deterministic scheduler and later fault-reporting path.

## 5. Compatibility

- The stable `mmu_map` / `mmu_translate` / `mmu_unmap` API remains intact.
- `mmu_translate_checked()` extends the surface without breaking existing callers.
- The read-only flat VPN cache remains available for diagnostics while the radix tree is canonical.

## 6. Implementation Plan

1. Keep TVA helpers and allocator semantics stable.
2. Use the radix walk as the canonical translation structure.
3. Preserve compatibility through the existing MMU API.
4. Extend translation with checked access modes and fault reporting.
5. Feed this model into the later kernel fault/reporting path.

## 7. Open Questions

- Should a future wider TVA design replace the current 30-trit "narrow virtual" space?
- Should permission state eventually grow beyond simple read/write/execute bits?
- How much of the diagnostics surface should remain API-stable once the kernel fault path exists?

## 8. Acceptance Criteria

- [ ] `tva_from_vpn_offset(vpn, offset)` and `tva_vpn(tva)` / `tva_offset(tva)` are inverse operations.
- [ ] `mmu_map` + `mmu_translate` round-trip: translate(map(tva)) returns a valid physical address.
- [ ] `mmu_translate` returns nullopt for unmapped TVA.
- [ ] `mmu_unmap` frees the physical page (page allocator free count increases).
- [ ] Double-map of same VPN returns false.
- [ ] Sparse far-apart VPNs translate correctly through the radix walk.
- [ ] Unmapping one VPN prunes its empty radix branch without affecting siblings.
- [ ] `page_table_dump()` exposes radix structural diagnostics.
- [ ] `page_table_stats()` reports mapped entries and radix shape.
- [ ] `page_table_trace()` reports hit, miss, and invalid-TVAs deterministically.
- [ ] Read-only mappings reject writes with `PermissionDenied`.
- [ ] Non-executable mappings reject execute translations with `PermissionDenied`.
- [ ] Checked translation distinguishes invalid TVA, unmapped TVA, and permission faults.
- [ ] Diagnostics expose per-leaf permission state.
- [ ] All existing TernOS tests continue to pass.
