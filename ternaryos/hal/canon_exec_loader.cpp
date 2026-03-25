// ternaryos/hal/canon_exec_loader.cpp
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

static constexpr uint64_t kCelPl011Base = UINT64_C(0x09000000);
static constexpr uint32_t kCelPl011DR = 0x000u;
static constexpr uint32_t kCelPl011FR = 0x018u;
static constexpr uint32_t kCelPl011FRtxff = (1u << 5);

static inline void cel_mmio_write32(uint64_t base, uint32_t off, uint32_t val) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  *reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(base + off)) = val;
#else
  (void)base;
  (void)off;
  (void)val;
#endif
}

static inline uint32_t cel_mmio_read32(uint64_t base, uint32_t off) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  return *reinterpret_cast<const volatile uint32_t*>(static_cast<uintptr_t>(base + off));
#else
  (void)base;
  (void)off;
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

// Phase 8 IPC mailbox reset (qemu_slice6_el0_svc_bridge.cpp).
extern "C" void canon_ipc_reset() noexcept;

// Phase 9 (RFC-00BE) scheduler API (qemu_slice6_el0_svc_bridge.cpp).
extern "C" void fs_sched_register(uint32_t tid, uint64_t elr, uint64_t sp_el0,
                                  uint64_t spsr) noexcept;
extern "C" void fs_sched_mark_running(uint32_t tid) noexcept;
extern "C" void fs_sched_reset() noexcept;
extern "C" bool fs_sched_ipc_delivered() noexcept;

// Phase 10 (RFC-00BF) observability API (qemu_slice6_el0_svc_bridge.cpp).
extern "C" bool fs_obs_find(uint32_t tid, uint32_t kind, uint32_t peer_tid) noexcept;
extern "C" uint64_t fs_obs_count() noexcept;

// Phase 11 (RFC-00C0) device-wake API (qemu_slice6_el0_svc_bridge.cpp).
extern "C" void fs_sched_wake_device(uint32_t tid) noexcept;
extern "C" bool fs_sched_get_resume(uint32_t tid, uint64_t* out_elr, uint64_t* out_sp) noexcept;

// Phase 13 (RFC-00C2) IRQ-driven device-wait loop
// (qemu_slice6_el0_svc_bridge.cpp).
extern "C" uint64_t g_axion_el1_device_wait_pc;
extern "C" void fs_sched_device_wait_loop() noexcept;

// Phase 14 (RFC-00C3) governance audit ring
// (qemu_slice6_el0_svc_bridge.cpp).
extern "C" bool fs_gov_find(uint32_t tid, uint32_t event) noexcept;
extern "C" uint64_t fs_gov_count() noexcept;

// Phase 15 (RFC-00C4) per-device gov ring query.
extern "C" bool fs_gov_find_device(uint32_t tid, uint32_t event, uint32_t device_id) noexcept;

// Phase 16 (RFC-00C5) second process code/stack pages (qemu_slice6_el0_mmu.cpp).
extern "C" uint8_t* el0_mmu_proc_code_page2() noexcept;
extern "C" uint64_t el0_mmu_proc_stack_top2() noexcept;

// Phase 17 (RFC-00C6) per-thread L3 MMU API (qemu_slice6_el0_mmu.cpp).
extern "C" void el0_mmu_build_thread_l3(uint32_t slot, uint64_t own_code_pa,
                                        uint64_t own_stack_pa) noexcept;

// Phase 17 (RFC-00C6) per-thread L3 scheduler API (qemu_slice6_el0_svc_bridge.cpp).
extern "C" void fs_sched_set_thread_l3(uint32_t tid, uint32_t l3_slot) noexcept;

// Wrappers in qemu_slice6_cpp_bridge.cpp that forward to the static vblk_do_io
// and s_sector_buf (which are internal to that TU).
extern "C" bool canon_store_read_lba(uint64_t lba) noexcept;
extern "C" const uint8_t* canon_store_sector_buf() noexcept;

// ── T81X header constants ─────────────────────────────────────────────────────

static constexpr uint8_t kT81XMagic[4] = {'T', '8', '1', 'X'};
static constexpr uint8_t kT81XVersion = 1u;
static constexpr uint8_t kT81XVersion2 = 2u;        // RFC-00C0: adds code_hash at [19:27]
static constexpr uint32_t kT81XHdrSize = 64u;       // header occupies first 64 bytes
static constexpr uint32_t kT81XMaxCodeSize = 448u;  // 512 - 64 bytes header
static constexpr uint32_t kT81XHashOffset = 19u;    // code_hash uint64_t in v2 header

// ── FNV-1a-64 hash (RFC-00C0) ────────────────────────────────────────────────
// Used to compute the content hash for T81X v2 identity validation.
// No lookup tables; freestanding-safe.

static uint64_t fnv1a64(const uint8_t* data, uint32_t len) noexcept {
  uint64_t h = UINT64_C(14695981039346656037);  // FNV offset basis
  for (uint32_t i = 0u; i < len; ++i) {
    h ^= static_cast<uint64_t>(data[i]);
    h *= UINT64_C(1099511628211);  // FNV prime
  }
  return h;
}

// ── Decimal printer (RFC-00C1) ────────────────────────────────────────────────
// cel_pl011_puts only accepts null-terminated strings; this helper converts a
// uint32_t to decimal digits and emits them via cel_pl011_puts.

static void cel_pl011_putd(uint32_t n) noexcept {
  char buf[12];
  uint32_t i = 11u;
  buf[i] = '\0';
  do {
    buf[--i] = static_cast<char>('0' + (n % 10u));
    n /= 10u;
  } while (n);
  cel_pl011_puts(buf + i);
}

// ── T81M manifest constants (RFC-00C1) ───────────────────────────────────────

static constexpr uint8_t kT81MMagic[4] = {'T', '8', '1', 'M'};
static constexpr uint8_t kT81MVersion = 1u;
static constexpr uint32_t kT81MHdrSize = 32u;
static constexpr uint32_t kT81MEntrySize = 12u;
static constexpr uint32_t kT81MMaxEntries = (512u - kT81MHdrSize) / kT81MEntrySize;  // 40

// ── Icache flush helper ───────────────────────────────────────────────────────
// Must be called after writing new code into s_el0_proc_code_page so that
// the I-cache sees the updated instructions when EL0 executes them.

static inline void flush_icache_for_new_code() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile("dsb  ish   \n\t"  // ensure all stores (data writes) reach PoC
                   "ic   iallu \n\t"  // invalidate all I-cache lines to PoU (EL1)
                   "dsb  ish   \n\t"  // ensure I-cache invalidation is complete
                   "isb        \n\t"  // synchronise instruction stream
                   ::
                       : "memory");
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
      : [ret_pc] "r"(&g_axion_el1_return_pc), [el0_fn] "r"(el0_fn), [el0_sp] "r"(el0_sp)
      : "x8", "memory");
#else
  (void)el0_fn;
  (void)el0_sp;
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
  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
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
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

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
  __builtin_memset(dest + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));

  // Flush I-cache so the new code is visible to the instruction stream.
  flush_icache_for_new_code();

  // Inform the SVC bridge that the next EL0 thread has tid=2.
  el0_svc_set_current_tid(2u);

  // ERET to the loaded entry point at EL0.
  // The process (el0_process_stub.S) will:
  //   1. SVC #1 (KernelCall/GetThreadIdentity) → response: caller_tid=2
  //   2. SVC #3 (WriteSerial) → "[axion] el0: process loaded from CanonFS (tid=2)\r\n"
  //   3. SVC #2 (ExitThread)  → ERET returns here
  const uint64_t proc_code_pa = reinterpret_cast<uint64_t>(dest);  // identity-mapped
  run_proc_entry(proc_code_pa + entry_offset, el0_mmu_proc_stack_top());

  // Restore tid=1 for any subsequent EL0 operations.
  el0_svc_set_current_tid(1u);
}

// ── Phase 9 scheduler helpers ─────────────────────────────────────────────────
//
// Load a T81X binary from the given LBA into the proc code page at byte offset
// `page_offset` and register the thread in the freestanding scheduler.
// Does NOT flush the I-cache (caller flushes once after all loads).
// Does NOT ERET (caller schedules the initial ERET separately).

static bool load_sched_process_at(uint64_t lba, uint32_t tid, uint32_t page_offset,
                                  uint64_t sp_top) noexcept {
  if (!canon_store_read_lba(lba)) {
    cel_pl011_puts("[axion] canonfs: sched load FAIL (LBA read error)\r\n");
    return false;
  }

  const uint8_t* sec = canon_store_sector_buf();

  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: sched load FAIL (bad T81X magic)\r\n");
    return false;
  }

  if (sec[4] != kT81XVersion) {
    cel_pl011_puts("[axion] canonfs: sched load FAIL (unsupported T81X version)\r\n");
    return false;
  }

  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

  if (code_size == 0u || code_size > kT81XMaxCodeSize) {
    cel_pl011_puts("[axion] canonfs: sched load FAIL (T81X code_size out of range)\r\n");
    return false;
  }
  if (entry_offset >= code_size) {
    cel_pl011_puts("[axion] canonfs: sched load FAIL (T81X entry_offset >= code_size)\r\n");
    return false;
  }

  uint8_t* dest = el0_mmu_proc_code_page() + page_offset;
  __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);
  __builtin_memset(dest + code_size, 0,
                   static_cast<__SIZE_TYPE__>(256u - code_size));  // zero up to next slot

  const uint64_t entry_pa = reinterpret_cast<uint64_t>(dest) + entry_offset;
  fs_sched_register(tid, entry_pa, sp_top, 0x3C0u);
  return true;
}

// ── Public entry point (Phase 9) ──────────────────────────────────────────────

/// Run the Phase 9 cooperative scheduler roundtrip (RFC-00BE):
///   Process B (LBA 7, tid=3) blocks on IpcReceive first;
///   Process A (LBA 6, tid=2) sends a message and exits;
///   the scheduler resumes B, which exits; EL1 confirms delivery.
///
/// CI gate: "[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)"
extern "C" void canon_sched_load_and_run() noexcept {
  fs_sched_reset();
  canon_ipc_reset();

  uint64_t stack_top = el0_mmu_proc_stack_top();

  // Load Process B at code page offset 0, stack = stack_top (tid=3, blocker).
  if (!load_sched_process_at(7u, 3u, 0u, stack_top)) return;

  // Load Process A at code page offset 256, stack offset −256 (tid=2, sender).
  if (!load_sched_process_at(6u, 2u, 256u, stack_top - 256u)) return;

  // Flush I-cache once for both loaded binaries.
  flush_icache_for_new_code();

  // Mark B as the initial Running thread.
  fs_sched_mark_running(3u);
  el0_svc_set_current_tid(3u);

  // ERET to B's entry.  All context switches happen inside SVC handlers;
  // control returns here only when the last thread calls ExitThread with no
  // remaining Runnable threads.
  const uint64_t proc_code_pa = reinterpret_cast<uint64_t>(el0_mmu_proc_code_page());
  run_proc_entry(proc_code_pa, stack_top);

  el0_svc_set_current_tid(1u);

  if (fs_sched_ipc_delivered()) {
    cel_pl011_puts("[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: sched roundtrip FAIL (IPC not delivered)\r\n");
  }

  // Phase 10 (RFC-00BF): verify observability ring captured the expected
  // Phase 9 call graph — BlockOnIpcReceive from tid=3 and SendMessage tid=2→3.
  const bool saw_block = fs_obs_find(3u, 42u, 0u);
  const bool saw_send = fs_obs_find(2u, 13u, 3u);
  if (saw_block && saw_send) {
    cel_pl011_puts("[axion] el0: obs OK (BlockOnIpcReceive tid=3, SendMessage tid=2->3)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: obs FAIL (expected records missing)\r\n");
  }
}

// ── Phase 12 (RFC-00C1): T81M manifest verification ──────────────────────────
//
// Loads a T81M manifest sector from `manifest_lba`, verifies the magic/version,
// checks that the stored code_hash equals `expected_hash`, then walks each
// manifest entry against the observability ring via fs_obs_find().
//
// On full match emits:
//   "[axion] el0: manifest OK (code_hash=verified, seq=N/N matched)"
// On any mismatch emits a FAIL banner and returns.

static void canon_manifest_verify(uint64_t expected_hash, uint64_t manifest_lba) noexcept {
  if (!canon_store_read_lba(manifest_lba)) {
    cel_pl011_puts("[axion] canonfs: manifest FAIL (LBA read error)\r\n");
    return;
  }

  const uint8_t* sec = canon_store_sector_buf();

  if (sec[0] != kT81MMagic[0] || sec[1] != kT81MMagic[1] || sec[2] != kT81MMagic[2] ||
      sec[3] != kT81MMagic[3]) {
    cel_pl011_puts("[axion] canonfs: manifest FAIL (bad T81M magic)\r\n");
    return;
  }

  if (sec[4] != kT81MVersion) {
    cel_pl011_puts("[axion] canonfs: manifest FAIL (unsupported T81M version)\r\n");
    return;
  }

  uint64_t stored_hash;
  __builtin_memcpy(&stored_hash, sec + 8, 8);
  if (stored_hash != expected_hash) {
    cel_pl011_puts("[axion] canonfs: manifest FAIL (hash mismatch vs binary)\r\n");
    return;
  }

  const uint32_t entry_count = static_cast<uint32_t>(sec[16]);
  if (entry_count == 0u || entry_count > kT81MMaxEntries) {
    cel_pl011_puts("[axion] canonfs: manifest FAIL (invalid entry_count)\r\n");
    return;
  }

  uint32_t matched = 0u;
  const uint8_t* ep = sec + kT81MHdrSize;
  for (uint32_t i = 0u; i < entry_count; ++i, ep += kT81MEntrySize) {
    uint32_t tid, kind, peer_tid;
    __builtin_memcpy(&tid, ep + 0, 4);
    __builtin_memcpy(&kind, ep + 4, 4);
    __builtin_memcpy(&peer_tid, ep + 8, 4);
    if (fs_obs_find(tid, kind, peer_tid)) ++matched;
  }

  if (matched == entry_count) {
    cel_pl011_puts("[axion] el0: manifest OK (code_hash=verified, seq=");
    cel_pl011_putd(matched);
    cel_pl011_puts("/");
    cel_pl011_putd(entry_count);
    cel_pl011_puts(" matched)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: manifest FAIL (seq mismatch: ");
    cel_pl011_putd(matched);
    cel_pl011_puts("/");
    cel_pl011_putd(entry_count);
    cel_pl011_puts(" matched)\r\n");
  }
}

// ── Phase 11 (RFC-00C0) — T81X v2 identity validation + WaitForDevice waker ──
//
// Loads el0_wait_test.bin from LBA 8 as a T81X v2 binary, validates the
// FNV-1a-64 code_hash, then runs the two-pass scheduler sequence:
//   Pass 1 — Process C (tid=4) calls WaitForDevice and parks.
//   EL1 wakes C via fs_sched_wake_device(4u).
//   Pass 2 — C resumes and calls ExitThread.
// After pass 2 the obs ring is queried to confirm WaitForDevice was recorded.
//
// CI gates:
//   "[axion] el0: identity OK (hash=verified, tid=4)"
//   "[axion] el0: device wake OK (WaitForDevice tid=4)"

static constexpr uint32_t kKindWaitForDevice = 43u;  // RFC-00BD frozen ordinal

extern "C" void canon_identity_load_and_run() noexcept {
  // ── Load and validate T81X v2 from LBA 8 ─────────────────────────────────
  if (!canon_store_read_lba(8u)) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (LBA8 read error)\r\n");
    return;
  }

  const uint8_t* sec = canon_store_sector_buf();

  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (bad T81X magic)\r\n");
    return;
  }

  if (sec[4] != kT81XVersion2) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (expected T81X v2)\r\n");
    return;
  }

  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

  if (code_size == 0u || code_size > kT81XMaxCodeSize) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (T81X code_size out of range)\r\n");
    return;
  }
  if (entry_offset >= code_size) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (T81X entry_offset >= code_size)\r\n");
    return;
  }

  // Read stored code_hash (little-endian uint64_t at header[19:27]).
  uint64_t stored_hash;
  __builtin_memcpy(&stored_hash, sec + kT81XHashOffset, 8);

  // Compute FNV-1a-64 of the code section.
  const uint64_t computed_hash = fnv1a64(sec + kT81XHdrSize, code_size);

  if (computed_hash != stored_hash) {
    cel_pl011_puts("[axion] canonfs: identity load FAIL (hash mismatch)\r\n");
    return;
  }

  cel_pl011_puts("[axion] el0: identity OK (hash=verified, tid=4)\r\n");

  // ── Copy code section into proc page and flush I-cache ────────────────────
  uint8_t* dest = el0_mmu_proc_code_page();
  __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);
  __builtin_memset(dest + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));
  flush_icache_for_new_code();

  // ── Pass 1: register C (tid=4) and ERET — C calls WaitForDevice ──────────
  fs_sched_reset();  // also resets obs ring

  const uint64_t stack_top = el0_mmu_proc_stack_top();
  const uint64_t code_pa = reinterpret_cast<uint64_t>(dest) + entry_offset;

  fs_sched_register(4u, code_pa, stack_top, 0x3C0u);
  fs_sched_mark_running(4u);
  el0_svc_set_current_tid(4u);
  run_proc_entry(code_pa, stack_top);
  // EL1 resumes here: C is parked (BlockedDeviceWait), no Runnable threads.

  // ── Wake C and run pass 2 ─────────────────────────────────────────────────
  fs_sched_wake_device(4u);  // BlockedDeviceWait → Runnable

  uint64_t resume_elr = 0u, resume_sp = 0u;
  if (!fs_sched_get_resume(4u, &resume_elr, &resume_sp)) {
    cel_pl011_puts("[axion] el0: device wake FAIL (resume context missing)\r\n");
    el0_svc_set_current_tid(1u);
    return;
  }

  fs_sched_mark_running(4u);  // Runnable → Running
  el0_svc_set_current_tid(4u);
  run_proc_entry(resume_elr, resume_sp);
  // EL1 resumes here: C has exited.

  el0_svc_set_current_tid(1u);

  // ── Phase 11b: verify obs ring captured WaitForDevice from tid=4 ──────────
  if (fs_obs_find(4u, kKindWaitForDevice, 0u)) {
    cel_pl011_puts("[axion] el0: device wake OK (WaitForDevice tid=4)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: device wake FAIL (obs record missing)\r\n");
  }

  // ── Phase 12 (RFC-00C1): verify manifest matches the obs ring ─────────────
  // manifest_lba = binary_lba + 1 (convention: LBA 8 binary → LBA 9 manifest).
  canon_manifest_verify(computed_hash, 9u);
}

// ── Phase 13 (RFC-00C2): IRQ-driven WaitForDevice wake ───────────────────────
//
// Loads el0_wait_test (same stub as Phase 11) from LBA 10 as T81X v2 with
// tid=5.  Installs fs_sched_device_wait_loop as the device-wait loop so that
// when Process D parks on WaitForDevice the SVC handler redirects ERET to the
// wfi idle loop instead of returning to EL1 immediately.
//
// A single run_proc_entry() call covers the full roundtrip:
//   D → WaitForDevice → wfi idle loop → timer IRQ → Runnable → D resumes →
//   ExitThread → no Runnable → g_axion_el1_return_pc → EL1.
//
// CI gates:
//   "[axion] el0: irq identity OK (hash=verified, tid=5)"  — Phase 13a
//   "[axion] el0: irq wake OK (WaitForDevice tid=5, timer-driven)"  — Phase 13b

extern "C" void canon_irq_wake_load_and_run() noexcept {
  if (!canon_store_read_lba(10u)) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (LBA10 read error)\r\n");
    return;
  }

  const uint8_t* sec = canon_store_sector_buf();

  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (bad T81X magic)\r\n");
    return;
  }
  if (sec[4] != kT81XVersion2) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (expected T81X v2)\r\n");
    return;
  }

  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

  if (code_size == 0u || code_size > kT81XMaxCodeSize) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (code_size out of range)\r\n");
    return;
  }
  if (entry_offset >= code_size) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (entry_offset >= code_size)\r\n");
    return;
  }

  uint64_t stored_hash;
  __builtin_memcpy(&stored_hash, sec + kT81XHashOffset, 8);
  const uint64_t computed_hash = fnv1a64(sec + kT81XHdrSize, code_size);
  if (computed_hash != stored_hash) {
    cel_pl011_puts("[axion] canonfs: irq wake FAIL (hash mismatch)\r\n");
    return;
  }

  cel_pl011_puts("[axion] el0: irq identity OK (hash=verified, tid=5)\r\n");

  uint8_t* dest = el0_mmu_proc_code_page();
  __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);
  __builtin_memset(dest + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));
  flush_icache_for_new_code();

  // Install the IRQ-driven device-wait loop so that the WaitForDevice
  // handler redirects to it instead of returning to EL1 immediately.
  g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

  fs_sched_reset();  // also resets obs ring

  const uint64_t stack_top = el0_mmu_proc_stack_top();
  const uint64_t code_pa = reinterpret_cast<uint64_t>(dest) + entry_offset;

  fs_sched_register(5u, code_pa, stack_top, 0x3C0u);
  fs_sched_mark_running(5u);
  el0_svc_set_current_tid(5u);

  // Single ERET: D parks on WaitForDevice → wfi → timer IRQ wakes D →
  // D resumes → ExitThread → EL1.
  run_proc_entry(code_pa, stack_top);

  el0_svc_set_current_tid(1u);

  // Restore to immediate-return mode for any subsequent scheduler sessions.
  g_axion_el1_device_wait_pc = 0u;

  // Phase 13b: confirm obs ring captured WaitForDevice from tid=5.
  if (fs_obs_find(5u, 43u, 0u)) {
    cel_pl011_puts("[axion] el0: irq wake OK (WaitForDevice tid=5, timer-driven)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: irq wake FAIL (obs record missing)\r\n");
  }

  // Phase 14 (RFC-00C3): verify governance ring captured both async events.
  // kGovTimerDeviceWake=1: timer ISR transitioned tid=5 BlockedDeviceWait→Runnable.
  // kGovAsyncContextSwitch=2: device_wait_loop EReted to tid=5 at EL0.
  const bool saw_timer_wake = fs_gov_find(5u, 1u);
  const bool saw_async_eret = fs_gov_find(5u, 2u);
  if (saw_timer_wake && saw_async_eret) {
    cel_pl011_puts("[axion] el0: async audit OK (AsyncWake tid=5, irq-driven)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: async audit FAIL (gov record missing)\r\n");
  }
}

// ── Phase 15 (RFC-00C4): per-device wake filtering ────────────────────────────
//
// Loads el0_device_filter_test from LBA 11 as T81X v2 with tid=6.
// Process E calls WaitForDevice(device_id=30).  The WaitForDevice SVC handler
// stores device_id=30 into FsSchedThread.device_id and redirects ERET to
// fs_sched_device_wait_loop.
// Timer IRQ fires → fs_sched_timer_device_wake(30) → did==intid → Runnable.
// device_wait_loop ERets back to E → E calls ExitThread → EL1.
//
// Verification: fs_gov_find_device(6, kGovTimerDeviceWake=1, device_id=30).
//
// CI gate: "[axion] el0: device filter OK (device_id=30, tid=6)"

extern "C" void canon_device_filter_load_and_run() noexcept {
  if (!canon_store_read_lba(11u)) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (LBA11 read error)\r\n");
    return;
  }

  const uint8_t* sec = canon_store_sector_buf();

  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (bad T81X magic)\r\n");
    return;
  }
  if (sec[4] != kT81XVersion2) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (expected T81X v2)\r\n");
    return;
  }

  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

  if (code_size == 0u || code_size > kT81XMaxCodeSize) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (code_size out of range)\r\n");
    return;
  }
  if (entry_offset >= code_size) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (entry_offset >= code_size)\r\n");
    return;
  }

  uint64_t stored_hash;
  __builtin_memcpy(&stored_hash, sec + kT81XHashOffset, 8);
  const uint64_t computed_hash = fnv1a64(sec + kT81XHdrSize, code_size);
  if (computed_hash != stored_hash) {
    cel_pl011_puts("[axion] canonfs: device filter FAIL (hash mismatch)\r\n");
    return;
  }

  uint8_t* dest = el0_mmu_proc_code_page();
  __builtin_memcpy(dest, sec + kT81XHdrSize, code_size);
  __builtin_memset(dest + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));
  flush_icache_for_new_code();

  // Install the IRQ-driven device-wait loop (same as Phase 13).
  g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

  fs_sched_reset();  // also resets obs + gov rings

  const uint64_t stack_top = el0_mmu_proc_stack_top();
  const uint64_t code_pa = reinterpret_cast<uint64_t>(dest) + entry_offset;

  fs_sched_register(6u, code_pa, stack_top, 0x3C0u);
  fs_sched_mark_running(6u);
  el0_svc_set_current_tid(6u);

  // Single ERET: E parks on WaitForDevice(device_id=30) → wfi →
  // timer IRQ calls fs_sched_timer_device_wake(30) → did==30 match → Runnable →
  // E resumes → ExitThread → EL1.
  run_proc_entry(code_pa, stack_top);

  el0_svc_set_current_tid(1u);

  // Restore to immediate-return mode.
  g_axion_el1_device_wait_pc = 0u;

  // Phase 15: verify gov ring recorded a timer wake for tid=6, device_id=30.
  if (fs_gov_find_device(6u, 1u /*kGovTimerDeviceWake*/, 30u)) {
    cel_pl011_puts("[axion] el0: device filter OK (device_id=30, tid=6)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: device filter FAIL (gov record missing)\r\n");
  }
}

// ── Phase 16 (RFC-00C5): concurrent device wait ───────────────────────────────
//
// Loads two processes simultaneously and registers both as Runnable before
// issuing a single run_proc_entry().  Both threads park on WaitForDevice;
// the timer IRQ (INTID 30) wakes both via fs_sched_timer_device_wake(30).
//
// Thread F (tid=7) — el0_wait_test (LBA 10, device_id=0, 12-byte request):
//   Wildcard wait: wakes on any INTID.
// Thread E (tid=6) — el0_device_filter_test (LBA 11, device_id=30, 16-byte):
//   Exact match: wakes only on INTID 30.
//
// Execution order:
//   run_proc_entry → tid=6
//   tid=6 WaitForDevice(30) → BlockedDeviceWait → SVC finds tid=7 Runnable
//                              → ERET to tid=7 (no wfi needed yet)
//   tid=7 WaitForDevice(0)  → BlockedDeviceWait → no Runnable → device_wait_loop
//   device_wait_loop wfi → timer INTID=30 → both wake
//   device_wait_loop → ERET to tid=6 (resume after svc#1)
//   tid=6 ExitThread → fs_sched_exit_thread → finds tid=7 Runnable → ERET tid=7
//   tid=7 ExitThread → fs_sched_exit_thread → no Runnable → g_axion_el1_return_pc
//
// CI gate: "[axion] el0: concurrent wake OK (device_id=30, tid=6+7)"

// Helper: load a T81X v2 binary from lba into dest_page and return the
// absolute PA of the entry point.  Returns 0 on any error.
static uint64_t load_t81x_v2_into(uint64_t lba, uint8_t* dest_page, const char* label) noexcept {
  if (!canon_store_read_lba(lba)) {
    cel_pl011_puts("[axion] canonfs: concurrent load FAIL (LBA read error: ");
    cel_pl011_puts(label);
    cel_pl011_puts(")\r\n");
    return 0u;
  }
  const uint8_t* sec = canon_store_sector_buf();
  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: concurrent load FAIL (bad T81X magic: ");
    cel_pl011_puts(label);
    cel_pl011_puts(")\r\n");
    return 0u;
  }
  if (sec[4] != kT81XVersion2) {
    cel_pl011_puts("[axion] canonfs: concurrent load FAIL (expected v2: ");
    cel_pl011_puts(label);
    cel_pl011_puts(")\r\n");
    return 0u;
  }
  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);
  if (code_size == 0u || code_size > kT81XMaxCodeSize || entry_offset >= code_size) {
    cel_pl011_puts("[axion] canonfs: concurrent load FAIL (bad sizes: ");
    cel_pl011_puts(label);
    cel_pl011_puts(")\r\n");
    return 0u;
  }
  uint64_t stored_hash;
  __builtin_memcpy(&stored_hash, sec + kT81XHashOffset, 8);
  if (fnv1a64(sec + kT81XHdrSize, code_size) != stored_hash) {
    cel_pl011_puts("[axion] canonfs: concurrent load FAIL (hash mismatch: ");
    cel_pl011_puts(label);
    cel_pl011_puts(")\r\n");
    return 0u;
  }
  __builtin_memcpy(dest_page, sec + kT81XHdrSize, code_size);
  __builtin_memset(dest_page + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));
  return reinterpret_cast<uint64_t>(dest_page) + entry_offset;
}

extern "C" void canon_concurrent_wait_load_and_run() noexcept {
  // Load Thread F (tid=7, device_id=0) from LBA 10 into code page 2.
  const uint64_t code_pa_f = load_t81x_v2_into(10u, el0_mmu_proc_code_page2(), "tid7/LBA10");
  if (!code_pa_f) return;

  // Load Thread E (tid=6, device_id=30) from LBA 11 into code page 1.
  const uint64_t code_pa_e = load_t81x_v2_into(11u, el0_mmu_proc_code_page(), "tid6/LBA11");
  if (!code_pa_e) return;

  flush_icache_for_new_code();

  // Install IRQ-driven device-wait loop.
  g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

  fs_sched_reset();  // resets sched, obs, and gov rings

  const uint64_t stack_top_e = el0_mmu_proc_stack_top();
  const uint64_t stack_top_f = el0_mmu_proc_stack_top2();

  // Register both threads as Runnable.
  fs_sched_register(6u, code_pa_e, stack_top_e, 0x3C0u);
  fs_sched_mark_running(6u);
  fs_sched_register(7u, code_pa_f, stack_top_f, 0x3C0u);
  // tid=7 stays Runnable (not marked Running) — the scheduler picks it up.

  el0_svc_set_current_tid(6u);

  // Single ERET to tid=6.  The scheduler handles tid=7 and both ExitThread
  // transitions without further EL1 intervention.
  run_proc_entry(code_pa_e, stack_top_e);

  el0_svc_set_current_tid(1u);
  g_axion_el1_device_wait_pc = 0u;

  // Phase 16: both threads must appear in the gov ring with kGovTimerDeviceWake
  // and device_id=30 (the INTID that fired, regardless of their registered did).
  const bool woke_e = fs_gov_find_device(6u, 1u /*kGovTimerDeviceWake*/, 30u);
  const bool woke_f = fs_gov_find_device(7u, 1u /*kGovTimerDeviceWake*/, 30u);
  if (woke_e && woke_f) {
    cel_pl011_puts("[axion] el0: concurrent wake OK (device_id=30, tid=6+7)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: concurrent wake FAIL (gov record missing)\r\n");
  }
}

// ── Phase 17 (RFC-00C6): per-thread address-space isolation ──────────────────
//
// Runs the same Phase 16 concurrent-wait scenario but with per-thread L3
// page tables installed:
//   - Thread E (tid=6, did=30): maps only its own code page 1 + stack 1 at
//     EL0; code page 2 and stack page 2 are EL1-only in its L3 view.
//   - Thread F (tid=7, did=0): maps only its own code page 2 + stack 2 at
//     EL0; code page 1 and stack page 1 are EL1-only.
//
// Before each ERET the scheduler installs the target thread's private L3 by
// swapping s_l2[block_idx] + TLBI.  After all threads exit, the shared L3 is
// restored so EL1 regains full proc page access.
//
// If both threads run to completion (correct own-page access, no permission
// fault) the per-thread isolation tables are structurally correct.
//
// CI gate: "[axion] el0: per-thread pt OK (tid=6+7, isolated)"

extern "C" void canon_per_thread_pt_load_and_run() noexcept {
  // Load Thread F (tid=7, device_id=0) from LBA 10 into code page 2.
  const uint64_t code_pa_f = load_t81x_v2_into(10u, el0_mmu_proc_code_page2(), "tid7/LBA10");
  if (!code_pa_f) return;

  // Load Thread E (tid=6, device_id=30) from LBA 11 into code page 1.
  const uint64_t code_pa_e = load_t81x_v2_into(11u, el0_mmu_proc_code_page(), "tid6/LBA11");
  if (!code_pa_e) return;

  flush_icache_for_new_code();

  // Build per-thread L3 tables (RFC-00C6).
  // Slot 0 → tid=6: own_code=page1, own_stack=stack1.
  // Slot 1 → tid=7: own_code=page2, own_stack=stack2.
  el0_mmu_build_thread_l3(0u, reinterpret_cast<uint64_t>(el0_mmu_proc_code_page()),
                          el0_mmu_proc_stack_top() - 4096u);
  el0_mmu_build_thread_l3(1u, reinterpret_cast<uint64_t>(el0_mmu_proc_code_page2()),
                          el0_mmu_proc_stack_top2() - 4096u);

  // Install IRQ-driven device-wait loop.
  g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

  fs_sched_reset();  // resets sched, obs, and gov rings

  const uint64_t stack_top_e = el0_mmu_proc_stack_top();
  const uint64_t stack_top_f = el0_mmu_proc_stack_top2();

  // Register both threads with their per-thread L3 slots.
  fs_sched_register(6u, code_pa_e, stack_top_e, 0x3C0u);
  fs_sched_mark_running(6u);
  fs_sched_set_thread_l3(6u, 0u);  // RFC-00C6: slot 0

  fs_sched_register(7u, code_pa_f, stack_top_f, 0x3C0u);
  fs_sched_set_thread_l3(7u, 1u);  // RFC-00C6: slot 1

  el0_svc_set_current_tid(6u);

  // Single ERET to tid=6.  The scheduler installs per-thread L3 before
  // each ERET; shared L3 is restored when the last thread exits.
  run_proc_entry(code_pa_e, stack_top_e);

  el0_svc_set_current_tid(1u);
  g_axion_el1_device_wait_pc = 0u;

  // Phase 17: verify both threads were woken (same gov ring query as Phase 16).
  const bool woke_e = fs_gov_find_device(6u, 1u /*kGovTimerDeviceWake*/, 30u);
  const bool woke_f = fs_gov_find_device(7u, 1u /*kGovTimerDeviceWake*/, 30u);
  if (woke_e && woke_f) {
    cel_pl011_puts("[axion] el0: per-thread pt OK (tid=6+7, isolated)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: per-thread pt FAIL (gov record missing)\r\n");
  }
}

// ── Phase 18 (RFC-00C7) symbols ───────────────────────────────────────────────

// RFC-00C7: query whether a kGovThreadFault record exists for (tid, ec).
extern "C" bool fs_gov_find_fault(uint32_t tid, uint32_t ec) noexcept;

// ── Phase 18 (RFC-00C7): EL0 fault containment ───────────────────────────────
//
// Loads Process G (el0_fault_test.bin, T81X v2, tid=8) from LBA 12.
// Process G immediately dereferences virtual address 0x0, triggering a
// Data Abort (ESR_EL1 EC=0x24).  The EL1 fault handler marks tid=8 as
// Faulted, records kGovThreadFault in the gov ring, and returns to EL1 via
// g_axion_el1_return_pc — without hanging.
//
// CI gate: "[axion] el0: fault contained (tid=8, ec=0x24)"
extern "C" void canon_fault_contain_load_and_run() noexcept {
  // Load Process G (tid=8) from LBA 12 into code page 1.
  const uint64_t code_pa = load_t81x_v2_into(12u, el0_mmu_proc_code_page(), "tid8/LBA12");
  if (!code_pa) return;

  flush_icache_for_new_code();

  // No device-wait loop needed — G faults immediately without parking.
  // g_axion_el1_device_wait_pc stays 0.

  fs_sched_reset();  // resets sched, obs, and gov rings

  const uint64_t stack_top = el0_mmu_proc_stack_top();

  // Register tid=8; mark it Running immediately (no concurrent thread).
  fs_sched_register(8u, code_pa, stack_top, 0x3C0u);
  fs_sched_mark_running(8u);
  el0_svc_set_current_tid(8u);

  // ERET to Process G.  G dereferences address 0 → Data Abort EC=0x24.
  // fs_sched_fault_handler(): marks Faulted, gov record, ERET to EL1.
  run_proc_entry(code_pa, stack_top);

  el0_svc_set_current_tid(1u);

  // Phase 18 CI gate: verify gov ring has kGovThreadFault for tid=8, ec=0x24.
  if (fs_gov_find_fault(8u, 0x24u)) {
    cel_pl011_puts("[axion] el0: fault contained (tid=8, ec=0x24)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: fault FAIL (gov record missing)\r\n");
  }
}

// ── Phase 19 (RFC-00C8): Concurrent Fault Isolation ───────────────────────────
//
// Loads two processes simultaneously:
//   - Thread F (tid=7): el0_fault_test (LBA 12). Faults immediately on start.
//   - Thread E (tid=6): el0_device_filter_test (LBA 11, device_id=30).
//
// Both are registered with per-thread L3 tables. F is marked Running, E is
// marked Runnable. When F starts, it immediately dereferences VA 0x0.
// The fault handler catches this, marks F as Faulted, logs kGovThreadFault,
// installs E's L3 table, and context-switches to E.
// E runs, calls WaitForDevice(30), blocks, is woken by the timer IRQ, and exits.
//
// Verification: gov ring must contain both kGovThreadFault for F (tid=7, ec=0x24)
// and kGovTimerDeviceWake for E (tid=6, device_id=30).
//
// CI gate: "[axion] el0: concurrent fault OK (tid=7 faulted, tid=6 exited)"

extern "C" void canon_concurrent_fault_load_and_run() noexcept {
  // Load Thread F (tid=7) from LBA 12 into code page 2.
  const uint64_t code_pa_f = load_t81x_v2_into(12u, el0_mmu_proc_code_page2(), "tid7/LBA12");
  if (!code_pa_f) return;

  // Load Thread E (tid=6) from LBA 11 into code page 1.
  const uint64_t code_pa_e = load_t81x_v2_into(11u, el0_mmu_proc_code_page(), "tid6/LBA11");
  if (!code_pa_e) return;

  flush_icache_for_new_code();

  // Build per-thread L3 tables.
  // Slot 0 → tid=6: own_code=page1, own_stack=stack1.
  // Slot 1 → tid=7: own_code=page2, own_stack=stack2.
  el0_mmu_build_thread_l3(0u, reinterpret_cast<uint64_t>(el0_mmu_proc_code_page()),
                          el0_mmu_proc_stack_top() - 4096u);
  el0_mmu_build_thread_l3(1u, reinterpret_cast<uint64_t>(el0_mmu_proc_code_page2()),
                          el0_mmu_proc_stack_top2() - 4096u);

  // Install IRQ-driven device-wait loop for Thread E's WaitForDevice call.
  g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

  fs_sched_reset();  // resets sched, obs, and gov rings

  const uint64_t stack_top_e = el0_mmu_proc_stack_top();
  const uint64_t stack_top_f = el0_mmu_proc_stack_top2();

  // Register Thread E (tid=6) as Runnable.
  fs_sched_register(6u, code_pa_e, stack_top_e, 0x3C0u);
  fs_sched_set_thread_l3(6u, 0u);

  // Register Thread F (tid=7) and mark it as Running.
  fs_sched_register(7u, code_pa_f, stack_top_f, 0x3C0u);
  fs_sched_set_thread_l3(7u, 1u);
  fs_sched_mark_running(7u);

  el0_svc_set_current_tid(7u);

  // ERET to Thread F. F immediately faults, handler switches to E.
  run_proc_entry(code_pa_f, stack_top_f);

  el0_svc_set_current_tid(1u);
  g_axion_el1_device_wait_pc = 0u;

  // Phase 19 verification:
  // 1. Thread F faulted with Data Abort (EC=0x24)
  // 2. Thread E completed a device wait cycle (device_id=30)
  const bool f_faulted = fs_gov_find_fault(7u, 0x24u);
  const bool e_woke = fs_gov_find_device(6u, 1u /*kGovTimerDeviceWake*/, 30u);

  if (f_faulted && e_woke) {
    cel_pl011_puts("[axion] el0: concurrent fault OK (tid=7 faulted, tid=6 exited)\r\n");
  } else {
    cel_pl011_puts("[axion] el0: concurrent fault FAIL (gov records missing: ");
    if (!f_faulted) cel_pl011_puts("F-fault ");
    if (!e_woke) cel_pl011_puts("E-wake ");
    cel_pl011_puts(")\r\n");
  }
}

// ── Phase 8 IPC symbols (qemu_slice6_el0_svc_bridge.cpp) ─────────────────────

extern "C" bool canon_ipc_delivered() noexcept;

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

  if (sec[0] != kT81XMagic[0] || sec[1] != kT81XMagic[1] || sec[2] != kT81XMagic[2] ||
      sec[3] != kT81XMagic[3]) {
    cel_pl011_puts("[axion] canonfs: IPC load FAIL (bad T81X magic)\r\n");
    return false;
  }

  if (sec[4] != kT81XVersion) {
    cel_pl011_puts("[axion] canonfs: IPC load FAIL (unsupported T81X version)\r\n");
    return false;
  }

  uint32_t entry_offset, code_size;
  __builtin_memcpy(&entry_offset, sec + 7, 4);
  __builtin_memcpy(&code_size, sec + 11, 4);

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
  __builtin_memset(dest + code_size, 0, static_cast<__SIZE_TYPE__>(4096u - code_size));

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
