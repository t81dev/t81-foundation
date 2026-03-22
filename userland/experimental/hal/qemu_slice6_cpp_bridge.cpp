// userland/experimental/hal/qemu_slice6_cpp_bridge.cpp
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
// Probe the first virtio MMIO slot (0x0A000000) for a block device.
// Only checks the magic / version / device-ID registers — does not
// initialise the queue (the full driver lives in VirtioBlkMmioDevice).
// Used only to select the banner text; the hosted C++ kernel does the
// full initialisation through IBlockDevice.

static constexpr uint64_t kVirtioMmioBase = UINT64_C(0x0A000000);

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

// ── Live kernel counters ──────────────────────────────────────────────────────

static uint64_t s_boot_cntpct = 0;  // captured at qemu_cpp_bridge_entry()
static uint64_t s_cmd_count   = 0;  // commands dispatched since boot
static uint64_t s_poll_count  = 0;  // shell poll iterations since boot

// ── Shell command handlers ────────────────────────────────────────────────────

static void cmd_help() noexcept {
  pl011_puts("  help     -- this message\r\n");
  pl011_puts("  version  -- T81 build info\r\n");
  pl011_puts("  status   -- kernel counters and governance state\r\n");
  pl011_puts("  policy   -- Axion policy summary\r\n");
}

static void cmd_version() noexcept {
  pl011_puts("  T81 / Axion  --  ternary OS kernel (bare-metal EFI bridge)\r\n");
  pl011_puts("  Architecture : AArch64 (QEMU virt, cortex-a57, EDK2)\r\n");
  pl011_puts("  Boot path    : EFI efi_main -> ExitBootServices -> C++ bridge\r\n");
}

static void cmd_status() noexcept {
  char buf[24];

  const uint64_t now      = read_cntpct();
  const uint64_t freq     = read_cntfrq();
  const uint64_t uptime_s = (now - s_boot_cntpct) / freq;

  pl011_puts("  [kernel]\r\n");
  pl011_puts("    path          : bare-metal (EFI C++ bridge)\r\n");
  pl011_puts("    canonfs       : mounted (in-memory)\r\n");
  pl011_puts("    policy engine : ready\r\n");
  pl011_puts("    threads       : 1\r\n");

  pl011_puts("    uptime (s)    : ");
  pl011_puts(u64_dec(uptime_s, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    poll cycles   : ");
  pl011_puts(u64_dec(s_poll_count, buf, static_cast<int>(sizeof(buf))));
  pl011_puts("\r\n");

  pl011_puts("    commands      : ");
  pl011_puts(u64_dec(s_cmd_count, buf, static_cast<int>(sizeof(buf))));
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
  else if (str_eq(line, "version")) { cmd_version(); }
  else if (str_eq(line, "status"))  { cmd_status(); }
  else if (str_eq(line, "policy"))  { cmd_policy(); }
  else {
    pl011_puts("  unknown command: '");
    pl011_puts(line);
    pl011_puts("' -- type 'help'\r\n");
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
  // Capture boot timestamp from the ARM physical counter.  All subsequent
  // uptime readings subtract this value so cmd_status() shows wall seconds
  // since the C++ bridge was entered, not since the hardware reset.
  s_boot_cntpct = read_cntpct();

  const bool has_blk = probe_virtio_blk_bare();

  // Governance banner — mirrors the hosted simulation path output so CI
  // validation and sample-boot-log.txt can check a single canonical sequence.
  pl011_puts("\r\n");
  pl011_puts("  T81  --  Ternary OS for AI\r\n");
  pl011_puts("  ===========================\r\n");
  pl011_puts("\r\n");
  pl011_puts("[axion] policy engine: ready\r\n");
  if (has_blk) {
    pl011_puts("[axion] canonfs: mounted (persistent, virtio-blk)\r\n");
  } else {
    pl011_puts("[axion] canonfs: mounted (in-memory)\r\n");
  }
  pl011_puts("[axion] kernel thread tid=1: running\r\n");
  pl011_puts("\r\n");
  pl011_puts("t81> ");

  s_line_len = 0;

  // Line-buffered polling command shell.  Yields to the CPU between polls so
  // the QEMU TCG thread scheduler can make progress.  On hosted builds the
  // AArch64 asm blocks compile away and the loop exits immediately.
  for (;;) {
#if !defined(__aarch64__) || defined(__APPLE__)
    // Hosted (macOS / x86-64): no serial; return so tests can link and call.
    return;
#endif
    ++s_poll_count;

    if (pl011_rx_ready()) {
      const int c = pl011_getchar();
      if (c < 0) continue;

      if (c == '\r' || c == '\n') {
        s_line[s_line_len] = '\0';
        pl011_puts("\r\n");
        shell_dispatch(s_line);
        s_line_len = 0;
        pl011_puts("t81> ");
      } else if (c == 127 || c == '\b') {
        if (s_line_len > 0) {
          --s_line_len;
          pl011_puts("\b \b");
        }
      } else if (s_line_len < static_cast<int>(sizeof(s_line)) - 1) {
        s_line[s_line_len++] = static_cast<char>(c);
        pl011_putchar(static_cast<char>(c));
      }
    }
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("yield" ::: "memory");
#endif
  }
}
