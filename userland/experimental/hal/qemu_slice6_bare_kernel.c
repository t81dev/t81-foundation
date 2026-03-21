// experimental/ternaryos/hal/qemu_slice6_bare_kernel.c
//
// Post-ExitBootServices bare-metal kernel entry for the QEMU virt AArch64
// machine.  Called from qemu_armv8_efi_stub.c after EFI boot services have
// been terminated.  At this point the UEFI environment is gone; only MMIO
// registers and CPU system registers are available.
//
// Execution steps:
//   1. Write "[axion] bare-metal EL1 kernel entry\n" directly to PL011 MMIO
//      at 0x09000000 — proves raw hardware access after ExitBootServices.
//   2. Perform a minimal GICv3 CPU interface enable (ICC_SRE_EL1 + ICC_PMR_EL1
//      + ICC_IGRPEN1_EL1) so the ARM generic timer PPI can fire.
//   3. Arm the ARM generic timer physical countdown at CNTP_TVAL_EL0 =
//      625 000 counts (10 ms @ 62.5 MHz) with ENABLE=1.
//   4. Write "[axion] timer armed, entering WFI loop\n" to the PL011.
//   5. Enter an infinite WFI loop — each timer IRQ will wake the core.
//
// On hosted macOS / x86-64 builds the AArch64-only blocks compile away to
// no-ops, and the function returns immediately so unit tests can link it.
//
// References: RFC-00B3 §3.9 (QEMU lane), RFC-00B0 §4.4 (ethics-first boot).

#include <stdint.h>

// ── QEMU virt AArch64 memory map ─────────────────────────────────────────────

#define QEMU_PL011_BASE  UINT64_C(0x09000000)

// PL011 register offsets (byte offsets; access as 32-bit words)
#define PL011_DR_OFF  0x000u   // Data Register
#define PL011_FR_OFF  0x018u   // Flag Register
#define PL011_CR_OFF  0x030u   // Control Register

#define PL011_FR_TXFF (1u << 5)   // Transmit FIFO full
#define PL011_CR_UARTEN (1u << 0) // UART enable

// ── MMIO helpers ──────────────────────────────────────────────────────────────

static inline void mmio_write32(uint64_t base, uint32_t off, uint32_t val) {
#if defined(__aarch64__) && !defined(__APPLE__)
  volatile uint32_t* reg = (volatile uint32_t*)(uintptr_t)(base + off);
  *reg = val;
#else
  (void)base; (void)off; (void)val;
#endif
}

static inline uint32_t mmio_read32(uint64_t base, uint32_t off) {
#if defined(__aarch64__) && !defined(__APPLE__)
  const volatile uint32_t* reg = (const volatile uint32_t*)(uintptr_t)(base + off);
  return *reg;
#else
  (void)base; (void)off;
  return 0;
#endif
}

// ── PL011 bare-metal transmit ─────────────────────────────────────────────────

static void bare_pl011_putchar(char c) {
  // Spin until TX FIFO has room.
  while (mmio_read32(QEMU_PL011_BASE, PL011_FR_OFF) & PL011_FR_TXFF) {
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("yield" ::: "memory");
#endif
  }
  mmio_write32(QEMU_PL011_BASE, PL011_DR_OFF, (uint32_t)(unsigned char)c);
}

static void bare_pl011_puts(const char* s) {
  while (*s) {
    bare_pl011_putchar(*s++);
  }
}

// ── Forward declaration of C++ bridge entry ───────────────────────────────────
//
// qemu_cpp_bridge_entry() is defined in qemu_slice6_cpp_bridge.cpp and linked
// into the same BOOTAA64.EFI image.  It provides the T81 governance banner and
// the interactive t81> shell using only language-level C++ (freestanding).
//
// On bare-metal AArch64 qemu_cpp_bridge_entry() never returns.
// On hosted (macOS / x86-64) builds it returns immediately.

extern void qemu_cpp_bridge_entry(void);

// ── Bare-metal kernel entry point ─────────────────────────────────────────────
//
// Purpose: confirm that ExitBootServices succeeded and that EL1 PL011 MMIO
// access works, then hand off to the C++ kernel bridge for the banner + shell.
//
// System-register access notes for QEMU HVF (Apple Hypervisor.framework):
//
//   ICC_SRE_EL1  — trapped: EDK2 does not set ICC_SRE_EL2.SRE before hand-off.
//   CNTP_TVAL_EL0— trapped: EDK2 does not set CNTHCTL_EL2.EL1PCTEN.
//
// On Linux QEMU (TCG / KVM) EDK2 performs the correct EL2 setup and both
// registers are accessible.  GIC and timer init are handled inside the C++
// bridge (qemu_slice6_cpp_bridge.cpp) on that path.

void qemu_bare_kernel_entry(void) {
  // Phase 2 probe: confirm EL1 PL011 MMIO access after ExitBootServices.
  bare_pl011_puts("[axion] bare-metal EL1 kernel entry\r\n");
  bare_pl011_puts("[axion] ExitBootServices complete; handing off to C++ kernel\r\n");

  // Hand off to the freestanding C++ bridge for the T81 banner and shell.
  // On bare-metal AArch64 this call never returns.
  qemu_cpp_bridge_entry();
}
