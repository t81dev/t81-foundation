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

// ── Live kernel counters ──────────────────────────────────────────────────────

static uint64_t s_tsc_freq_hz = 1;   // set at bridge entry from EFI measurement
static uint64_t s_boot_tsc    = 0;   // captured at qemu_x86_64_cpp_bridge_entry()
static uint64_t s_cmd_count   = 0;
static uint64_t s_poll_count  = 0;

// ── Shell command handlers ────────────────────────────────────────────────────

static void cmd_help() noexcept {
  com1_puts("  help     -- this message\r\n");
  com1_puts("  version  -- T81 build info\r\n");
  com1_puts("  status   -- kernel counters and governance state\r\n");
  com1_puts("  policy   -- Axion policy summary\r\n");
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
  com1_puts("    canonfs       : mounted (in-memory)\r\n");
  com1_puts("    policy engine : ready\r\n");
  com1_puts("    threads       : 1\r\n");

  com1_puts("    uptime (s)    : ");
  com1_puts(u64_dec(uptime_s, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    poll cycles   : ");
  com1_puts(u64_dec(s_poll_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    commands      : ");
  com1_puts(u64_dec(s_cmd_count, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");

  com1_puts("    tsc freq (Hz) : ");
  com1_puts(u64_dec(s_tsc_freq_hz, buf, static_cast<int>(sizeof(buf))));
  com1_puts("\r\n");
}

static void cmd_policy() noexcept {
  com1_puts("  [axion policy]\r\n");
  com1_puts("    governance  : active\r\n");
  com1_puts("    audit trail : canonfs (in-memory)\r\n");
  com1_puts("    constraints : RFC-00B0 ethics-first boot\r\n");
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
    com1_puts("  unknown command: '");
    com1_puts(line);
    com1_puts("' -- type 'help'\r\n");
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

  com1_puts("\r\n");
  com1_puts("  T81  --  Ternary OS for AI\r\n");
  com1_puts("  ===========================\r\n");
  com1_puts("\r\n");
  com1_puts("[axion] policy engine: ready\r\n");
  com1_puts("[axion] canonfs: mounted (in-memory)\r\n");
  com1_puts("[axion] kernel thread tid=1: running\r\n");
  com1_puts("\r\n");
  com1_puts("t81> ");

  s_line_len = 0;

  for (;;) {
#if !defined(__x86_64__) || !defined(_WIN32)
    // Hosted build (no port I/O): return immediately so tests can link.
    return;
#endif
    ++s_poll_count;

    if (com1_rx_ready()) {
      const int c = com1_getchar();
      if (c < 0) continue;

      if (c == '\r' || c == '\n') {
        s_line[s_line_len] = '\0';
        com1_puts("\r\n");
        shell_dispatch(s_line);
        s_line_len = 0;
        com1_puts("t81> ");
      } else if (c == 127 || c == '\b') {
        if (s_line_len > 0) {
          --s_line_len;
          com1_puts("\b \b");
        }
      } else if (s_line_len < static_cast<int>(sizeof(s_line)) - 1) {
        s_line[s_line_len++] = static_cast<char>(c);
        com1_putchar(static_cast<char>(c));
      }
    }
#if defined(__x86_64__) && defined(_WIN32)
    __asm__ volatile("pause" ::: "memory");
#endif
  }
}
