// userland/experimental/hal/qemu_x86_64_cpp_bridge.cpp
//
// Freestanding C++ kernel bridge for QEMU x86_64 bare-metal.
//
// Parallel to qemu_slice6_cpp_bridge.cpp (AArch64 / PL011) but targets
// x86_64 with COM1 (16550A, port 0x3F8) for I/O and RDTSC for timing.
//
// Compilation constraints:
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib
//   --target=x86_64-pc-windows-msvc  (PE32+ for OVMF loader)
//
// No dynamic allocation; no C++ runtime; no libcxx headers.
// Only language-level C++ and the freestanding subset of <stdint.h>.
//
// On hosted (Linux / macOS native compiler) builds all I/O is suppressed
// and the function returns immediately so unit tests can link it.
//
// RFC-00B3 §3.10 (x86 lane), RFC-00B0 §4.4 (ethics-first boot).

#include <stdint.h>

// ── COM1 port map ─────────────────────────────────────────────────────────────

static constexpr uint16_t kCom1Base    = 0x3F8u;
static constexpr uint16_t kUartTHR     = 0u;   // Transmit Holding Register
static constexpr uint16_t kUartRBR     = 0u;   // Receive Buffer Register
static constexpr uint16_t kUartLSR     = 5u;   // Line Status Register
static constexpr uint8_t  kLsrTHRE     = 0x20u; // TX Holding Register Empty
static constexpr uint8_t  kLsrDR       = 0x01u; // Data Ready (RX)

// ── x86 port I/O helpers ─────────────────────────────────────────────────────

// Port I/O is available in the EFI freestanding environment (ring 0 after
// ExitBootServices).  On hosted (native Linux/macOS) builds the asm blocks
// are guarded out so the file compiles cleanly for test linkage.

static inline void outb_x86(uint16_t port, uint8_t val) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  // Compiled with --target=x86_64-pc-windows-msvc: EFI freestanding path.
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
#else
  (void)port; (void)val;
#endif
}

static inline uint8_t inb_x86(uint16_t port) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  uint8_t val;
  __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port) : "memory");
  return val;
#else
  (void)port;
  return 0u;
#endif
}

// 32-bit port I/O — used for PCI config space access.
static inline void outl_x86(uint16_t port, uint32_t val) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
#else
  (void)port; (void)val;
#endif
}

static inline uint32_t inl_x86(uint16_t port) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  uint32_t val;
  __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port) : "memory");
  return val;
#else
  (void)port;
  return 0u;
#endif
}

// ── PCI config space — mechanism 1 ───────────────────────────────────────────
// CONFIG_ADDRESS at 0xCF8; CONFIG_DATA at 0xCFC.  Available at ring 0 after
// ExitBootServices on every x86 platform.

static constexpr uint16_t kPciCfgAddr = 0xCF8u;
static constexpr uint16_t kPciCfgData = 0xCFCu;

static uint32_t pci_config_read32(uint8_t bus, uint8_t dev,
                                   uint8_t func, uint8_t off) noexcept {
  const uint32_t addr = (1u << 31)
                      | (static_cast<uint32_t>(bus)  << 16)
                      | (static_cast<uint32_t>(dev)  << 11)
                      | (static_cast<uint32_t>(func) <<  8)
                      | (off & 0xFCu);
  outl_x86(kPciCfgAddr, addr);
  return inl_x86(kPciCfgData);
}

// ── COM1 TX / RX ─────────────────────────────────────────────────────────────

static void com1_putchar(char c) noexcept {
  while (!(inb_x86(kCom1Base + kUartLSR) & kLsrTHRE)) {
#if defined(__x86_64__) && defined(_WIN32)
    __asm__ volatile("pause" ::: "memory");
#endif
  }
  outb_x86(kCom1Base + kUartTHR, static_cast<uint8_t>(c));
}

static void com1_puts(const char* s) noexcept {
  while (*s) com1_putchar(*s++);
}

static bool com1_rx_ready() noexcept {
  return !!(inb_x86(kCom1Base + kUartLSR) & kLsrDR);
}

static int com1_getchar() noexcept {
  if (!com1_rx_ready()) return -1;
  return static_cast<int>(inb_x86(kCom1Base + kUartRBR));
}

// ── Virtio-blk discovery (x86_64) ────────────────────────────────────────────
// Two-path probe:
//   1. MMIO: reads magic/version/device-id at the virtio-mmio slot 1 address.
//      Succeeds when QEMU is launched with an explicit virtio-mmio bus at
//      0x0A000200 (future improvement; slot 0 = 0x0A000000 is the boot disk).
//   2. PCI scan: counts virtio-blk-pci devices on bus 0 via config space.
//      QEMU q35 creates virtio-blk-pci for -drive if=virtio (PCI transport).
//      Two devices = boot disk + CanonFS disk → persistent CanonFS available.

// MMIO base for virtio slot 1 (dedicated CanonFS store; slot 0 = boot disk).
static constexpr uint64_t kVirtioMmioBase = UINT64_C(0x0A000200);

static inline uint32_t mmio_rd32_x86(uint64_t base, uint32_t off) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  return *reinterpret_cast<const volatile uint32_t*>(
      static_cast<uintptr_t>(base + off));
#else
  (void)base; (void)off;
  return 0u;
#endif
}

// Virtio PCI IDs (vendor 0x1AF4 = Red Hat / virtio).
static constexpr uint16_t kVirtioPciVendor    = 0x1AF4u;
static constexpr uint16_t kVirtioBlkPciLegacy = 0x1001u;  // virtio < 1.0
static constexpr uint16_t kVirtioBlkPciModern = 0x1042u;  // virtio 1.0+

// Scan PCI bus 0 (devices 0–31, function 0) and count virtio-blk devices.
static int count_virtio_blk_pci() noexcept {
  int found = 0;
  for (uint8_t dev = 0u; dev < 32u; ++dev) {
    const uint32_t id = pci_config_read32(0u, dev, 0u, 0x00u);
    if (id == 0xFFFFFFFFu) continue;  // empty slot
    const uint16_t vendor = static_cast<uint16_t>(id & 0xFFFFu);
    const uint16_t device = static_cast<uint16_t>(id >> 16);
    if (vendor == kVirtioPciVendor
        && (device == kVirtioBlkPciLegacy || device == kVirtioBlkPciModern)) {
      ++found;
    }
  }
  return found;
}

// Returns true if a CanonFS block device is detected (not the boot disk).
// On AArch64 virt: virtio-mmio probe succeeds at kVirtioMmioBase.
// On x86_64 q35:  PCI scan finds ≥2 virtio-blk-pci devices.
static bool probe_virtio_blk_bare() noexcept {
  // Path 1 — MMIO (virt machine or future explicit virtio-mmio bus on q35).
  const uint32_t magic = mmio_rd32_x86(kVirtioMmioBase, 0x000u);
  const uint32_t ver   = mmio_rd32_x86(kVirtioMmioBase, 0x004u);
  const uint32_t devid = mmio_rd32_x86(kVirtioMmioBase, 0x008u);
  if (magic == 0x74726976u && ver == 2u && devid == 2u) return true;

  // Path 2 — PCI scan (q35 with virtio-blk-pci).
  // ≥2 virtio-blk devices → boot disk (device N) + CanonFS disk (device N+1).
  return count_virtio_blk_pci() >= 2;
}

// ── Virtio-blk I/O probe (x86_64, legacy PCI I/O-port transport) ─────────────
// Performs a full virtio-blk-pci legacy initialisation of the second
// virtio-blk device (index 1 — the CanonFS store), then:
//   1. READ  LBA 0 → verify CST1 magic
//   2. WRITE known pattern to LBA 1
//   3. READ  LBA 1 → byte-compare round-trip
// Legacy virtio PCI register map (BAR0, I/O space, virtio 0.9.5):
//   base+0x00: DeviceFeatures (R,32)   base+0x04: GuestFeatures (W,32)
//   base+0x08: QueueAddress  (W,32)    base+0x0C: QueueSize (R,16)
//   base+0x0E: QueueSelect   (W,16)    base+0x10: QueueNotify (W,16)
//   base+0x12: DeviceStatus  (RW,8)    base+0x13: ISRStatus (R,8)
// Queue memory layout (QUEUE_SIZE=4, align=4096):
//   [0,   64): Descriptor table (4×16 bytes)
//   [64,  76): Available ring   (flags[2]+idx[2]+ring[4×2])
//   [4096, 4132): Used ring     (flags[2]+idx[2]+ring[4×8])

static constexpr int kVQSz = 4;

static inline void outw_x86(uint16_t port, uint16_t val) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
#else
  (void)port; (void)val;
#endif
}
static inline uint16_t inw_x86(uint16_t port) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  uint16_t v;
  __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(port) : "memory");
  return v;
#else
  (void)port; return 0u;
#endif
}
static inline void x86_mfence() noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("mfence" ::: "memory");
#endif
}

struct LVirtqDesc     { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; };
struct LVirtqAvail    { uint16_t flags; uint16_t idx; uint16_t ring[kVQSz]; };
struct LVirtqUsedElem { uint32_t id; uint32_t len; };
struct LVirtqUsed     { uint16_t flags; uint16_t idx; LVirtqUsedElem ring[kVQSz]; };
struct LVirtioBlkReq  { uint32_t type; uint32_t reserved; uint64_t sector; };

// Legacy queue: contiguous 4096-aligned buffer.
// Available ring immediately after descriptor table (offset 64).
// Used ring at offset 4096.
alignas(4096) static uint8_t    s_lvq_buf[8192];
alignas(16)   static LVirtioBlkReq s_lblk_req;
alignas(512)  static uint8_t    s_lsector_buf[512];
static uint8_t s_lblk_status;

static constexpr uint16_t kLVD_Next  = 0x0001u;
static constexpr uint16_t kLVD_Write = 0x0002u;
static constexpr uint32_t kLBlkIn   = 0u;
static constexpr uint32_t kLBlkOut  = 1u;

// Find the I/O port base (BAR0 & ~3) of the N-th virtio-blk-pci device
// (0-indexed).  Returns 0 if not found or BAR0 is not an I/O BAR.
static uint16_t find_nth_vblk_iobase(int n) noexcept {
  int found = 0;
  for (uint8_t dev = 0u; dev < 32u; ++dev) {
    const uint32_t id = pci_config_read32(0u, dev, 0u, 0x00u);
    if (id == 0xFFFFFFFFu) continue;
    const uint16_t vendor = static_cast<uint16_t>(id & 0xFFFFu);
    const uint16_t device = static_cast<uint16_t>(id >> 16);
    if (vendor == kVirtioPciVendor &&
        (device == kVirtioBlkPciLegacy || device == kVirtioBlkPciModern)) {
      if (found == n) {
        const uint32_t bar0 = pci_config_read32(0u, dev, 0u, 0x10u);
        return (bar0 & 1u) ? static_cast<uint16_t>(bar0 & ~3u) : 0u;
      }
      ++found;
    }
  }
  return 0u;
}

static bool lvblk_do_io(uint16_t base, uint32_t type, uint64_t lba) noexcept {
  auto* desc  = reinterpret_cast<LVirtqDesc*>(&s_lvq_buf[0]);
  auto* avail = reinterpret_cast<LVirtqAvail*>(&s_lvq_buf[kVQSz * 16]);
  auto* used  = reinterpret_cast<LVirtqUsed*>(&s_lvq_buf[4096]);

  s_lblk_req.type     = type;
  s_lblk_req.reserved = 0u;
  s_lblk_req.sector   = lba;
  s_lblk_status       = 0xFFu;

  desc[0] = { reinterpret_cast<uint64_t>(&s_lblk_req),
              static_cast<uint32_t>(sizeof(s_lblk_req)),
              kLVD_Next, 1u };
  desc[1] = { reinterpret_cast<uint64_t>(s_lsector_buf), 512u,
              static_cast<uint16_t>(kLVD_Next |
                (type == kLBlkIn ? kLVD_Write : 0u)), 2u };
  desc[2] = { reinterpret_cast<uint64_t>(&s_lblk_status), 1u,
              kLVD_Write, 0u };

  const uint16_t old_used = used->idx;
  avail->ring[avail->idx % kVQSz] = 0u;
  x86_mfence();
  avail->idx = static_cast<uint16_t>(avail->idx + 1u);
  x86_mfence();
  outw_x86(static_cast<uint16_t>(base + 0x10u), 0u);  // QueueNotify queue 0
  x86_mfence();

  for (uint32_t i = 0u; i < 2000000u; ++i) {
    x86_mfence();
    if (used->idx != old_used) break;
#if defined(__x86_64__) && defined(_WIN32)
    __asm__ volatile("pause" ::: "memory");
#endif
  }
  return (used->idx != old_used) && (s_lblk_status == 0u);
}

static void canonfs_io_probe_x86() noexcept {
  // Find I/O base of the second virtio-blk device (index 1 = CanonFS store).
  const uint16_t base = find_nth_vblk_iobase(1);
  if (base == 0u) {
    com1_puts("[axion] canonfs: I/O probe FAIL (no virtio-blk I/O BAR)\r\n");
    return;
  }

  // Reset → ACK → DRIVER.
  outb_x86(static_cast<uint16_t>(base + 0x12u), 0u);   // reset
  x86_mfence();
  outb_x86(static_cast<uint16_t>(base + 0x12u), 0x01u); // ACK
  outb_x86(static_cast<uint16_t>(base + 0x12u), 0x03u); // ACK | DRIVER
  outl_x86(static_cast<uint16_t>(base + 0x04u), 0u);    // no guest features

  // Queue 0 setup: select, read size, write address (page frame number).
  outw_x86(static_cast<uint16_t>(base + 0x0Eu), 0u);    // QueueSelect = 0
  const uint16_t qsz = inw_x86(static_cast<uint16_t>(base + 0x0Cu));
  if (qsz < static_cast<uint16_t>(kVQSz)) {
    com1_puts("[axion] canonfs: I/O probe FAIL (queue too small)\r\n");
    return;
  }
  const uint32_t pfn = static_cast<uint32_t>(
      reinterpret_cast<uint64_t>(s_lvq_buf) >> 12);
  outl_x86(static_cast<uint16_t>(base + 0x08u), pfn);   // QueueAddress (PFN)
  outb_x86(static_cast<uint16_t>(base + 0x12u), 0x07u); // ACK|DRIVER|DRIVER_OK
  x86_mfence();

  // LBA 0 READ → check CST1 magic.
  for (int i = 0; i < 512; ++i) s_lsector_buf[i] = 0u;
  if (!lvblk_do_io(base, kLBlkIn, 0u)) {
    com1_puts("[axion] canonfs: I/O probe FAIL (LBA0 read error)\r\n");
    return;
  }
  if (s_lsector_buf[0] != 'C' || s_lsector_buf[1] != 'S' ||
      s_lsector_buf[2] != 'T' || s_lsector_buf[3] != '1') {
    com1_puts("[axion] canonfs: I/O probe FAIL (LBA0 bad magic)\r\n");
    return;
  }

  // LBA 1 WRITE known pattern.
  for (int i = 0; i < 512; ++i)
    s_lsector_buf[i] = static_cast<uint8_t>(0xA5u ^ static_cast<uint8_t>(i));
  if (!lvblk_do_io(base, kLBlkOut, 1u)) {
    com1_puts("[axion] canonfs: I/O probe FAIL (LBA1 write error)\r\n");
    return;
  }

  // LBA 1 READ back and verify.
  for (int i = 0; i < 512; ++i) s_lsector_buf[i] = 0u;
  if (!lvblk_do_io(base, kLBlkIn, 1u)) {
    com1_puts("[axion] canonfs: I/O probe FAIL (LBA1 read-back error)\r\n");
    return;
  }
  for (int i = 0; i < 512; ++i) {
    if (s_lsector_buf[i] !=
        static_cast<uint8_t>(0xA5u ^ static_cast<uint8_t>(i))) {
      com1_puts("[axion] canonfs: I/O probe FAIL (LBA1 mismatch)\r\n");
      return;
    }
  }

  com1_puts("[axion] canonfs: I/O probe OK"
            " (LBA0 magic=CST1, LBA1 round-trip pass)\r\n");
}

// ── RDTSC timer ──────────────────────────────────────────────────────────────
// The TSC frequency (Hz) is measured by the EFI stub via BS->Stall before
// ExitBootServices and passed in as tsc_freq_hz.  RDTSC is available from
// any privilege level after ExitBootServices on QEMU q35.

static inline uint64_t rdtsc() noexcept {
#if defined(__x86_64__)
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
#else
  return 0;
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

// Render uint64_t as decimal.  bufsz must be >= 21.
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

// ── Freestanding thread table ─────────────────────────────────────────────────

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

static uint64_t s_tsc_freq_hz     = 1;   // set at bridge entry
static uint64_t s_boot_tsc        = 0;
static uint64_t s_cmd_count       = 0;
static uint64_t s_loop_iters      = 0;
static uint64_t s_tick_count      = 0;
static uint64_t s_sched_switches  = 0;
static uint64_t s_interrupt_count = 0;  // serial RX events
static uint64_t s_timer_irqs      = 0;  // hardware timer IRQ count
static bool     s_has_blk         = false;
static bool     s_lapic_armed     = false;  // true = LAPIC 100Hz; false = PIT fallback

// ── Hardware-timer IRQ callback ───────────────────────────────────────────────
// Called from axion_timer_stub_x86_64() (qemu_x86_64_bridge_irq.cpp) on
// every PIT channel 0 tick (~100Hz).  Must be async-signal-safe.

static constexpr uint64_t kSchedTickInterval = 500u;

extern "C" void bridge_timer_irq_tick_x86() noexcept {
  ++s_timer_irqs;
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
// Returns true → LAPIC timer armed (IDT 0x40); false → PIT fallback (IDT 0x20).
extern "C" bool bridge_hw_init_x86_64(uint64_t tsc_freq_hz) noexcept;

// ── Freestanding scheduler tick ───────────────────────────────────────────────

static void freestanding_sched_tick() noexcept {
  ++s_tick_count;
  if (s_thread_count <= 1 || (s_tick_count % kSchedTickInterval) != 0) {
    ++s_threads[s_current_thread].tick_count;
    return;
  }
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
  com1_puts("  help     -- this message\r\n");
  com1_puts("  uname    -- system identity (RFC-00B9 §8.3)\r\n");
  com1_puts("  version  -- T81 build info\r\n");
  com1_puts("  status   -- kernel counters and governance state\r\n");
  com1_puts("  threads  -- thread table (tid, state, ticks)\r\n");
  com1_puts("  sched    -- scheduler counters (loop iters, ticks, switches)\r\n");
  com1_puts("  policy   -- Axion policy summary\r\n");
}

static void cmd_uname() noexcept {
  com1_puts("  T81 TernaryOS 1.0 x86_64 axion-kernel (bare-metal EFI)\r\n");
}

static void cmd_version() noexcept {
  com1_puts("  T81 / Axion  --  ternary OS kernel (bare-metal EFI bridge)\r\n");
  com1_puts("  Architecture : x86_64 (QEMU q35, OVMF)\r\n");
  com1_puts("  Boot path    : EFI efi_main -> ExitBootServices -> C++ bridge\r\n");
}

static void cmd_status() noexcept {
  char buf[24];

  const uint64_t now      = rdtsc();
  const uint64_t uptime_s = (now - s_boot_tsc) / s_tsc_freq_hz;

  com1_puts("  [kernel]\r\n");
  com1_puts("    path          : bare-metal (EFI C++ bridge, x86_64)\r\n");
  if (s_has_blk) {
    com1_puts("    canonfs       : mounted (persistent, virtio-blk)\r\n");
  } else {
    com1_puts("    canonfs       : mounted (in-memory)\r\n");
  }
  com1_puts("    policy engine : ready\r\n");

  com1_puts("    threads       : ");
  com1_puts(u64_dec(static_cast<uint64_t>(s_thread_count), buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    uptime (s)    : ");
  com1_puts(u64_dec(uptime_s, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    loop_iters    : ");
  com1_puts(u64_dec(s_loop_iters, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    tick_count    : ");
  com1_puts(u64_dec(s_tick_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    sched switches: ");
  com1_puts(u64_dec(s_sched_switches, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    interrupts    : ");
  com1_puts(u64_dec(s_interrupt_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    commands      : ");
  com1_puts(u64_dec(s_cmd_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    tsc freq (Hz) : ");
  com1_puts(u64_dec(s_tsc_freq_hz, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
}

static void cmd_threads() noexcept {
  char buf[24];
  com1_puts("  [threads]\r\n");
  for (int i = 0; i < s_thread_count; ++i) {
    const FsThread& t = s_threads[i];
    com1_puts("    tid=");
    com1_puts(u64_dec(t.tid, buf, static_cast<int>(sizeof(buf))));
    switch (t.state) {
      case FsThreadState::Running: com1_puts("  Running"); break;
      case FsThreadState::Ready:   com1_puts("  Ready  "); break;
      case FsThreadState::Blocked: com1_puts("  Blocked"); break;
      default:                     com1_puts("  Empty  "); break;
    }
    com1_puts("  ticks=");
    com1_puts(u64_dec(t.tick_count, buf, static_cast<int>(sizeof(buf))));
    com1_puts("\r\n");
  }
  com1_puts("    count=");
  com1_puts(u64_dec(static_cast<uint64_t>(s_thread_count), buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
}

static void cmd_sched() noexcept {
  char buf[24];
  com1_puts("  [scheduler]\r\n");
  if (s_lapic_armed) {
    com1_puts("    model        : preemptive (LAPIC, 100Hz, IDT 0x40)\r\n");
  } else {
    com1_puts("    model        : preemptive (PIT ch0, 100Hz, IDT 0x20)\r\n");
  }
  com1_puts("    loop_iters   : ");
  com1_puts(u64_dec(s_loop_iters, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
  com1_puts("    tick_count   : ");
  com1_puts(u64_dec(s_tick_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
  com1_puts("    timer_irqs   : ");
  com1_puts(u64_dec(s_timer_irqs, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
  com1_puts("    switches     : ");
  com1_puts(u64_dec(s_sched_switches, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
  com1_puts("    serial_rx    : ");
  com1_puts(u64_dec(s_interrupt_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
  com1_puts("    tick_interval: ");
  com1_puts(u64_dec(kSchedTickInterval, buf, static_cast<int>(sizeof(buf))));
  com1_puts(" hw ticks\r\n");
}

static void cmd_policy() noexcept {
  com1_puts("  [axion policy]\r\n");
  com1_puts("    governance  : active\r\n");
  if (s_has_blk) {
    com1_puts("    audit trail : canonfs (persistent, virtio-blk)\r\n");
  } else {
    com1_puts("    audit trail : canonfs (in-memory)\r\n");
  }
  com1_puts("    constraints : RFC-00B0 ethics-first boot\r\n");
}

static void shell_dispatch(const char* line) noexcept {
  line = str_trim(line);
  if (*line == '\0') return;

  ++s_cmd_count;

  if      (str_eq(line, "help"))    { cmd_help(); }
  else if (str_eq(line, "uname"))   { cmd_uname(); }
  else if (str_eq(line, "version")) { cmd_version(); }
  else if (str_eq(line, "status"))  { cmd_status(); }
  else if (str_eq(line, "threads")) { cmd_threads(); }
  else if (str_eq(line, "sched"))   { cmd_sched(); }
  else if (str_eq(line, "policy"))  { cmd_policy(); }
  else {
    com1_puts("  [axion] ShellExec: Deny -- '");
    com1_puts(line);
    com1_puts("' not in builtin table\r\n");
  }
}

// ── Static shell buffer ───────────────────────────────────────────────────────

static char s_line[64];
static int  s_line_len;

// ── Bridge entry point ────────────────────────────────────────────────────────
//
// Called from qemu_x86_64_bare_kernel_entry() (C linkage) with the TSC
// frequency measured by the EFI stub before ExitBootServices.
// Never returns on x86_64 bare-metal; returns immediately on hosted builds.

extern "C" void qemu_x86_64_cpp_bridge_entry(uint64_t tsc_freq_hz) noexcept {
  s_tsc_freq_hz = tsc_freq_hz > 0 ? tsc_freq_hz : 1;
  s_boot_tsc    = rdtsc();

  // Register kernel thread (tid=1) in the freestanding thread table.
  s_threads[0] = FsThread{1u, FsThreadState::Running, 0u};
  s_thread_count   = 1;
  s_current_thread = 0;

  s_has_blk = probe_virtio_blk_bare();

  com1_puts("\r\n");
  com1_puts("  T81  --  Ternary OS for AI\r\n");
  com1_puts("  ===========================\r\n");
  com1_puts("\r\n");
  com1_puts("[axion] policy engine: ready\r\n");
  if (s_has_blk) {
    com1_puts("[axion] canonfs: mounted (persistent, virtio-blk)\r\n");
    canonfs_io_probe_x86();
  } else {
    com1_puts("[axion] canonfs: mounted (in-memory)\r\n");
  }
  com1_puts("[axion] kernel thread tid=1: running\r\n");
  com1_puts("[axion] event loop: priority dispatch (interrupt > pager > sched)\r\n");

  // Wire hardware timer interrupts: LAPIC (preferred) or 8259 PIC + PIT (fallback).
  s_lapic_armed = bridge_hw_init_x86_64(s_tsc_freq_hz);
  if (s_lapic_armed) {
    com1_puts("[axion] hw timer: LAPIC 100Hz armed (IDT 0x40)\r\n");
  } else {
    com1_puts("[axion] hw timer: PIT ch0 100Hz armed (IDT 0x20)\r\n");
  }
  com1_puts("[axion] t81sh: ready (principal=axion, tier=1)\r\n");

  com1_puts("\r\n");
  com1_puts("[axion@T81 tier=1]$ ");

  s_line_len = 0;

  // ── Priority-dispatch event loop ─────────────────────────────────────────────
  // Mirrors axion_kernel_step() priority order:
  //   1. Fault queue      (placeholder)
  //   2. Interrupt source (COM1 RX ≡ hardware interrupt)
  //   3. Pager events     (placeholder)
  //   4. Scheduler tick   (cooperative round-robin, every kSchedTickInterval iters)
  for (;;) {
#if !defined(__x86_64__) || !defined(_WIN32)
    return;  // hosted: return immediately for test linkage
#endif
    ++s_loop_iters;

    // Priority 2 — serial RX (interrupt source).
    if (com1_rx_ready()) {
      const int c = com1_getchar();
      if (c >= 0) {
        ++s_interrupt_count;
        if (c == '\r' || c == '\n') {
          s_line[s_line_len] = '\0';
          com1_puts("\r\n");
          shell_dispatch(s_line);
          s_line_len = 0;
          com1_puts("[axion@T81 tier=1]$ ");
        } else if (c == 127 || c == '\b') {
          if (s_line_len > 0) { --s_line_len; com1_puts("\b \b"); }
        } else if (s_line_len < static_cast<int>(sizeof(s_line)) - 1) {
          s_line[s_line_len++] = static_cast<char>(c);
          com1_putchar(static_cast<char>(c));
        }
      }
      continue;
    }

    // Priority 3 — pager (no-op placeholder).
    // Priority 4 — fallback poll tick (active before PIT IRQs fire, or every
    //              10 000 iterations as a safety net).
    if (s_timer_irqs == 0 || (s_loop_iters % 10000u) == 0) {
      freestanding_sched_tick();
    }

    // Idle: HLT — wakes on the next timer (PIT IRQ 0x20) or COM1 RX event.
#if defined(__x86_64__) && defined(_WIN32)
    __asm__ volatile("hlt" ::: "memory");
#endif
  }
}
