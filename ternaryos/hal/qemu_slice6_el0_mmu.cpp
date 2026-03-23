// ternaryos/hal/qemu_slice6_el0_mmu.cpp
//
// Freestanding EL0 page table setup for the QEMU AArch64 slice6 EFI bridge.
//
// Implements Phase 5 + Phase 7 of RFC-00BC: replaces the EDK2 identity-mapped
// TTBR0 with a minimal controlled page table that maps four 4 KB pages into the
// EL0 address space:
//
//   1. Init code page   — the 4 KB page containing axion_el0_entry (R+X EL0)
//   2. Init stack page  — dedicated 4 KB BSS page for EL0 init (R/W, no-exec)
//   3. Proc code page   — 4 KB BSS page for CanonFS-loaded process code (R+X)
//   4. Proc stack page  — dedicated 4 KB BSS page for loaded process (R/W, NX)
//
// All other VA regions are unmapped in TTBR0; any EL0 access outside these four
// pages causes a translation fault.  The SVC dispatcher uses el0_tva_valid()
// to validate pointers passed from EL0 before dereferencing them at EL1.
//
// Design notes (see RFC-00BC §"Relationship to Other Accepted RFCs"):
//
//   - We do NOT modify TCR_EL1 or MAIR_EL1.  EDK2 leaves both in a correct
//     state (T0SZ=25 / 39-bit VA, MAIR index 0 = normal WB memory).  Changing
//     TCR_EL1 while the EL1 instruction pointer is in the TTBR0 VA region
//     (0x40000000 for QEMU virt) would create a window where TTBR0 is indexed
//     with a new T0SZ but still holds the old EDK2 table — a guaranteed fault.
//     Leaving TCR unchanged and only swapping TTBR0 is safe.
//
//   - Page tables live in BSS (zero at startup, identity-mapped by EDK2).
//     The L1 table has 512 entries matching EDK2's expected T0SZ=25 layout.
//
//   - All physical addresses equal virtual addresses (EDK2 identity mapping),
//     so setting ELR_EL1 / SP_EL0 to the physical address of the code / stack
//     page works without any VA translation at the call site.
//
// Compilation constraints (same as qemu_slice6_bridge_irq.cpp):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=aarch64-pc-windows-msvc

#include <stdint.h>

// axion_el0_entry is the EL0 entry point (axion_el0_init.S).
// We derive the code page PA from its address.
extern "C" void axion_el0_entry() noexcept;

// ── AArch64 page descriptor constants (4 KB granule) ─────────────────────────
//
// Leaf entries at L3 use bit[1:0]=0b11 (valid + table/page).
// Table entries at L1/L2 also use bit[1:0]=0b11.
//
// AP[2:1] (bits [7:6]):
//   0b00 — EL1 R/W,  EL0 no access
//   0b01 — EL1 R/W,  EL0 R/W
//   0b10 — EL1 R,    EL0 no access
//   0b11 — EL1 R,    EL0 R    (used for code page: EL0 read + execute via UXN=0)
//
// UXN (bit 54): 1 = EL0 execute-never
// PXN (bit 53): 1 = EL1 execute-never

static constexpr uint64_t kPD_Valid    = (1ULL <<  0);
static constexpr uint64_t kPD_Table    = (1ULL <<  1);  // table or page entry
static constexpr uint64_t kPD_AttrIdx0 = (0ULL <<  2);  // MAIR[0] = normal WB
static constexpr uint64_t kPD_AP_RO    = (3ULL <<  6);  // EL0 R / EL1 R
static constexpr uint64_t kPD_AP_RW    = (1ULL <<  6);  // EL0 R/W / EL1 R/W
static constexpr uint64_t kPD_SH_Inner = (3ULL <<  8);  // inner shareable
static constexpr uint64_t kPD_AF       = (1ULL << 10);  // access flag (fault if 0)
static constexpr uint64_t kPD_PXN      = (1ULL << 53);  // EL1 execute-never
static constexpr uint64_t kPD_UXN      = (1ULL << 54);  // EL0 execute-never

// Code page leaf: EL0 read + execute (UXN=0), EL1 read + execute (PXN cleared).
// PXN is intentionally not set here: the EFI binary's code is a single flat
// .text section, so axion_el0_entry and EL1 functions (e.g. el0_svc_kernel_call_dispatch)
// share the same 4 KB page.  Setting PXN=1 would prevent EL1 from executing from
// that page and cause a permission fault on the first SVC dispatch.
// EL0 cannot write (AP_RO = AP[2:1]=11 = EL0 R / EL1 R).
static constexpr uint64_t kLeafCode =
    kPD_Valid | kPD_Table | kPD_AttrIdx0 | kPD_AP_RO |
    kPD_SH_Inner | kPD_AF;

// Proc code page leaf (Phase 7): EL0 read/write/execute, EL1 read/write/execute.
// AP_RW (AP[2:1]=01) is required so that canon_exec_load_and_run() can copy the
// T81X code section into this page from EL1 AFTER el0_mmu_init() has installed
// the new TTBR0.  Using AP_RO here would cause an EL1 permission fault on the
// first memcpy byte, silently hanging the system.
// UXN=0 / PXN=0: both EL0 and EL1 may execute from this page.
static constexpr uint64_t kLeafProcCode =
    kPD_Valid | kPD_Table | kPD_AttrIdx0 | kPD_AP_RW |
    kPD_SH_Inner | kPD_AF;

// Stack page leaf: EL0 read/write, no execute (UXN=1, PXN=1).
static constexpr uint64_t kLeafStack =
    kPD_Valid | kPD_Table | kPD_AttrIdx0 | kPD_AP_RW |
    kPD_SH_Inner | kPD_AF | kPD_UXN | kPD_PXN;

// Table descriptor: points to the next-level page table.
static constexpr uint64_t kTableDesc = kPD_Valid | kPD_Table;

// ── Static page tables (BSS — zero at startup) ───────────────────────────────
//
// EDK2 AAVMF uses T0SZ=25 (39-bit VA).  With 4 KB granule:
//   L1 index: VA[38:30] — 9 bits → 512 entries, each covers 1 GB
//   L2 index: VA[29:21] — 9 bits → 512 entries, each covers 2 MB
//   L3 index: VA[20:12] — 9 bits → 512 entries, each covers 4 KB
//
// The L1 array has 512 entries to match T0SZ=25.  Only the entry for the 1 GB
// region containing the EFI binary (typically 0x40000000–0x7FFFFFFF, L1[1]) is
// populated; all others remain zero (unmapped → EL0 fault if accessed).

// Root table: for T0SZ=25 (3-level) this is the L1 table (512 × 1 GB);
//             for T0SZ<25  (4-level) this is the L0 table (only 32 entries used for 44-bit VA).
alignas(4096) static uint64_t s_l1[512];
// Intermediate L1 table used only for 4-level walks (T0SZ < 25).
// When needed, s_l1[0] is redirected to point here instead of EDK2's L1.
alignas(4096) static uint64_t s_l1b[512];
alignas(4096) static uint64_t s_l2[512];         // L2: 512 × 2 MB (one 1 GB bank)
alignas(4096) static uint64_t s_l3_a[512];       // L3 for first 2 MB block
alignas(4096) static uint64_t s_l3_b[512];       // L3 for second 2 MB block (if needed)
alignas(4096) static uint64_t s_l3_c[512];       // L3 for third 2 MB block (if needed)

// 4 KB EL0 init stack page (replaces the Phase-4 s_el0_stack[1024] in cpp_bridge).
alignas(4096) static uint8_t s_el0_stack_page[4096];

// 4 KB pages for Phase 7 CanonFS-loaded process.
alignas(4096) static uint8_t s_el0_proc_code_page[4096];
alignas(4096) static uint8_t s_el0_proc_stack_page[4096];

// 4 KB pages for RFC-00C5 second concurrent process slot.
alignas(4096) static uint8_t s_el0_proc_code_page2[4096];
alignas(4096) static uint8_t s_el0_proc_stack_page2[4096];

// ── RFC-00C6: per-thread L3 tables ───────────────────────────────────────────
//
// Each concurrent thread gets a private L3 table (512 × 8 = 4 KB) for the
// 2 MB block containing all EL0 proc pages.  The table is cloned from the
// shared L3 baseline, then the other threads' proc code/stack entries are
// remapped to EL1-only (AP=0b00, UXN=1) so EL0 cannot reach them.
//
// Isolation is enforced by swapping s_l2[s_l2_el0_block_idx] to the thread's
// private L3 pointer before each ERET, and restoring the shared L3 pointer
// when all threads have exited back to EL1.
//
// Two slots cover the Phase 16/17 concurrent pair; extend kMaxThreadL3Slots
// if more threads are added.
static constexpr uint32_t kMaxThreadL3Slots = 2u;
alignas(4096) static uint64_t s_thread_l3[kMaxThreadL3Slots][512];

// ── TVA state (written once by el0_mmu_init, read by el0_tva_valid) ──────────
static uint64_t s_code_page_pa        = 0;
static uint64_t s_stack_page_pa       = 0;
static uint64_t s_proc_code_page_pa   = 0;
static uint64_t s_proc_stack_page_pa  = 0;
static uint64_t s_proc_code_page2_pa  = 0;
static uint64_t s_proc_stack_page2_pa = 0;

// ── RFC-00C6: runtime state set by el0_mmu_init ───────────────────────────────
static uint64_t* s_shared_l3          = nullptr;  // L3 for the EL0 pages block
static uint32_t  s_l2_el0_block_idx   = 0u;       // s_l2 index for that block
// All proc page PAs (used to strip EL0 access from other thread's pages).
static uint64_t  s_all_proc_page_pas[4];
static uint32_t  s_all_proc_page_count = 0u;

// ── System-register helpers (AArch64 only) ───────────────────────────────────

#if defined(__aarch64__) && !defined(__APPLE__)

static inline void aarch64_dsb_sy() noexcept {
    __asm__ volatile("dsb sy" ::: "memory");
}

static inline uint64_t read_ttbr0_el1() noexcept {
    uint64_t v;
    __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(v) :: "memory");
    return v;
}

static inline void write_ttbr0_el1(uint64_t v) noexcept {
    __asm__ volatile("msr ttbr0_el1, %0\n\tisb" :: "r"(v) : "memory");
}

static inline void tlbi_vmalle1() noexcept {
    // Invalidate all stage-1 EL1&0 TLB entries (TTBR0 regime).
    __asm__ volatile("tlbi vmalle1\n\tdsb sy\n\tisb" ::: "memory");
}

static inline uint64_t read_tcr_el1() noexcept {
    uint64_t v;
    __asm__ volatile("mrs %0, tcr_el1" : "=r"(v) :: "memory");
    return v;
}

#else

static inline void aarch64_dsb_sy()          noexcept {}
static inline uint64_t read_ttbr0_el1()      noexcept { return 0; }
static inline uint64_t read_tcr_el1()        noexcept { return 0; }
static inline void write_ttbr0_el1(uint64_t) noexcept {}
static inline void tlbi_vmalle1()             noexcept {}

#endif

// ── Page table builder ────────────────────────────────────────────────────────
//
// install_el0_page() adds one EL0 page to the already-populated s_l2.
// The 2 MB block containing `pa` is split into an L3 table:
//   – If the L2 entry is a 2 MB block descriptor: expand it into 512 4 KB
//     pages that replicate the block's attributes (preserving EL1 access to
//     the rest of the 2 MB range), then override the specific 4 KB entry.
//   – If the L2 entry already points to an L3 table from EDK2: copy the
//     table into one of our L3 pool arrays (so we own it) then override.
//   – If the L2 entry is already one of our L3 pool tables: override in-place.
//
// The l3_pool has 3 slots.  On QEMU virt the 22 KB EFI binary and all BSS
// pages share one 2 MB block, so typically only 1 slot is consumed.

static void install_el0_page(uint64_t pa, uint64_t leaf,
                              uint64_t* l3_pool[3], uint32_t& l3_used) noexcept {
    const uint32_t l2_idx = static_cast<uint32_t>((pa >> 21) & 0x1FFu);
    const uint32_t l3_idx = static_cast<uint32_t>((pa >> 12) & 0x1FFu);

    const uint64_t l2e = s_l2[l2_idx];
    uint64_t* tbl = nullptr;

    if ((l2e & 3u) == 3u) {
        // L2 entry is already a table descriptor.
        tbl = reinterpret_cast<uint64_t*>(l2e & ~0xFFFULL);
        // If it belongs to EDK2 (not one of our arrays), copy it so we own it.
        if (tbl != s_l3_a && tbl != s_l3_b && tbl != s_l3_c) {
            if (l3_used >= 3u) return;  // pool exhausted — silently skip
            uint64_t* own = l3_pool[l3_used++];
            for (int j = 0; j < 512; ++j) own[j] = tbl[j];
            s_l2[l2_idx] = reinterpret_cast<uint64_t>(own) | kTableDesc;
            tbl = own;
        }
    } else if ((l2e & 1u)) {
        // L2 entry is a 2 MB block descriptor — expand it into 512 4 KB pages.
        if (l3_used >= 3u) return;
        tbl = l3_pool[l3_used++];
        // Extract PA base and attribute bits from the block descriptor.
        const uint64_t blk_pa    = l2e & 0x0000FFFFFFE00000ULL;
        const uint64_t blk_upper = l2e & 0xFFFF000000000000ULL;
        const uint64_t blk_lower = l2e & 0x0000000000000FFCULL;
        for (int j = 0; j < 512; ++j) {
            // Convert block bits[1:0]=01 to page bits[1:0]=11; keep attrs.
            tbl[j] = blk_pa | ((uint64_t)j << 12) | blk_upper | blk_lower | 3ULL;
        }
        s_l2[l2_idx] = reinterpret_cast<uint64_t>(tbl) | kTableDesc;
    } else {
        // L2 entry is 0 (unmapped) — allocate a fresh zeroed L3 table.
        if (l3_used >= 3u) return;
        tbl = l3_pool[l3_used++];
        // tbl is already zeroed (BSS or zeroed in el0_mmu_init below).
        s_l2[l2_idx] = reinterpret_cast<uint64_t>(tbl) | kTableDesc;
    }

    tbl[l3_idx] = (pa & ~0xFFFULL) | leaf;
}


// ── Public API ────────────────────────────────────────────────────────────────

/// Build and activate the EL0 page tables.  Call once before run_el0_init().
///
/// Strategy (RFC-00BC §"Phase 5"):
///   1. Read the current TTBR0_EL1 (EDK2 identity-map L1 table).
///   2. Copy all 512 L1 entries into s_l1.  This preserves every EL1 mapping;
///      EL1 code at 0x40000000 remains accessible after the TTBR0 swap.
///   3. For the 1 GB region containing our EL0 pages, ensure s_l1 points to
///      our s_l2 (copy EDK2's L2 table if needed, or expand a 1 GB block).
///   4. For each of the four EL0 pages, call install_el0_page() which splits
///      the containing 2 MB block into an owned L3 table, replicates all 512
///      existing 4 KB pages (preserving EL1 access), then overrides the
///      specific entry with EL0-appropriate AP bits.
///   5. DSB → write new TTBR0_EL1 → TLB invalidate.
///
/// After this call el0_tva_valid() can validate EL0-supplied SVC pointers.
extern "C" void el0_mmu_init() noexcept {
    s_code_page_pa        = reinterpret_cast<uint64_t>(
                                reinterpret_cast<void*>(axion_el0_entry)) & ~0xFFFULL;
    s_stack_page_pa       = reinterpret_cast<uint64_t>(s_el0_stack_page);
    s_proc_code_page_pa   = reinterpret_cast<uint64_t>(s_el0_proc_code_page);
    s_proc_stack_page_pa  = reinterpret_cast<uint64_t>(s_el0_proc_stack_page);
    s_proc_code_page2_pa  = reinterpret_cast<uint64_t>(s_el0_proc_code_page2);
    s_proc_stack_page2_pa = reinterpret_cast<uint64_t>(s_el0_proc_stack_page2);

    // RFC-00C6: record all proc page PAs for per-thread L3 isolation.
    s_all_proc_page_pas[0] = s_proc_code_page_pa;
    s_all_proc_page_pas[1] = s_proc_stack_page_pa;
    s_all_proc_page_pas[2] = s_proc_code_page2_pa;
    s_all_proc_page_pas[3] = s_proc_stack_page2_pa;
    s_all_proc_page_count  = 4u;

    // RFC-00C6: record the L2 index for the EL0 pages 2 MB block.
    // All proc pages are in the same 2 MB bank as the EFI binary (BSS).
    s_l2_el0_block_idx = static_cast<uint32_t>((s_proc_code_page_pa >> 21) & 0x1FFu);

#if defined(__aarch64__) && !defined(__APPLE__)
    // Zero our page table arrays.
    for (auto& e : s_l1)   e = 0;
    for (auto& e : s_l1b)  e = 0;
    for (auto& e : s_l2)   e = 0;
    for (auto& e : s_l3_a) e = 0;
    for (auto& e : s_l3_b) e = 0;
    for (auto& e : s_l3_c) e = 0;

    // Determine the walk depth from TCR_EL1.T0SZ.
    //   T0SZ ≥ 25  → 3-level walk (root = L1, no L0).  TTBR0 points to L1[512].
    //   T0SZ < 25  → 4-level walk (root = L0).           TTBR0 points to L0 (≤512 entries).
    const uint32_t t0sz = static_cast<uint32_t>(read_tcr_el1() & 0x3Fu);
    const bool four_level = (t0sz < 25u);

    // Step 1: Clone the root table (L0 or L1 depending on T0SZ).
    const uint64_t edk2_ttbr0 = read_ttbr0_el1();
    const auto* edk2_root = reinterpret_cast<const uint64_t*>(edk2_ttbr0 & ~0xFFFULL);
    for (int i = 0; i < 512; ++i) s_l1[i] = edk2_root[i];

    // Pointer to the L1 table we will modify.  For 3-level walks this is s_l1.
    // For 4-level walks we clone EDK2's L1 into s_l1b and redirect s_l1[0] to it.
    uint64_t* our_l1 = s_l1;

    if (four_level) {
        // 4-level walk: s_l1 is the L0 root (root has < 512 used entries).
        // L0 index for all QEMU addresses (< 2^39) is 0.
        const uint64_t l0_entry = s_l1[0];
        if ((l0_entry & 3u) == 3u) {
            // L0[0] is a table descriptor → clone EDK2's L1 into s_l1b.
            const auto* edk2_l1 = reinterpret_cast<const uint64_t*>(l0_entry & ~0xFFFULL);
            for (int i = 0; i < 512; ++i) s_l1b[i] = edk2_l1[i];
        } else {
            // L0[0] is a 512 GB block (unusual) — zero our L1; install handles it.
            for (auto& e : s_l1b) e = 0;
        }
        // Redirect L0[0] to our owned L1 copy.
        s_l1[0] = reinterpret_cast<uint64_t>(s_l1b) | kTableDesc;
        our_l1 = s_l1b;
    }

    // Step 2: Populate s_l2 for the 1 GB bank that holds all our EL0 pages.
    const uint32_t l1_idx   = static_cast<uint32_t>((s_code_page_pa >> 30) & 0x1FFu);
    const uint64_t l1_entry = our_l1[l1_idx];

    if ((l1_entry & 3u) == 3u) {
        // Table descriptor → copy EDK2's L2 table into s_l2.
        const auto* edk2_l2 = reinterpret_cast<const uint64_t*>(l1_entry & ~0xFFFULL);
        for (int i = 0; i < 512; ++i) s_l2[i] = edk2_l2[i];
    } else if ((l1_entry & 1u)) {
        // Block descriptor (1 GB) → expand into 512 2 MB block entries.
        const uint64_t blk_pa    = l1_entry & 0x0000FFFFC0000000ULL;
        const uint64_t blk_upper = l1_entry & 0xFFFF000000000000ULL;
        const uint64_t blk_lower = l1_entry & 0x0000000000000FFCULL;
        for (int i = 0; i < 512; ++i) {
            s_l2[i] = blk_pa | ((uint64_t)i << 21) | blk_upper | blk_lower | 1ULL;
        }
    }
    // else: entry is 0 (unmapped) — s_l2 stays zeroed; install_el0_page handles it.
    our_l1[l1_idx] = reinterpret_cast<uint64_t>(s_l2) | kTableDesc;

    // Step 4: Add the six EL0 pages, splitting 2 MB blocks as needed.
    // All pages share one BSS 2 MB block so at most one L3 slot is consumed.
    uint64_t* l3_pool[3] = { s_l3_a, s_l3_b, s_l3_c };
    uint32_t  l3_used = 0u;
    install_el0_page(s_code_page_pa,         kLeafCode,     l3_pool, l3_used);
    install_el0_page(s_stack_page_pa,        kLeafStack,    l3_pool, l3_used);
    install_el0_page(s_proc_code_page_pa,    kLeafProcCode, l3_pool, l3_used);
    install_el0_page(s_proc_stack_page_pa,   kLeafStack,    l3_pool, l3_used);
    // RFC-00C5: second process slot.
    install_el0_page(s_proc_code_page2_pa,   kLeafProcCode, l3_pool, l3_used);
    install_el0_page(s_proc_stack_page2_pa,  kLeafStack,    l3_pool, l3_used);

    // RFC-00C6: capture the shared L3 pointer for this block so
    // el0_mmu_build_thread_l3() can clone it as a per-thread baseline.
    s_shared_l3 = reinterpret_cast<uint64_t*>(
                      s_l2[s_l2_el0_block_idx] & ~0xFFFULL);

    // Step 5: Flush → install → TLB invalidate.
    aarch64_dsb_sy();
    write_ttbr0_el1(reinterpret_cast<uint64_t>(s_l1));
    tlbi_vmalle1();
#endif
}

/// Returns the top of the EL0 init stack page.  Pass as SP_EL0 for Phase 5/6.
extern "C" uint64_t el0_mmu_stack_top() noexcept {
    return s_stack_page_pa + 4096u;
}

/// Returns a writable pointer to the process code page (Phase 7 loader target).
/// The caller copies the T81X code section here before EReting to EL0.
extern "C" uint8_t* el0_mmu_proc_code_page() noexcept {
    return s_el0_proc_code_page;
}

/// Returns the top of the EL0 process stack page.  Pass as SP_EL0 for Phase 7.
extern "C" uint64_t el0_mmu_proc_stack_top() noexcept {
    return s_proc_stack_page_pa + 4096u;
}

/// Returns a writable pointer to the second process code page (RFC-00C5).
extern "C" uint8_t* el0_mmu_proc_code_page2() noexcept {
    return s_el0_proc_code_page2;
}

/// Returns the top of the second process stack page (RFC-00C5).
extern "C" uint64_t el0_mmu_proc_stack_top2() noexcept {
    return s_proc_stack_page2_pa + 4096u;
}

// ── RFC-00C6: per-thread L3 table management ──────────────────────────────────

// Page leaf for proc pages that must not be EL0-accessible in a given thread's
// view: AP[2:1]=0b00 = EL1 R/W / EL0 no access; UXN = EL0 execute-never.
static constexpr uint64_t kLeafProcEl1Only =
    kPD_Valid | kPD_Table | kPD_AttrIdx0 | kPD_SH_Inner | kPD_AF | kPD_UXN;

/// Build an isolated L3 table for thread slot `slot`.
///
/// Clones the shared L3 baseline, then:
///   – Strips EL0 access from ALL proc pages (all 4 code/stack page pairs).
///   – Re-enables EL0 access for `own_code_pa` (R/W/X) and `own_stack_pa` (R/W, NX).
///
/// `own_code_pa`  — physical address of the thread's 4 KB code page base.
/// `own_stack_pa` — physical address of the thread's 4 KB stack page base
///                  (= stack_top - 4096).
extern "C" void el0_mmu_build_thread_l3(uint32_t slot,
                                          uint64_t own_code_pa,
                                          uint64_t own_stack_pa) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    if (slot >= kMaxThreadL3Slots || !s_shared_l3) return;
    uint64_t* tbl = s_thread_l3[slot];
    // Clone shared L3 (preserves EL1 mappings and init code/stack entries).
    for (int i = 0; i < 512; ++i) tbl[i] = s_shared_l3[i];
    // Strip EL0 access from every registered proc page.
    for (uint32_t i = 0u; i < s_all_proc_page_count; ++i) {
        const uint32_t idx = static_cast<uint32_t>(
                                 (s_all_proc_page_pas[i] >> 12) & 0x1FFu);
        tbl[idx] = (s_all_proc_page_pas[i] & ~0xFFFULL) | kLeafProcEl1Only;
    }
    // Re-enable own code page (EL0 R/W/X) and own stack (EL0 R/W, NX).
    tbl[(own_code_pa  >> 12) & 0x1FFu] = (own_code_pa  & ~0xFFFULL) | kLeafProcCode;
    tbl[(own_stack_pa >> 12) & 0x1FFu] = (own_stack_pa & ~0xFFFULL) | kLeafStack;
#else
    (void)slot; (void)own_code_pa; (void)own_stack_pa;
#endif
}

/// Install thread `slot`'s private L3 by swapping the L2 entry for the EL0
/// pages block, then invalidating TLB entries for the TTBR0 regime.
/// Must be called immediately before EReting to that thread.
extern "C" void el0_mmu_install_thread_l3(uint32_t slot) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    if (slot >= kMaxThreadL3Slots) return;
    s_l2[s_l2_el0_block_idx] =
        reinterpret_cast<uint64_t>(s_thread_l3[slot]) | kTableDesc;
    aarch64_dsb_sy();
    tlbi_vmalle1();
#else
    (void)slot;
#endif
}

/// Restore the shared L3 (full EL0 proc page access) after all threads have
/// exited and execution returns to EL1.
extern "C" void el0_mmu_install_shared_l3() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    if (!s_shared_l3) return;
    s_l2[s_l2_el0_block_idx] =
        reinterpret_cast<uint64_t>(s_shared_l3) | kTableDesc;
    aarch64_dsb_sy();
    tlbi_vmalle1();
#endif
}

/// Returns 1 if the range [va, va+size) lies entirely within one of the six
/// EL0-mapped pages; 0 otherwise.
/// Size 0 always returns 0 (vacuously invalid — reject zero-length spans).
/// Used by the SVC dispatcher for TVA validation (RFC-00BC §5,
/// RFC-0045 segment boundary rule, RFC-00B6 §5.1 TVA validation requirement).
extern "C" int el0_tva_valid(uint64_t va, uint64_t size) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    if (size == 0 || s_code_page_pa == 0) return 0;
    const uint64_t va_end = va + size;
    if (va_end < va) return 0;  // wraparound
    if (va >= s_code_page_pa         && va_end <= s_code_page_pa         + 4096u) return 1;
    if (va >= s_stack_page_pa        && va_end <= s_stack_page_pa        + 4096u) return 1;
    if (va >= s_proc_code_page_pa    && va_end <= s_proc_code_page_pa    + 4096u) return 1;
    if (va >= s_proc_stack_page_pa   && va_end <= s_proc_stack_page_pa   + 4096u) return 1;
    if (va >= s_proc_code_page2_pa   && va_end <= s_proc_code_page2_pa   + 4096u) return 1;
    if (va >= s_proc_stack_page2_pa  && va_end <= s_proc_stack_page2_pa  + 4096u) return 1;
    return 0;
#else
    (void)va; (void)size;
    return 1;  // hosted: no MMU isolation, allow all for test linkage
#endif
}
