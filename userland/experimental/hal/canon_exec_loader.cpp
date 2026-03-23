// userland/experimental/hal/canon_exec_loader.cpp
//
// Phase 7 — CanonFS process loader (RFC-00BC §"Phase 7").
//
// Reads a T81X binary from CanonFS virtio-blk LBA 3, validates the header,
// copies the code section into the EL0-mapped process code page, flushes the
// instruction cache, and ERets to the loaded entry point at EL0 with tid=2.
//
// T81X compact binary format (512 bytes, one sector at LBA 3):
//   [0:4]   magic        "T81X"  (0x54, 0x38, 0x31, 0x58)
//   [4]     version      1       (uint8_t)
//   [5:7]   reserved0    0       (2 bytes)
//   [7:11]  entry_offset 0       (uint32_t — offset from start of code section)
//   [11:15] code_size    N       (uint32_t — byte count of code section)
//   [15:19] data_size    0       (uint32_t — unused in Phase 7)
//   [19:63] reserved     0       (44 bytes)
//   [64:N+64] code section       (raw executable bytes, max 448 bytes per sector)
//
// The code section is position-independent (el0_process_stub.S uses only
// PC-relative addressing) and is executed from s_el0_proc_code_page.
//
// ERET sequence (mirrors run_el0_init in qemu_slice6_cpp_bridge.cpp):
//   ELR_EL1  = s_proc_code_page_pa + entry_offset
//   SP_EL0   = el0_mmu_proc_stack_top()
//   SPSR_EL1 = 0x3C0  (EL0t + DAIF masked)
//   ERET → EL0 process entry
//   (SVC #2 / ExitThread patches ELR_EL1 = g_axion_el1_return_pc; ERET back here)
//
// Compilation constraints (same as other slice6 freestanding TUs):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=aarch64-pc-windows-msvc

#include <stdint.h>

// ── PL011 helpers (duplicated — same pattern as qemu_slice6_bridge_irq.cpp) ──

static constexpr uint64_t kCelPl011Base   = UINT64_C(0x09000000);
static constexpr uint32_t kCelPl011DR     = 0x000u;
static constexpr uint32_t kCelPl011FR     = 0x018u;
static constexpr uint32_t kCelPl011FRtxff = (1u << 5);

static inline void cel_mmio_write32(uint64_t base, uint32_t off, uint32_t val) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    *reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(base + off)) = val;
#else
    (void)base; (void)off; (void)val;
#endif
}

static inline uint32_t cel_mmio_read32(uint64_t base, uint32_t off) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    return *reinterpret_cast<const volatile uint32_t*>(
        static_cast<uintptr_t>(base + off));
#else
    (void)base; (void)off;
    return 0u;
#endif
}

static void cel_pl011_puts(const char* s) noexcept {
    while (*s) {
        while (cel_mmio_read32(kCelPl011Base, kCelPl011FR) & kCelPl011FRtxff) {
#if defined(__aarch64__) && !defined(__APPLE__)
            __asm__ volatile("yield" ::: "memory");
#endif
        }
        cel_mmio_write32(kCelPl011Base, kCelPl011DR,
                         static_cast<uint32_t>(static_cast<unsigned char>(*s++)));
    }
}

// ── Cross-TU symbols ──────────────────────────────────────────────────────────

// Shared EL1 resume PC — written before ERET; ExitThread (SVC #2) patches
// ELR_EL1 to this address so the ERET from EL0 lands back after our eret insn.
extern "C" uint64_t g_axion_el1_return_pc;

// Phase 7 MMU exports — proc pages mapped in TTBR0 (qemu_slice6_el0_mmu.cpp).
extern "C" uint8_t* el0_mmu_proc_code_page() noexcept;
extern "C" uint64_t el0_mmu_proc_stack_top() noexcept;

// Phase 7 tid tracker — set before ERET so GetThreadIdentity returns tid=2.
extern "C" void el0_svc_set_current_tid(uint32_t tid) noexcept;

// Wrappers in qemu_slice6_cpp_bridge.cpp that forward to the static vblk_do_io
// and s_sector_buf (which are internal to that TU).
extern "C" bool          canon_store_read_lba(uint64_t lba) noexcept;
extern "C" const uint8_t* canon_store_sector_buf()          noexcept;

// ── T81X header constants ─────────────────────────────────────────────────────

static constexpr uint8_t  kT81XMagic[4]   = { 'T', '8', '1', 'X' };
static constexpr uint8_t  kT81XVersion     = 1u;
static constexpr uint32_t kT81XHdrSize     = 64u;   // header occupies first 64 bytes
static constexpr uint32_t kT81XMaxCodeSize = 448u;  // 512 - 64 bytes header

// ── Icache flush helper ───────────────────────────────────────────────────────
// Must be called after writing new code into s_el0_proc_code_page so that
// the I-cache sees the updated instructions when EL0 executes them.

static inline void flush_icache_for_new_code() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile(
        "dsb  ish   \n\t"   // ensure all stores (data writes) reach PoC
        "ic   iallu \n\t"   // invalidate all I-cache lines to PoU (EL1)
        "dsb  ish   \n\t"   // ensure I-cache invalidation is complete
        "isb        \n\t"   // synchronise instruction stream
        ::: "memory");
#endif
}

// ── Process ERET trampoline ───────────────────────────────────────────────────
// Mirrors run_el0_init() in qemu_slice6_cpp_bridge.cpp.
// el0_fn: PA of process entry (identity-mapped proc code page + entry_offset).
// el0_sp: top of process stack page (from el0_mmu_proc_stack_top()).

static void run_proc_entry(uint64_t el0_fn, uint64_t el0_sp) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile(
        "adr     x8, 1f\n\t"
        "str     x8, [%[ret_pc]]\n\t"
        "msr     elr_el1,  %[el0_fn]\n\t"
        "msr     sp_el0,   %[el0_sp]\n\t"
        "mov     x8, #0x3c0\n\t"
        "msr     spsr_el1, x8\n\t"
        "isb\n\t"
        "eret\n\t"
        "1:\n\t"
        :
        : [ret_pc] "r"(&g_axion_el1_return_pc),
          [el0_fn] "r"(el0_fn),
          [el0_sp] "r"(el0_sp)
        : "x8", "memory");
#else
    (void)el0_fn; (void)el0_sp;
#endif
}

// ── Public entry point ────────────────────────────────────────────────────────

/// Load and execute a T81X binary from CanonFS LBA 3.
/// Called from qemu_cpp_bridge_entry() between Phase 6 and t81sh banners.
/// On success emits "[axion] el0: process loaded from CanonFS (tid=2)\r\n"
/// (the CI gate) via WriteSerial SVC #3 from the loaded process itself.
extern "C" void canon_exec_load_and_run() noexcept {
    // Read LBA 3 from the CanonFS virtio-blk device.
    if (!canon_store_read_lba(3u)) {
        cel_pl011_puts("[axion] canonfs: exec-load FAIL (LBA3 read error)\r\n");
        return;
    }

    const uint8_t* sec = canon_store_sector_buf();

    // Validate T81X magic.
    if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] ||
        sec[2] != kT81XMagic[2] || sec[3] != kT81XMagic[3]) {
        cel_pl011_puts("[axion] canonfs: exec-load FAIL (bad T81X magic)\r\n");
        return;
    }

    // Check version.
    if (sec[4] != kT81XVersion) {
        cel_pl011_puts("[axion] canonfs: exec-load FAIL (unsupported T81X version)\r\n");
        return;
    }

    // Read entry_offset and code_size from header (little-endian uint32_t).
    uint32_t entry_offset, code_size;
    __builtin_memcpy(&entry_offset, sec +  7, 4);
    __builtin_memcpy(&code_size,    sec + 11, 4);

    // Bounds check: code_size must fit in one sector after the header.
    if (code_size == 0u || code_size > kT81XMaxCodeSize) {
        cel_pl011_puts("[axion] canonfs: exec-load FAIL (T81X code_size out of range)\r\n");
        return;
    }
    if (entry_offset >= code_size) {
        cel_pl011_puts("[axion] canonfs: exec-load FAIL (T81X entry_offset >= code_size)\r\n");
        return;
    }

    // Copy code section into the EL0-mapped process code page.
    uint8_t* dest = el0_mmu_proc_code_page();
    __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);

    // Zero the remainder of the 4 KB page (defensive — avoid stale bytes).
    __builtin_memset(dest + code_size, 0,
                     static_cast<__SIZE_TYPE__>(4096u - code_size));

    // Flush I-cache so the new code is visible to the instruction stream.
    flush_icache_for_new_code();

    // Inform the SVC bridge that the next EL0 thread has tid=2.
    el0_svc_set_current_tid(2u);

    // ERET to the loaded entry point at EL0.
    // The process (el0_process_stub.S) will:
    //   1. SVC #1 (KernelCall/GetThreadIdentity) → response: caller_tid=2
    //   2. SVC #3 (WriteSerial) → "[axion] el0: process loaded from CanonFS (tid=2)\r\n"
    //   3. SVC #2 (ExitThread)  → ERET returns here
    const uint64_t proc_code_pa =
        reinterpret_cast<uint64_t>(dest);  // identity-mapped
    run_proc_entry(proc_code_pa + entry_offset, el0_mmu_proc_stack_top());

    // Restore tid=1 for any subsequent EL0 operations.
    el0_svc_set_current_tid(1u);
}

// ── Phase 8 IPC symbols (qemu_slice6_el0_svc_bridge.cpp) ─────────────────────

extern "C" bool canon_ipc_delivered() noexcept;
extern "C" void canon_ipc_reset()     noexcept;

// ── Phase 8 IPC helpers ───────────────────────────────────────────────────────
//
// Load a T81X binary from the given LBA into the proc code page and ERET
// to it at EL0 with the supplied tid.  Returns false on any load failure.

static bool load_and_run_ipc_process(uint64_t lba, uint32_t tid) noexcept {
    if (!canon_store_read_lba(lba)) {
        cel_pl011_puts("[axion] canonfs: IPC load FAIL (LBA read error)\r\n");
        return false;
    }

    const uint8_t* sec = canon_store_sector_buf();

    if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] ||
        sec[2] != kT81XMagic[2] || sec[3] != kT81XMagic[3]) {
        cel_pl011_puts("[axion] canonfs: IPC load FAIL (bad T81X magic)\r\n");
        return false;
    }

    if (sec[4] != kT81XVersion) {
        cel_pl011_puts("[axion] canonfs: IPC load FAIL (unsupported T81X version)\r\n");
        return false;
    }

    uint32_t entry_offset, code_size;
    __builtin_memcpy(&entry_offset, sec +  7, 4);
    __builtin_memcpy(&code_size,    sec + 11, 4);

    if (code_size == 0u || code_size > kT81XMaxCodeSize) {
        cel_pl011_puts("[axion] canonfs: IPC load FAIL (T81X code_size out of range)\r\n");
        return false;
    }
    if (entry_offset >= code_size) {
        cel_pl011_puts("[axion] canonfs: IPC load FAIL (T81X entry_offset >= code_size)\r\n");
        return false;
    }

    uint8_t* dest = el0_mmu_proc_code_page();
    __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);
    __builtin_memset(dest + code_size, 0,
                     static_cast<__SIZE_TYPE__>(4096u - code_size));

    flush_icache_for_new_code();
    el0_svc_set_current_tid(tid);

    const uint64_t proc_code_pa = reinterpret_cast<uint64_t>(dest);
    run_proc_entry(proc_code_pa + entry_offset, el0_mmu_proc_stack_top());

    el0_svc_set_current_tid(1u);
    return true;
}

// ── Public entry point (Phase 8) ─────────────────────────────────────────────

/// Run the Phase 8 IPC roundtrip: load Process A (LBA 4, tid=2) then
/// Process B (LBA 5, tid=3); confirm the message was delivered; emit the
/// Phase 8 CI gate banner "[axion] el0: IPC roundtrip OK (A->B, tid=2,3)".
extern "C" void canon_ipc_load_and_run() noexcept {
    canon_ipc_reset();

    // Load and run Process A (SendMessage → ExitThread).
    if (!load_and_run_ipc_process(4u, 2u)) return;

    // Load and run Process B (ReceiveMessage → ExitThread).
    if (!load_and_run_ipc_process(5u, 3u)) return;

    // Confirm delivery and emit the CI gate.
    if (canon_ipc_delivered()) {
        cel_pl011_puts("[axion] el0: IPC roundtrip OK (A->B, tid=2,3)\r\n");
    } else {
        cel_pl011_puts("[axion] el0: IPC roundtrip FAIL (message not delivered)\r\n");
    }
}
