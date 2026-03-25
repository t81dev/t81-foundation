// ternaryos/hal/qemu_slice6_cpp_bridge.cpp
//
// Freestanding C++ kernel bridge for QEMU AArch64 bare-metal.
//
// Called from qemu_bare_kernel_entry() (qemu_slice6_bare_kernel.c) after
// ExitBootServices.  Provides the T81 governance banner and an interactive
// t81> prompt over the PL011 serial console without requiring a hosted C++
// standard-library environment.
//
// Compilation constraints:
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib
//   --target=aarch64-pc-windows-msvc  (PE32+ for EDK2 loader)
//
// No dynamic allocation; no C++ runtime; no libcxx headers.  Only language-
// level C++ and the freestanding subset of <stdint.h>.
//
// On hosted (macOS / x86-64) builds the AArch64-only MMIO blocks compile to
// no-ops and the function returns immediately so unit tests can link it.
//
// RFC-00B3 §3.9 (QEMU lane), RFC-00B0 §4.4 (ethics-first boot).

#include <stdint.h>

// ── QEMU virt AArch64 memory map ─────────────────────────────────────────────

static constexpr uint64_t kPl011Base     = UINT64_C(0x09000000);
static constexpr uint32_t kPl011DR       = 0x000u;  // Data Register
static constexpr uint32_t kPl011FR       = 0x018u;  // Flag Register
static constexpr uint32_t kPl011FRtxff   = (1u << 5);  // TX FIFO full
static constexpr uint32_t kPl011FRRXFE   = (1u << 4);  // RX FIFO empty

// ── MMIO helpers ─────────────────────────────────────────────────────────────

static inline void mmio_write32(uint64_t base, uint32_t off, uint32_t val) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  *reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(base + off)) = val;
#else
  (void)base; (void)off; (void)val;
#endif
}

static inline uint32_t mmio_read32(uint64_t base, uint32_t off) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  return *reinterpret_cast<const volatile uint32_t*>(
      static_cast<uintptr_t>(base + off));
#else
  (void)base; (void)off;
  return 0u;
#endif
}

// ── PL011 TX / RX ────────────────────────────────────────────────────────────

static void pl011_putchar(char c) noexcept {
  while (mmio_read32(kPl011Base, kPl011FR) & kPl011FRtxff) {
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("yield" ::: "memory");
#endif
  }
  mmio_write32(kPl011Base, kPl011DR,
               static_cast<uint32_t>(static_cast<unsigned char>(c)));
}

static void pl011_puts(const char* s) noexcept {
  while (*s) pl011_putchar(*s++);
}

static bool pl011_rx_ready() noexcept {
  return !(mmio_read32(kPl011Base, kPl011FR) & kPl011FRRXFE);
}

static int pl011_getchar() noexcept {
  if (!pl011_rx_ready()) return -1;
  return static_cast<int>(mmio_read32(kPl011Base, kPl011DR) & 0xFFu);
}

// ── Virtio-blk MMIO probe ────────────────────────────────────────────────────
// Probe the second virtio MMIO slot for the CanonFS block device.
//
// QEMU virt allocates virtio-mmio slots top-down (highest slot first).
// With 32 slots (0x0A000000 + N*0x200) and two virtio-blk-device instances:
//   Slot 31 (0x0A003E00): first  device added = FAT32 boot disk
//   Slot 30 (0x0A003C00): second device added = CanonFS raw block store
//
// This is confirmed by the EDK2 BdsDxe boot log:
//   Boot0001: VenHw(…,003C000A…) → No EFI found  (CanonFS store, slot 30)
//   Boot0002: VenHw(…,003E000A…) → BOOTAA64.EFI  (boot disk, slot 31)
//
// Only checks the magic / version / device-ID registers — does not
// initialise the queue (the full driver lives in VirtioBlkMmioDevice).
// Used only to select the banner text; the hosted C++ kernel does the
// full initialisation through IBlockDevice.

static constexpr uint64_t kVirtioMmioBase = UINT64_C(0x0A003C00);

enum class CanonFsProbeStatus : uint8_t {
  NotRun = 0,
  Ok,
  FailFeatureNegotiation,
  FailQueueTooSmall,
  FailLba0Read,
  FailLba0Magic,
  FailLba1Write,
  FailLba1ReadBack,
  FailLba1Mismatch,
};

static CanonFsProbeStatus s_canonfs_probe_status = CanonFsProbeStatus::NotRun;

static const char* canonfs_probe_status_text(CanonFsProbeStatus status) noexcept {
  switch (status) {
    case CanonFsProbeStatus::NotRun:                 return "not-run";
    case CanonFsProbeStatus::Ok:                     return "ok";
    case CanonFsProbeStatus::FailFeatureNegotiation: return "fail(feature-negotiation)";
    case CanonFsProbeStatus::FailQueueTooSmall:      return "fail(queue-too-small)";
    case CanonFsProbeStatus::FailLba0Read:           return "fail(lba0-read)";
    case CanonFsProbeStatus::FailLba0Magic:          return "fail(lba0-magic)";
    case CanonFsProbeStatus::FailLba1Write:          return "fail(lba1-write)";
    case CanonFsProbeStatus::FailLba1ReadBack:       return "fail(lba1-read-back)";
    case CanonFsProbeStatus::FailLba1Mismatch:       return "fail(lba1-mismatch)";
  }
  return "unknown";
}

static bool probe_virtio_blk_bare() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  const uint32_t magic = mmio_read32(kVirtioMmioBase, 0x000u);
  const uint32_t ver   = mmio_read32(kVirtioMmioBase, 0x004u);
  const uint32_t devid = mmio_read32(kVirtioMmioBase, 0x008u);
  return magic == 0x74726976u && ver == 2u && devid == 2u;
#else
  return false;
#endif
}

// ── Virtio-blk I/O probe (AArch64, modern MMIO transport) ────────────────────
// Full virtio queue initialisation + 3-step I/O test:
//   1. READ  LBA 0 → verify CST1 magic
//   2. WRITE known pattern to LBA 1
//   3. READ  LBA 1 → byte-compare round-trip
// Called once from qemu_cpp_bridge_entry() when s_has_blk is true.
// Emits [axion] canonfs: I/O probe OK/FAIL banner.

static constexpr int kVQSize = 4;  // queue depth — power-of-2, ≥3 (descs/req)

// Virtio-mmio register offsets (spec 1.0 §4.2.2, modern transport).
static constexpr uint32_t kVR_Status      = 0x070u;
static constexpr uint32_t kVR_DrvFeatSel  = 0x024u;
static constexpr uint32_t kVR_DrvFeatures = 0x020u;
static constexpr uint32_t kVR_QueueSel    = 0x030u;
static constexpr uint32_t kVR_QueueNumMax = 0x034u;
static constexpr uint32_t kVR_QueueNum    = 0x038u;
static constexpr uint32_t kVR_QueueReady  = 0x044u;
static constexpr uint32_t kVR_QueueNotify = 0x050u;
static constexpr uint32_t kVR_QueueDescLo = 0x080u;
static constexpr uint32_t kVR_QueueDescHi = 0x084u;
static constexpr uint32_t kVR_QueueDrvLo  = 0x090u;
static constexpr uint32_t kVR_QueueDrvHi  = 0x094u;
static constexpr uint32_t kVR_QueueDevLo  = 0x0A0u;
static constexpr uint32_t kVR_QueueDevHi  = 0x0A4u;

static constexpr uint32_t kVS_Ack         = 0x01u;
static constexpr uint32_t kVS_Driver      = 0x02u;
static constexpr uint32_t kVS_DriverOk    = 0x04u;
static constexpr uint32_t kVS_FeaturesOk  = 0x08u;

static constexpr uint16_t kVD_Next        = 0x0001u;
static constexpr uint16_t kVD_Write       = 0x0002u;

static constexpr uint32_t kBlkIn          = 0u;  // device→memory (read)
static constexpr uint32_t kBlkOut         = 1u;  // memory→device (write)

struct VirtqDesc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; };
struct VirtqAvail { uint16_t flags; uint16_t idx; uint16_t ring[kVQSize]; };
struct VirtqUsedElem { uint32_t id; uint32_t len; };
struct VirtqUsed  { uint16_t flags; uint16_t idx; VirtqUsedElem ring[kVQSize]; };
struct VirtioBlkReqHdr { uint32_t type; uint32_t reserved; uint64_t sector; };

// Static virtio queue data — resides in BSS (zero at startup, identity-mapped).
alignas(16)  static VirtqDesc       s_vq_desc[kVQSize];
alignas(2)   static VirtqAvail      s_vq_avail;
alignas(4)   static VirtqUsed       s_vq_used;
alignas(16)  static VirtioBlkReqHdr s_blk_req;
alignas(512) static uint8_t         s_sector_buf[512];
static uint8_t s_blk_status;

static inline void aarch64_dsb() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile("dsb sy" ::: "memory");
#endif
}

// Submit one virtio-blk request and poll for completion.
static bool vblk_do_io(uint32_t type, uint64_t lba) noexcept {
  s_blk_req.type     = type;
  s_blk_req.reserved = 0u;
  s_blk_req.sector   = lba;
  s_blk_status       = 0xFFu;

  s_vq_desc[0] = { reinterpret_cast<uint64_t>(&s_blk_req),
                   static_cast<uint32_t>(sizeof(s_blk_req)),
                   kVD_Next, 1u };
  s_vq_desc[1] = { reinterpret_cast<uint64_t>(s_sector_buf), 512u,
                   static_cast<uint16_t>(kVD_Next |
                     (type == kBlkIn ? kVD_Write : 0u)), 2u };
  s_vq_desc[2] = { reinterpret_cast<uint64_t>(&s_blk_status), 1u,
                   kVD_Write, 0u };

  const uint16_t old_used = s_vq_used.idx;
  s_vq_avail.ring[s_vq_avail.idx % kVQSize] = 0u;
  aarch64_dsb();
  s_vq_avail.idx = static_cast<uint16_t>(s_vq_avail.idx + 1u);
  aarch64_dsb();
  mmio_write32(kVirtioMmioBase, kVR_QueueNotify, 0u);
  aarch64_dsb();

  for (uint32_t i = 0u; i < 2000000u; ++i) {
    aarch64_dsb();
    if (s_vq_used.idx != old_used) break;
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("yield" ::: "memory");
#endif
  }
  return (s_vq_used.idx != old_used) && (s_blk_status == 0u);
}

static void canonfs_io_probe() noexcept {
  const uint64_t mmio = kVirtioMmioBase;
  s_canonfs_probe_status = CanonFsProbeStatus::NotRun;

  // Reset → ACK → DRIVER → negotiate features (none) → FEATURES_OK.
  mmio_write32(mmio, kVR_Status, 0u);
  aarch64_dsb();
  mmio_write32(mmio, kVR_Status, kVS_Ack | kVS_Driver);
  mmio_write32(mmio, kVR_DrvFeatSel, 0u);
  mmio_write32(mmio, kVR_DrvFeatures, 0u);
  mmio_write32(mmio, kVR_Status, kVS_Ack | kVS_Driver | kVS_FeaturesOk);
  aarch64_dsb();
  if (!(mmio_read32(mmio, kVR_Status) & kVS_FeaturesOk)) {
    s_canonfs_probe_status = CanonFsProbeStatus::FailFeatureNegotiation;
    pl011_puts("[axion] canonfs: I/O probe FAIL (feature negotiation)\r\n");
    return;
  }

  // Queue 0 setup.
  mmio_write32(mmio, kVR_QueueSel, 0u);
  if (mmio_read32(mmio, kVR_QueueNumMax) < static_cast<uint32_t>(kVQSize)) {
    s_canonfs_probe_status = CanonFsProbeStatus::FailQueueTooSmall;
    pl011_puts("[axion] canonfs: I/O probe FAIL (queue too small)\r\n");
    return;
  }
  mmio_write32(mmio, kVR_QueueNum, static_cast<uint32_t>(kVQSize));

  const uint64_t desc_pa  = reinterpret_cast<uint64_t>(s_vq_desc);
  const uint64_t avail_pa = reinterpret_cast<uint64_t>(&s_vq_avail);
  const uint64_t used_pa  = reinterpret_cast<uint64_t>(&s_vq_used);
  mmio_write32(mmio, kVR_QueueDescLo, static_cast<uint32_t>(desc_pa));
  mmio_write32(mmio, kVR_QueueDescHi, static_cast<uint32_t>(desc_pa  >> 32));
  mmio_write32(mmio, kVR_QueueDrvLo,  static_cast<uint32_t>(avail_pa));
  mmio_write32(mmio, kVR_QueueDrvHi,  static_cast<uint32_t>(avail_pa >> 32));
  mmio_write32(mmio, kVR_QueueDevLo,  static_cast<uint32_t>(used_pa));
  mmio_write32(mmio, kVR_QueueDevHi,  static_cast<uint32_t>(used_pa  >> 32));
  mmio_write32(mmio, kVR_QueueReady, 1u);

  // DRIVER_OK — device is live.
  mmio_write32(mmio, kVR_Status,
               kVS_Ack | kVS_Driver | kVS_FeaturesOk | kVS_DriverOk);
  aarch64_dsb();

  // LBA 0 READ → check CST1 magic.
  for (int i = 0; i < 512; ++i) s_sector_buf[i] = 0u;
  if (!vblk_do_io(kBlkIn, 0u)) {
    s_canonfs_probe_status = CanonFsProbeStatus::FailLba0Read;
    pl011_puts("[axion] canonfs: I/O probe FAIL (LBA0 read error)\r\n");
    return;
  }
  if (s_sector_buf[0] != 'C' || s_sector_buf[1] != 'S' ||
      s_sector_buf[2] != 'T' || s_sector_buf[3] != '1') {
    s_canonfs_probe_status = CanonFsProbeStatus::FailLba0Magic;
    pl011_puts("[axion] canonfs: I/O probe FAIL (LBA0 bad magic)\r\n");
    return;
  }

  // LBA 1 WRITE known pattern.
  for (int i = 0; i < 512; ++i)
    s_sector_buf[i] = static_cast<uint8_t>(0xA5u ^ static_cast<uint8_t>(i));
  if (!vblk_do_io(kBlkOut, 1u)) {
    s_canonfs_probe_status = CanonFsProbeStatus::FailLba1Write;
    pl011_puts("[axion] canonfs: I/O probe FAIL (LBA1 write error)\r\n");
    return;
  }

  // LBA 1 READ back and verify.
  for (int i = 0; i < 512; ++i) s_sector_buf[i] = 0u;
  if (!vblk_do_io(kBlkIn, 1u)) {
    s_canonfs_probe_status = CanonFsProbeStatus::FailLba1ReadBack;
    pl011_puts("[axion] canonfs: I/O probe FAIL (LBA1 read-back error)\r\n");
    return;
  }
  for (int i = 0; i < 512; ++i) {
    if (s_sector_buf[i] !=
        static_cast<uint8_t>(0xA5u ^ static_cast<uint8_t>(i))) {
      s_canonfs_probe_status = CanonFsProbeStatus::FailLba1Mismatch;
      pl011_puts("[axion] canonfs: I/O probe FAIL (LBA1 mismatch)\r\n");
      return;
    }
  }

  s_canonfs_probe_status = CanonFsProbeStatus::Ok;
  pl011_puts("[axion] canonfs: I/O probe OK"
             " (LBA0 magic=CST1, LBA1 round-trip pass)\r\n");
}

// ── CanonFS I/O wrappers (used by canon_exec_loader.cpp) ─────────────────────
// These thin wrappers expose the static vblk_do_io() and s_sector_buf to other
// TUs.  The queue must already be initialised by canonfs_io_probe() before
// these are called.

extern "C" bool canon_store_read_lba(uint64_t lba) noexcept {
  return vblk_do_io(kBlkIn, lba);
}

extern "C" const uint8_t* canon_store_sector_buf() noexcept {
  return s_sector_buf;
}

// ── ARM generic timer ────────────────────────────────────────────────────────
// CNTPCT_EL0 (physical counter) and CNTFRQ_EL0 (frequency in Hz) are
// accessible from EL1 without additional configuration on QEMU virt.
// Typical QEMU value: 62 500 000 Hz.

static uint64_t read_cntpct() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  uint64_t v;
  __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v));
  return v;
#else
  return 0;
#endif
}

static uint64_t read_cntfrq() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  uint64_t v;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
  return v > 0 ? v : 1;
#else
  return 1;  // avoid div-by-zero on hosted builds
#endif
}

// ── String helpers (no stdlib) ───────────────────────────────────────────────

static bool str_eq(const char* a, const char* b) noexcept {
  while (*a && *b && *a == *b) { ++a; ++b; }
  return *a == *b;
}

static const char* str_trim(const char* s) noexcept {
  while (*s == ' ') ++s;
  return s;
}

// Render uint64_t as decimal into caller buffer (bufsz >= 21).
// Returns pointer into buf at the start of the rendered string.
static const char* u64_dec(uint64_t v, char* buf, int bufsz) noexcept {
  buf[bufsz - 1] = '\0';
  int i = bufsz - 2;
  if (v == 0) {
    buf[i--] = '0';
  } else {
    while (v > 0 && i >= 0) {
      buf[i--] = static_cast<char>('0' + v % 10);
      v /= 10;
    }
  }
  return &buf[i + 1];
}

static const char* u64_hex(uint64_t v, char* buf, int bufsz) noexcept {
  static constexpr char kHex[] = "0123456789abcdef";
  if (bufsz < 3) return "";
  buf[bufsz - 1] = '\0';
  int i = bufsz - 2;
  if (v == 0) {
    buf[i--] = '0';
  } else {
    while (v > 0 && i >= 2) {
      buf[i--] = kHex[static_cast<int>(v & 0xFu)];
      v >>= 4;
    }
  }
  buf[i--] = 'x';
  buf[i] = '0';
  return &buf[i];
}

// ── Freestanding thread table ─────────────────────────────────────────────────
// Mirrors KernelRuntimeState::thread_runtime without heap allocation.
// Kernel thread (tid=1) is registered at bridge entry.

enum class FsThreadState : uint8_t { Empty = 0, Running = 1, Ready = 2, Blocked = 3 };

struct FsThread {
  uint32_t      tid        = 0;
  FsThreadState state      = FsThreadState::Empty;
  uint64_t      tick_count = 0;
};

static constexpr int kMaxFsThreads = 8;
static FsThread  s_threads[kMaxFsThreads];
static int       s_thread_count   = 0;
static int       s_current_thread = 0;

// ── Live kernel counters ──────────────────────────────────────────────────────
// Mirrors KernelRuntimeState::Counters for the freestanding bridge context.

static uint64_t s_boot_cntpct    = 0;  // captured at bridge entry
static uint64_t s_cmd_count      = 0;  // commands dispatched
static uint64_t s_loop_iters     = 0;  // priority-dispatch loop iterations
static uint64_t s_tick_count     = 0;  // scheduler tick calls
static uint64_t s_sched_switches = 0;  // thread context switches
static uint64_t s_interrupt_count = 0; // serial RX events (our interrupt source)
static uint64_t s_timer_irqs     = 0;  // hardware timer IRQ count (GICv3 PPI30)
static bool     s_has_blk        = false;

static constexpr uint64_t kSchedTickInterval = 500u;

// ── Hardware-timer IRQ callback ───────────────────────────────────────────────
// Called from axion_irq_handler_aarch64() (qemu_slice6_bridge_irq.cpp) on
// every GICv3 PPI 30 tick (~100Hz).  Declared extern "C" so the IRQ file can
// call it without name-mangling.

extern "C" void bridge_timer_irq_tick() noexcept {
  ++s_timer_irqs;
  // Drive the cooperative scheduler from the hardware timer tick.
  // freestanding_sched_tick() is defined below; forward-declare for clarity.
  // (The linker resolves the call since both are in the same translation unit.)
  ++s_tick_count;
  if (s_thread_count > 1 && (s_tick_count % kSchedTickInterval) == 0) {
    s_threads[s_current_thread].state = FsThreadState::Ready;
    int next = (s_current_thread + 1) % s_thread_count;
    while (next != s_current_thread) {
      if (s_threads[next].state == FsThreadState::Ready) break;
      next = (next + 1) % s_thread_count;
    }
    if (next != s_current_thread) {
      s_threads[next].state    = FsThreadState::Running;
      s_current_thread         = next;
      ++s_sched_switches;
    } else {
      s_threads[s_current_thread].state = FsThreadState::Running;
    }
  }
  ++s_threads[s_current_thread].tick_count;
}

// Forward declaration of the hw-init function (defined in bridge_irq.cpp).
extern "C" void bridge_hw_init_aarch64() noexcept;

// ── EL0 init bootstrap ────────────────────────────────────────────────────────
// Symbols from axion_el0_init.S, qemu_slice6_bridge_irq.cpp, and
// qemu_slice6_el0_mmu.cpp (Phase 5: controlled TTBR0 page tables).
extern "C" void     axion_el0_entry()  noexcept;
extern "C" uint64_t g_axion_el1_return_pc;
extern "C" void     el0_mmu_init()           noexcept;  // build + install TTBR0 tables
extern "C" uint64_t el0_mmu_stack_top()      noexcept;  // PA of top of EL0 init stack
// Phase 7: process pages (qemu_slice6_el0_mmu.cpp).
extern "C" uint8_t* el0_mmu_proc_code_page() noexcept;
extern "C" uint64_t el0_mmu_proc_stack_top() noexcept;

// Phase 7: CanonFS process loader (canon_exec_loader.cpp).
extern "C" void canon_exec_load_and_run() noexcept;
// Phase 8: EL0 IPC roundtrip (canon_exec_loader.cpp).
extern "C" void canon_ipc_load_and_run() noexcept;
// Phase 9 (RFC-00BE): cooperative scheduler roundtrip (canon_exec_loader.cpp).
extern "C" void canon_sched_load_and_run() noexcept;
// Phase 11 (RFC-00C0): T81X v2 identity validation + WaitForDevice waker.
extern "C" void canon_identity_load_and_run() noexcept;
// Phase 13 (RFC-00C2): IRQ-driven WaitForDevice wake (canon_exec_loader.cpp).
extern "C" void canon_irq_wake_load_and_run() noexcept;
// Phase 15 (RFC-00C4): per-device wake filtering (canon_exec_loader.cpp).
extern "C" void canon_device_filter_load_and_run() noexcept;
// Phase 16 (RFC-00C5): concurrent device wait (canon_exec_loader.cpp).
extern "C" void canon_concurrent_wait_load_and_run() noexcept;
// Phase 17 (RFC-00C6): per-thread address-space isolation (canon_exec_loader.cpp).
extern "C" void canon_per_thread_pt_load_and_run() noexcept;
// Phase 18 (RFC-00C7): EL0 fault containment (canon_exec_loader.cpp).
extern "C" void canon_fault_contain_load_and_run() noexcept;
// Phase 19 (RFC-00C8): concurrent fault isolation (canon_exec_loader.cpp).
extern "C" void canon_concurrent_fault_load_and_run() noexcept;
// Phase 20 (RFC-00CA): EL0 fault summary query (canon_exec_loader.cpp).
extern "C" void canon_fault_summary_query_load_and_run() noexcept;
// Phase 21 (RFC-00CB): EL0 fault detail query (canon_exec_loader.cpp).
extern "C" void canon_fault_detail_query_load_and_run() noexcept;
// Slice6 shell introspection helpers (qemu_slice6_el0_svc_bridge.cpp).
extern "C" uint64_t fs_sched_faulted_count() noexcept;
extern "C" bool fs_sched_fault_nth(uint32_t index,
                                    uint32_t* out_tid,
                                    uint32_t* out_ec,
                                    uint64_t* out_far) noexcept;
extern "C" uint64_t fs_gov_count() noexcept;
extern "C" uint64_t fs_gov_event_count(uint32_t event) noexcept;

// ERets to axion_el0_entry at EL0t (SPSR_EL1 = 0x3C0 — EL0 + DAIF masked).
// Saves the EL1 resume label in g_axion_el1_return_pc BEFORE ERET so the
// ExitToEL1 SVC (#2) handler can redirect the return ERET back here.
// Returns only after SVC #2 fires and the trap frame is patched.
// Phase 5: el0_sp comes from el0_mmu_stack_top() (4 KB page in controlled
// TTBR0); the old 1 KiB BSS buffer has been removed.
static void run_el0_init() noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  const uint64_t el0_fn =
      reinterpret_cast<uint64_t>(reinterpret_cast<void*>(axion_el0_entry));
  const uint64_t el0_sp = el0_mmu_stack_top();

  __asm__ volatile(
      // Preserve EL1 callee-saved state across the EL0 roundtrip. The trap
      // exit path restores EL0 registers before it ERETs back here.
      "sub     sp, sp, #96\n\t"
      "stp     x19, x20, [sp, #0]\n\t"
      "stp     x21, x22, [sp, #16]\n\t"
      "stp     x23, x24, [sp, #32]\n\t"
      "stp     x25, x26, [sp, #48]\n\t"
      "stp     x27, x28, [sp, #64]\n\t"
      "stp     x29, x30, [sp, #80]\n\t"
      // x8 = address of EL1 resume label (after eret); save to global.
      "adr     x8, 1f\n\t"
      "str     x8, [%[ret_pc]]\n\t"
      // Set ERET target and EL0 stack.
      "msr     elr_el1,  %[el0_fn]\n\t"
      "msr     sp_el0,   %[el0_sp]\n\t"
      // SPSR_EL1 = 0x3C0: EL0t (M=0b0000) + DAIF all masked (bits[9:6]=1111).
      "mov     x8, #0x3c0\n\t"
      "msr     spsr_el1, x8\n\t"
      "isb\n\t"
      // ERET to EL0 — execution returns here when SVC #2 fires.
      "eret\n\t"
      "1:\n\t"
      "ldp     x19, x20, [sp, #0]\n\t"
      "ldp     x21, x22, [sp, #16]\n\t"
      "ldp     x23, x24, [sp, #32]\n\t"
      "ldp     x25, x26, [sp, #48]\n\t"
      "ldp     x27, x28, [sp, #64]\n\t"
      "ldp     x29, x30, [sp, #80]\n\t"
      "add     sp, sp, #96\n\t"
      :
      : [ret_pc] "r"(&g_axion_el1_return_pc),
        [el0_fn] "r"(el0_fn),
        [el0_sp] "r"(el0_sp)
      : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
        "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "memory");
#endif
}

// ── Freestanding scheduler tick ───────────────────────────────────────────────
// Fallback poll-based tick — used only when IRQs have not fired yet (e.g.
// the first few loop iterations before GICv3 is live).  After hardware timer
// IRQs start firing, bridge_timer_irq_tick() drives the scheduler directly.
// Round-robin among ready threads every kSchedTickInterval loop iterations.

static void freestanding_sched_tick() noexcept {
  ++s_tick_count;
  if (s_thread_count <= 1 || (s_tick_count % kSchedTickInterval) != 0) {
    ++s_threads[s_current_thread].tick_count;
    return;
  }
  // Mark current thread Ready; advance to next runnable thread.
  s_threads[s_current_thread].state = FsThreadState::Ready;
  int next = (s_current_thread + 1) % s_thread_count;
  while (next != s_current_thread) {
    if (s_threads[next].state == FsThreadState::Ready) break;
    next = (next + 1) % s_thread_count;
  }
  if (next != s_current_thread) {
    s_threads[next].state = FsThreadState::Running;
    s_current_thread = next;
    ++s_sched_switches;
  } else {
    s_threads[s_current_thread].state = FsThreadState::Running;
  }
  ++s_threads[s_current_thread].tick_count;
}

// ── Shell command handlers ────────────────────────────────────────────────────

static void cmd_help() noexcept {
  pl011_puts("  help     -- this message\r\n");
  pl011_puts("  uname    -- system identity (RFC-00B9 §8.3)\r\n");
  pl011_puts("  version  -- T81 build info\r\n");
  pl011_puts("  canonfs  -- storage transport and probe status\r\n");
  pl011_puts("  status   -- kernel counters and governance state\r\n");
  pl011_puts("  threads  -- thread table (tid, state, ticks)\r\n");
  pl011_puts("  sched    -- scheduler counters (loop iters, ticks, switches)\r\n");
  pl011_puts("  faults   -- retained EL0 fault records (tid, ec, far)\r\n");
  pl011_puts("  gov      -- governance ring counters by event\r\n");
  pl011_puts("  policy   -- Axion policy summary\r\n");
}

static void cmd_uname() noexcept {
  pl011_puts("  T81 TernaryOS 1.0 AArch64 axion-kernel (bare-metal EFI)\r\n");
}

static void cmd_version() noexcept {
  pl011_puts("  T81 / Axion  --  ternary OS kernel (bare-metal EFI bridge)\r\n");
  pl011_puts("  Architecture : AArch64 (QEMU virt, cortex-a57, EDK2)\r\n");
  pl011_puts("  Boot path    : EFI efi_main -> ExitBootServices -> C++ bridge\r\n");
}

static void cmd_canonfs() noexcept {
  char hex[24];
  pl011_puts("  [canonfs]\r\n");
  if (s_has_blk) {
    pl011_puts("    mode          : persistent (virtio-blk)\r\n");
    pl011_puts("    transport     : virtio-mmio v2\r\n");
    pl011_puts("    store_mmio    : ");
    pl011_puts(u64_hex(kVirtioMmioBase, hex, static_cast<int>(sizeof(hex))));
    pl011_puts("\r\n");
    pl011_puts("    io_probe      : ");
    pl011_puts(canonfs_probe_status_text(s_canonfs_probe_status));
    pl011_puts("\r\n");
    pl011_puts("    lba0_expect   : CST1\r\n");
    pl011_puts("    lba1_probe    : round-trip pattern\r\n");
  } else {
    pl011_puts("    mode          : in-memory\r\n");
    pl011_puts("    transport     : none\r\n");
    pl011_puts("    io_probe      : skipped (no virtio-blk store)\r\n");
  }
}

static void cmd_status() noexcept {
  char buf[24];

  const uint64_t now      = read_cntpct();
  const uint64_t freq     = read_cntfrq();
  const uint64_t uptime_s = (now - s_boot_cntpct) / freq;

  pl011_puts("  [kernel]\r\n");
  pl011_puts("    path          : bare-metal (EFI C++ bridge, AArch64)\r\n");
  if (s_has_blk) {
    pl011_puts("    canonfs       : mounted (persistent, virtio-blk)\r\n");
  } else {
    pl011_puts("    canonfs       : mounted (in-memory)\r\n");
  }
  pl011_puts("    policy engine : ready\r\n");

  pl011_puts("    threads       : ");
  pl011_puts(u64_dec(static_cast<uint64_t>(s_thread_count), buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    uptime (s)    : ");
  pl011_puts(u64_dec(uptime_s, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    loop_iters    : ");
  pl011_puts(u64_dec(s_loop_iters, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    tick_count    : ");
  pl011_puts(u64_dec(s_tick_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    sched switches: ");
  pl011_puts(u64_dec(s_sched_switches, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    interrupts    : ");
  pl011_puts(u64_dec(s_interrupt_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    commands      : ");
  pl011_puts(u64_dec(s_cmd_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
}

static void cmd_threads() noexcept {
  char buf[24];
  pl011_puts("  [threads]\r\n");
  for (int i = 0; i < s_thread_count; ++i) {
    const FsThread& t = s_threads[i];
    pl011_puts("    tid=");
    pl011_puts(u64_dec(t.tid, buf, static_cast<int>(sizeof(buf))));
    switch (t.state) {
      case FsThreadState::Running: pl011_puts("  Running"); break;
      case FsThreadState::Ready:   pl011_puts("  Ready  "); break;
      case FsThreadState::Blocked: pl011_puts("  Blocked"); break;
      default:                     pl011_puts("  Empty  "); break;
    }
    pl011_puts("  ticks=");
    pl011_puts(u64_dec(t.tick_count, buf, static_cast<int>(sizeof(buf))));
    pl011_puts("\r\n");
  }
  pl011_puts("    count=");
  pl011_puts(u64_dec(static_cast<uint64_t>(s_thread_count), buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
}

static void cmd_sched() noexcept {
  char buf[24];
  pl011_puts("  [scheduler]\r\n");
  pl011_puts("    model        : preemptive (GICv3 PPI30, 100Hz)\r\n");
  pl011_puts("    loop_iters   : ");
  pl011_puts(u64_dec(s_loop_iters, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    tick_count   : ");
  pl011_puts(u64_dec(s_tick_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    timer_irqs   : ");
  pl011_puts(u64_dec(s_timer_irqs, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    switches     : ");
  pl011_puts(u64_dec(s_sched_switches, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    serial_rx    : ");
  pl011_puts(u64_dec(s_interrupt_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    tick_interval: ");
  pl011_puts(u64_dec(kSchedTickInterval, buf, static_cast<int>(sizeof(buf))));
  pl011_puts(" hw ticks\r\n");
}

static void cmd_faults() noexcept {
  char buf[24];
  char hex[24];
  const uint64_t count = fs_sched_faulted_count();

  pl011_puts("  [faults]\r\n");
  pl011_puts("    count         : ");
  pl011_puts(u64_dec(count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  if (count == 0u) {
    pl011_puts("    retained      : none\r\n");
    return;
  }

  for (uint32_t i = 0u; i < count; ++i) {
    uint32_t tid = 0u;
    uint32_t ec = 0u;
    uint64_t far = 0u;
    if (!fs_sched_fault_nth(i, &tid, &ec, &far)) continue;

    pl011_puts("    tid=");
    pl011_puts(u64_dec(static_cast<uint64_t>(tid), buf, static_cast<int>(sizeof(buf))));
    pl011_puts(" ec=");
    pl011_puts(u64_hex(static_cast<uint64_t>(ec), hex, static_cast<int>(sizeof(hex))));
    pl011_puts(" far=");
    pl011_puts(u64_hex(far, hex, static_cast<int>(sizeof(hex))));
    pl011_puts("\r\n");
  }
}

static void cmd_gov() noexcept {
  char buf[24];
  const uint64_t total = fs_gov_count();
  const uint64_t timer = fs_gov_event_count(1u);
  const uint64_t async = fs_gov_event_count(2u);
  const uint64_t fault = fs_gov_event_count(3u);

  pl011_puts("  [governance]\r\n");
  pl011_puts("    ring events   : ");
  pl011_puts(u64_dec(total, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    timer_wake    : ");
  pl011_puts(u64_dec(timer, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    async_switch  : ");
  pl011_puts(u64_dec(async, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
  pl011_puts("    thread_fault  : ");
  pl011_puts(u64_dec(fault, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");
}

static void cmd_policy() noexcept {
  pl011_puts("  [axion policy]\r\n");
  pl011_puts("    governance  : active\r\n");
  pl011_puts("    audit trail : canonfs (in-memory)\r\n");
  pl011_puts("    constraints : RFC-00B0 ethics-first boot\r\n");
}

static void shell_dispatch(const char* line) noexcept {
  line = str_trim(line);
  if (*line == '\0') return;

  ++s_cmd_count;

  if      (str_eq(line, "help"))    { cmd_help(); }
  else if (str_eq(line, "uname"))   { cmd_uname(); }
  else if (str_eq(line, "version")) { cmd_version(); }
  else if (str_eq(line, "canonfs")) { cmd_canonfs(); }
  else if (str_eq(line, "status"))  { cmd_status(); }
  else if (str_eq(line, "threads")) { cmd_threads(); }
  else if (str_eq(line, "sched"))   { cmd_sched(); }
  else if (str_eq(line, "faults"))  { cmd_faults(); }
  else if (str_eq(line, "gov"))     { cmd_gov(); }
  else if (str_eq(line, "policy"))  { cmd_policy(); }
  else {
    pl011_puts("  [axion] ShellExec: Deny -- '");
    pl011_puts(line);
    pl011_puts("' not in builtin table\r\n");
  }
}

// ── Static shell buffer (file scope avoids static-local runtime concerns) ────

static char s_line[64];
static int  s_line_len;

// ── Bridge entry point ────────────────────────────────────────────────────────
//
// Called from qemu_bare_kernel_entry() (C linkage) after the bare-metal C
// probe has confirmed EL1 PL011 MMIO access.  Never returns on AArch64;
// returns immediately on hosted builds.

extern "C" void qemu_cpp_bridge_entry(void) noexcept {
  // Capture boot timestamp.
  s_boot_cntpct = read_cntpct();

  // Register kernel thread (tid=1) in the freestanding thread table.
  s_threads[0] = FsThread{1u, FsThreadState::Running, 0u};
  s_thread_count   = 1;
  s_current_thread = 0;

  s_has_blk = probe_virtio_blk_bare();

  // Governance banner.
  pl011_puts("\r\n");
  pl011_puts("  T81  --  Ternary OS for AI\r\n");
  pl011_puts("  ===========================\r\n");
  pl011_puts("\r\n");
  pl011_puts("[axion] policy engine: ready\r\n");
  if (s_has_blk) {
    pl011_puts("[axion] canonfs: mounted (persistent, virtio-blk)\r\n");
    canonfs_io_probe();
  } else {
    pl011_puts("[axion] canonfs: mounted (in-memory)\r\n");
  }
  pl011_puts("[axion] kernel thread tid=1: running\r\n");
  pl011_puts("[axion] event loop: priority dispatch (interrupt > pager > sched)\r\n");

  // Wire hardware timer interrupts: GICv3 + ARM physical timer (PPI 30, ~100Hz).
  bridge_hw_init_aarch64();
  pl011_puts("[axion] hw timer: GICv3 PPI30 armed (10ms)\r\n");

  // Phase 5: Install controlled TTBR0 page tables before ERETing to EL0.
  // el0_mmu_init() replaces EDK2's identity-mapped TTBR0 with a minimal table
  // that maps only the code page (EL0 R+X) and the stack page (EL0 R/W).
  el0_mmu_init();
  pl011_puts("[axion] el0: page-isolated (TTBR0 active, EL0 stack mapped)\r\n");

  // EL0 init probe: ERET to axion_el0_entry at EL0t.
  // Phase 6 sequence (RFC-00BC):
  //   SVC #1 (KernelCall/GetThreadIdentity) → wire request on EL0 stack
  //   SVC #3 (WriteSerial) → "[axion] el0: init OK (tid=1)\r\n"
  //   SVC #2 (ExitThread)  → ERET returns here
  run_el0_init();
  // Phase 6 CI gate: KernelCall SVC bridge is wired if we reached this point.
  pl011_puts("[axion] el0: kernel call bridge OK (KernelCall SVC wired)\r\n");

  // Phase 7: CanonFS process loading — read T81X from LBA 3, copy to proc code
  // page (already mapped in TTBR0 by el0_mmu_init), set tid=2, ERET to EL0.
  // The process emits "[axion] el0: process loaded from CanonFS (tid=2)" via
  // WriteSerial SVC #3, which is the Phase 7 CI gate.
  if (s_has_blk) {
    canon_exec_load_and_run();
  }

  // Phase 8: EL0 IPC roundtrip — load Process A (LBA 4, tid=2) and
  // Process B (LBA 5, tid=3); confirm A→B message delivery.
  // CI gate: "[axion] el0: IPC roundtrip OK (A->B, tid=2,3)"
  if (s_has_blk) {
    canon_ipc_load_and_run();
  }

  // Phase 9 (RFC-00BE): cooperative scheduler roundtrip — Process B (LBA 7,
  // tid=3) blocks on IpcReceive; Process A (LBA 6, tid=2) sends and exits;
  // scheduler resumes B; B exits.
  // CI gate: "[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)"
  if (s_has_blk) {
    canon_sched_load_and_run();
  }

  // Phase 11 (RFC-00C0): T81X v2 identity hash validation + WaitForDevice
  // waker roundtrip — Process C (LBA 8, tid=4) parks on WaitForDevice;
  // EL1 wakes it directly; C exits.
  // CI gates: "[axion] el0: identity OK (hash=verified, tid=4)"
  //           "[axion] el0: device wake OK (WaitForDevice tid=4)"
  if (s_has_blk) {
    canon_identity_load_and_run();
  }

  // Phase 13 (RFC-00C2): GICv3 timer-driven WaitForDevice wake.
  //   Process D (tid=5) calls WaitForDevice; the SVC handler redirects ERET to
  //   fs_sched_device_wait_loop (wfi idle); the timer ISR wakes D via
  //   fs_sched_timer_device_wake(); D resumes and calls ExitThread.
  //   CI gates:
  //     "[axion] el0: irq identity OK (hash=verified, tid=5)"
  //     "[axion] el0: irq wake OK (WaitForDevice tid=5, timer-driven)"
  if (s_has_blk) {
    canon_irq_wake_load_and_run();
  }

  // Phase 15 (RFC-00C4): per-device filter — Process E (tid=6) sends
  //   WaitForDevice(device_id=30); only the timer IRQ (INTID 30) unblocks it.
  //   CI gate: "[axion] el0: device filter OK (device_id=30, tid=6)"
  if (s_has_blk) {
    canon_device_filter_load_and_run();
  }

  // Phase 16 (RFC-00C5): concurrent device wait — Process E (tid=6, did=30) and
  //   Process F (tid=7, did=0/any) both park on WaitForDevice simultaneously.
  //   Timer IRQ (INTID 30) wakes both via per-device filter; both exit cleanly.
  //   CI gate: "[axion] el0: concurrent wake OK (device_id=30, tid=6+7)"
  if (s_has_blk) {
    canon_concurrent_wait_load_and_run();
  }

  // Phase 17 (RFC-00C6): per-thread address-space isolation — same scenario as
  //   Phase 16 but with per-thread L3 tables.  Before each ERET the scheduler
  //   swaps s_l2[block_idx] to the target thread's private L3 + TLBI, so each
  //   thread can only access its own EL0 proc pages.
  //   CI gate: "[axion] el0: per-thread pt OK (tid=6+7, isolated)"
  if (s_has_blk) {
    canon_per_thread_pt_load_and_run();
  }

  // Phase 18 (RFC-00C7): EL0 fault containment — Process G (tid=8) immediately
  //   dereferences address 0x0 (unmapped at EL0), triggering a Data Abort
  //   (ESR_EL1 EC=0x24).  The EL1 synchronous vector (offset 0x400) now checks
  //   the EC field; non-SVC faults route to fs_sched_fault_handler() which marks
  //   the thread Faulted, records kGovThreadFault in the gov ring, and returns
  //   to EL1 without hanging.
  //   CI gate: "[axion] el0: fault contained (tid=8, ec=0x24)"
  if (s_has_blk) {
    canon_fault_contain_load_and_run();
  }

  // Phase 19 (RFC-00C8): concurrent fault isolation — start faulting tid=7
  //   with healthy tid=6 already Runnable.  tid=7 faults immediately;
  //   fs_sched_fault_handler() must switch directly to tid=6, which then
  //   completes the normal WaitForDevice(timer) -> ExitThread path.
  //   CI gate:
  //     "[axion] el0: concurrent fault OK (tid=7 faulted, tid=6 exited)"
  if (s_has_blk) {
    canon_concurrent_fault_load_and_run();
  }

  // Phase 20 (RFC-00CA): start a faulting thread with an EL0 query thread
  //   already Runnable.  After the fault-handler handoff, the query thread
  //   calls KernelCall(QueryFaultSummary) from EL0 and exits; EL1 validates
  //   the returned summary block from the thread's stack.
  //   CI gate:
  //     "[axion] el0: fault summary OK (tid=9 sees tid=8 fault)"
  if (s_has_blk) {
    canon_fault_summary_query_load_and_run();
  }

  // Phase 21 (RFC-00CB): start a faulting thread with an EL0 observer thread
  //   already Runnable. After the fault-handler handoff, the observer issues
  //   KernelCall(ReadFaultInbox) for tid=8 and EL1 validates the returned
  //   retained EC/FAR detail block from the observer's stack.
  //   CI gate:
  //     "[axion] el0: fault detail OK (tid=10 sees tid=8 ec=0x24 far=0x0)"
  if (s_has_blk) {
    canon_fault_detail_query_load_and_run();
  }

  pl011_puts("[axion] t81sh: ready (principal=axion, tier=1)\r\n");

  pl011_puts("\r\n");
  pl011_puts("[axion@T81 tier=1]$ ");

  s_line_len = 0;

  // ── Priority-dispatch event loop ─────────────────────────────────────────────
  // Mirrors axion_kernel_step() priority order:
  //   1. Fault queue      (placeholder — no hardware faults in this context)
  //   2. Interrupt source (serial RX ≡ our hardware interrupt equivalent)
  //   3. Pager events     (placeholder — no memory pressure in this context)
  //   4. Scheduler tick   (cooperative round-robin, every kSchedTickInterval iters)
  //
  // On hosted builds all bare-metal asm compiles away and the loop exits
  // immediately so unit tests can link without MMIO access.
  for (;;) {
#if !defined(__aarch64__) || defined(__APPLE__)
    return;  // hosted: return immediately for test linkage
#endif
    ++s_loop_iters;

    // Priority 2 — serial RX (interrupt source).
    if (pl011_rx_ready()) {
      const int c = pl011_getchar();
      if (c >= 0) {
        ++s_interrupt_count;
        if (c == '\r' || c == '\n') {
          s_line[s_line_len] = '\0';
          pl011_puts("\r\n");
          shell_dispatch(s_line);
          s_line_len = 0;
          pl011_puts("[axion@T81 tier=1]$ ");
        } else if (c == 127 || c == '\b') {
          if (s_line_len > 0) { --s_line_len; pl011_puts("\b \b"); }
        } else if (s_line_len < static_cast<int>(sizeof(s_line)) - 1) {
          s_line[s_line_len++] = static_cast<char>(c);
          pl011_putchar(static_cast<char>(c));
        }
      }
      continue;
    }

    // Priority 3 — pager (no-op placeholder).
    // Priority 4 — fallback poll tick (active before GICv3 IRQs are live, or
    //              every 10 000 iterations as a safety net).
    if (s_timer_irqs == 0 || (s_loop_iters % 10000u) == 0) {
      freestanding_sched_tick();
    }

    // Idle: WFI — wakes on the next timer or serial IRQ.
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("wfi" ::: "memory");
#endif
  }
}
