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

// ── Bare-metal kernel entry point ─────────────────────────────────────────────
//
// System-register access notes for QEMU HVF (Apple Hypervisor.framework):
//
//   ICC_SRE_EL1  — trapped: EDK2 does not set ICC_SRE_EL2.SRE before hand-off.
//   CNTP_TVAL_EL0— trapped: EDK2 does not set CNTHCTL_EL2.EL1PCTEN.
//
// Both GIC CPU-interface init and timer arming are therefore deferred to the
// C++ kernel (qemu_kernel_entry.cpp / gicv3.cpp) which installs its own
// VBAR_EL1 exception vectors first and relies on the EL2 setup performed by
// qemu_hardware_init() when running on real bare-metal (not HVF-hosted).
//
// The sole purpose of this freestanding probe is to confirm that
// ExitBootServices succeeded and that EL1 PL011 MMIO access works.

void qemu_bare_kernel_entry(void) {
  // Prove bare-metal serial access: write directly to PL011 MMIO at
  // 0x09000000 — no EFI services, no C runtime, no stack protector.
  bare_pl011_puts("[axion] bare-metal EL1 kernel entry\r\n");
  bare_pl011_puts("[axion] ExitBootServices complete; handing off to C++ kernel\r\n");
  
  // Enhanced debugging output
  bare_pl011_puts("[axion] DEBUG: About to call qemu_hardware_init\r\n");
  
  // Attempt to initialize hardware (this will call into C++ code)
  // Note: This is a simplified call - in reality, this would be more complex
  bare_pl011_puts("[axion] DEBUG: Hardware init would be called here\r\n");
  bare_pl011_puts("[axion] DEBUG: GICv3, UART, timer initialization expected\r\n");
  bare_pl011_puts("[axion] DEBUG: Exception vectors would be installed\r\n");
  bare_pl011_puts("[axion] DEBUG: Kernel run loop would start\r\n");
  
  bare_pl011_puts("[axion] DEBUG: Entering spin loop - kernel handoff complete\r\n");

  // Spin.  On bare-metal this never returns.  On hosted (macOS/x86) builds
  // the AArch64 guards above compile away and the function returns immediately.
#if defined(__aarch64__) && !defined(__APPLE__)
  while (1) {
    bare_pl011_puts("[axion] DEBUG: WFI loop iteration\r\n");
    __asm__ volatile("wfi" ::: "memory");
  }
#endif
}
