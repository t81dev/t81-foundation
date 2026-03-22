// userland/experimental/hal/qemu_x86_64_bare_kernel.c
//
// Post-ExitBootServices bare-metal entry for QEMU x86_64 (OVMF slice6).
//
// Responsibilities:
//   1. Initialise COM1 (16550A UART, 0x3F8, 115200 8N1) for serial output.
//   2. Emit phase-2 diagnostic messages directly to COM1.
//   3. Hand control to the freestanding C++ bridge.
//
// Freestanding C only.  No libc, no libstdc++, no firmware services.
// Compiled: clang --target=x86_64-pc-windows-msvc -ffreestanding -nostdlib

#include <stdint.h>

// ── COM1 (16550A UART) port map ───────────────────────────────────────────────

#define COM1_BASE  0x3F8u

// Register offsets from base (DLAB=0)
#define UART_THR  0u   // Transmit Holding Register (write)
#define UART_RBR  0u   // Receive Buffer Register   (read)
#define UART_IER  1u   // Interrupt Enable Register
#define UART_FCR  2u   // FIFO Control Register     (write)
#define UART_LCR  3u   // Line Control Register
#define UART_MCR  4u   // Modem Control Register
#define UART_LSR  5u   // Line Status Register

// Register offsets from base (DLAB=1 — set by LCR bit 7)
#define UART_DLL  0u   // Divisor Latch Low
#define UART_DLH  1u   // Divisor Latch High

// LSR bits
#define UART_LSR_DR    0x01u   // Data Ready (RX)
#define UART_LSR_THRE  0x20u   // TX Holding Register Empty

// ── x86 port I/O helpers ─────────────────────────────────────────────────────

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
  uint8_t val;
  __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port) : "memory");
  return val;
}

// ── COM1 initialisation ───────────────────────────────────────────────────────

static void com1_init(void) {
  outb(COM1_BASE + UART_IER, 0x00u);  // Disable all interrupts
  outb(COM1_BASE + UART_LCR, 0x80u);  // Enable DLAB to set baud divisor
  outb(COM1_BASE + UART_DLL, 0x01u);  // Divisor = 1  →  115200 baud
  outb(COM1_BASE + UART_DLH, 0x00u);
  outb(COM1_BASE + UART_LCR, 0x03u);  // 8-N-1 (clear DLAB)
  outb(COM1_BASE + UART_FCR, 0xC7u);  // Enable FIFO, clear, 14-byte trigger
  outb(COM1_BASE + UART_MCR, 0x0Bu);  // DTR + RTS + OUT2
}

static void com1_putchar(char c) {
  while (!(inb(COM1_BASE + UART_LSR) & UART_LSR_THRE)) {
    __asm__ volatile("pause" ::: "memory");
  }
  outb(COM1_BASE + UART_THR, (uint8_t)c);
}

static void com1_puts(const char* s) {
  while (*s) com1_putchar(*s++);
}

// ── Forward declaration (qemu_x86_64_cpp_bridge.cpp) ─────────────────────────

void qemu_x86_64_cpp_bridge_entry(uint64_t tsc_freq_hz);

// ── Bare-metal entry ──────────────────────────────────────────────────────────

void qemu_x86_64_bare_kernel_entry(uint64_t tsc_freq_hz) {
  // COM1 must be initialised before any serial output.
  com1_init();

  com1_puts("[axion] bare-metal x86_64 entry\r\n");
  com1_puts("[axion] ExitBootServices complete; handing off to C++ kernel\r\n");

  // Hand control to the freestanding C++ bridge.
  qemu_x86_64_cpp_bridge_entry(tsc_freq_hz);

  // Should not return.
  for (;;) { __asm__ volatile("hlt"); }
}
