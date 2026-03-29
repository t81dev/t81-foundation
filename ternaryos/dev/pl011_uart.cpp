// experimental/ternaryos/dev/pl011_uart.cpp
//
// ARM PL011 UART MMIO driver.
//
// Bare-metal path: direct volatile 32-bit MMIO reads/writes.
// Hosted path: all MMIO is compiled out; output goes to pl011_host_sink_.

#include "pl011_uart.hpp"

namespace t81::ternaryos::dev {

// ── MMIO helpers (bare-metal only) ──────────────────────────────────────────

#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)

static inline void mmio_write32(uint64_t addr, uint32_t val) noexcept {
  *reinterpret_cast<volatile uint32_t*>(addr) = val;
}

static inline uint32_t mmio_read32(uint64_t addr) noexcept {
  return *reinterpret_cast<const volatile uint32_t*>(addr);
}

static inline bool tx_fifo_full(uint64_t base) noexcept {
  return (mmio_read32(base + kPl011RegFR) & kPl011FrTXFF) != 0;
}

static inline bool tx_fifo_empty(uint64_t base) noexcept {
  return (mmio_read32(base + kPl011RegFR) & kPl011FrTXFE) != 0;
}

#else  // hosted build

static FILE* pl011_host_sink_ = nullptr;

[[maybe_unused]] static inline void mmio_write32(uint64_t, uint32_t) noexcept {}
[[maybe_unused]] static inline uint32_t mmio_read32(uint64_t) noexcept { return 0; }
[[maybe_unused]] static inline bool tx_fifo_full(uint64_t) noexcept { return false; }
[[maybe_unused]] static inline bool tx_fifo_empty(uint64_t) noexcept { return true; }

#endif

// ── Hosted sink ──────────────────────────────────────────────────────────────

void pl011_set_host_sink(FILE* sink) noexcept {
#if !defined(__aarch64__) || defined(__APPLE__) || \
    defined(T81_TERNARYOS_HOSTED_BUILD)
  pl011_host_sink_ = sink;
#else
  (void)sink;
#endif
}

// ── Driver API ───────────────────────────────────────────────────────────────

void pl011_init(uint64_t base) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)
  // Disable UART while reconfiguring.
  mmio_write32(base + kPl011RegCR, 0);

  // Mask all interrupts.
  mmio_write32(base + kPl011RegIMSC, 0);

  // 115200 baud from a 24 MHz UARTCLK (QEMU virt default):
  //   Divisor = 24_000_000 / (16 × 115200) = 13.020833...
  //   IBRD = 13,  FBRD = round(0.020833 × 64) = 1
  mmio_write32(base + kPl011RegIBRD, 13u);
  mmio_write32(base + kPl011RegFBRD,  1u);

  // 8-bit word, FIFOs enabled, 1 stop bit, no parity.
  mmio_write32(base + kPl011RegLCRH, kPl011LcrhFEN | kPl011LcrhWlen8);

  // Enable UART, TX, RX.
  mmio_write32(base + kPl011RegCR, kPl011CrUARTEN | kPl011CrTXE | kPl011CrRXE);
#else
  (void)base;
#endif
}

void pl011_putchar(uint64_t base, char c) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)
  // Spin until there is room in the TX FIFO.
  while (tx_fifo_full(base)) {
    __asm__ volatile("yield");
  }
  mmio_write32(base + kPl011RegDR, static_cast<uint32_t>(static_cast<unsigned char>(c)));
#else
  (void)base;
  if (pl011_host_sink_) {
    fputc(static_cast<unsigned char>(c), pl011_host_sink_);
  }
#endif
}

void pl011_puts(uint64_t base, const char* s) noexcept {
  if (!s) return;
  while (*s) {
    pl011_putchar(base, *s);
    ++s;
  }
}

void pl011_flush(uint64_t base) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)
  // Spin until TX FIFO is empty.
  while (!tx_fifo_empty(base)) {
    __asm__ volatile("yield");
  }
#else
  (void)base;
  if (pl011_host_sink_) fflush(pl011_host_sink_);
#endif
}

bool pl011_rx_ready(uint64_t base) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)
  return (mmio_read32(base + kPl011RegFR) & kPl011FrRXFE) == 0;
#else
  (void)base;
  return false;
#endif
}

int pl011_getchar(uint64_t base) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__) && \
    !defined(T81_TERNARYOS_HOSTED_BUILD)
  if (!pl011_rx_ready(base)) return -1;
  return static_cast<int>(mmio_read32(base + kPl011RegDR) & 0xFFu);
#else
  (void)base;
  return -1;
#endif
}

}  // namespace t81::ternaryos::dev
