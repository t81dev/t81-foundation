#pragma once

// experimental/ternaryos/dev/pl011_uart.hpp
//
// ARM PL011 UART MMIO driver — minimal transmit path for TernOS console I/O.
//
// On the QEMU virt AArch64 machine the PL011 is mapped at 0x09000000 and
// wired to SPI 33 (GIC INTID 33).  EDK2 initialises it at 115200 8N1 before
// transferring control to the UEFI application, so after ExitBootServices the
// transmit path works without re-initialising the baud-rate divisors.
//
// On hosted (non-bare-metal) builds every MMIO write is compiled out; output
// falls back to writing to the host FILE* set by pl011_set_host_sink().  This
// keeps the driver testable on macOS/Linux without special privileges.
//
// Usage (bare-metal / QEMU):
//   pl011_init(kQemuVirtPl011Base);        // only needed if EDK2 not present
//   pl011_puts(kQemuVirtPl011Base, "ok\n");
//
// Usage (hosted unit tests):
//   pl011_set_host_sink(stdout);
//   pl011_puts(0, "logged to stdout\n");   // base ignored on hosted builds

#include <cstdint>
#include <cstdio>

namespace t81::ternaryos::dev {

/// MMIO base address of the PL011 UART on the QEMU virt AArch64 machine.
inline constexpr uint64_t kQemuVirtPl011Base = 0x0000'0000'0900'0000ULL;

// ── PL011 register offsets ───────────────────────────────────────────────────

inline constexpr uint64_t kPl011RegDR    = 0x000;  ///< Data register (TX/RX)
inline constexpr uint64_t kPl011RegFR    = 0x018;  ///< Flag register
inline constexpr uint64_t kPl011RegIBRD  = 0x024;  ///< Integer baud-rate divisor
inline constexpr uint64_t kPl011RegFBRD  = 0x028;  ///< Fractional baud-rate divisor
inline constexpr uint64_t kPl011RegLCRH  = 0x02C;  ///< Line control register
inline constexpr uint64_t kPl011RegCR    = 0x030;  ///< Control register
inline constexpr uint64_t kPl011RegIMSC  = 0x038;  ///< Interrupt mask set/clear

/// UARTFR bit 4: RX FIFO empty.
inline constexpr uint32_t kPl011FrRXFE = (1u << 4);
/// UARTFR bit 5: TX FIFO full.
inline constexpr uint32_t kPl011FrTXFF = (1u << 5);
/// UARTFR bit 7: TX FIFO empty (all bytes shifted out).
inline constexpr uint32_t kPl011FrTXFE = (1u << 7);

/// UARTCR bit 0: UART enable.
inline constexpr uint32_t kPl011CrUARTEN = (1u << 0);
/// UARTCR bit 8: TX enable.
inline constexpr uint32_t kPl011CrTXE    = (1u << 8);
/// UARTCR bit 9: RX enable.
inline constexpr uint32_t kPl011CrRXE    = (1u << 9);

/// UARTLCR_H bit 4: enable TX/RX FIFOs.
inline constexpr uint32_t kPl011LcrhFEN  = (1u << 4);
/// UARTLCR_H bits [6:5]: word length; 0b11 = 8-bit.
inline constexpr uint32_t kPl011LcrhWlen8 = (3u << 5);

// ── Hosted-build sink ────────────────────────────────────────────────────────

/// On hosted (non-AArch64) builds, redirect all PL011 output to this FILE*.
/// Defaults to nullptr (output is dropped).  Set to stdout or a test FILE*
/// before calling pl011_puts / pl011_putchar.
///
/// On real AArch64 bare-metal this function is a no-op.
void pl011_set_host_sink(FILE* sink) noexcept;

// ── Driver API ───────────────────────────────────────────────────────────────

/// Initialise the PL011 at 115200 8N1 with FIFOs enabled.
///
/// Safe to call after ExitBootServices (no EFI dependency).  On QEMU with EDK2
/// this is typically a no-op because EDK2 already set the baud rate; calling it
/// again is harmless.
///
/// On hosted builds this function is a no-op.
void pl011_init(uint64_t base) noexcept;

/// Transmit a single byte.  Spins until the TX FIFO has room.
/// On hosted builds writes to the host sink (if set) instead of MMIO.
void pl011_putchar(uint64_t base, char c) noexcept;

/// Transmit a NUL-terminated string.
void pl011_puts(uint64_t base, const char* s) noexcept;

/// Wait until the TX FIFO is completely drained (TXFE set).
/// Use before powering off or before ExitBootServices handoff.
void pl011_flush(uint64_t base) noexcept;

/// Return true if at least one byte is waiting in the RX FIFO.
/// On hosted (non-AArch64) builds always returns false.
bool pl011_rx_ready(uint64_t base) noexcept;

/// Read one byte from the RX FIFO without blocking.
/// Returns the byte value [0, 255] if data is available, or -1 if the RX
/// FIFO is empty.  On hosted builds always returns -1.
int pl011_getchar(uint64_t base) noexcept;

}  // namespace t81::ternaryos::dev
