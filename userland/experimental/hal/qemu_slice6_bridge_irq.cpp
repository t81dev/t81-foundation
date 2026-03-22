// userland/experimental/hal/qemu_slice6_bridge_irq.cpp
//
// Freestanding AArch64 hardware-interrupt wiring for the QEMU slice6 EFI bridge.
//
// Linked into BOOTAA64.EFI alongside:
//   aarch64_exception_vectors.S  — vector table; axion_irq_entry saves regs
//                                  and calls axion_irq_handler_aarch64()
//   qemu_slice6_cpp_bridge.cpp   — provides bridge_timer_irq_tick()
//
// Exported symbols:
//   bridge_hw_init_aarch64()      — call once from qemu_cpp_bridge_entry()
//                                   after the governance banner is printed.
//                                   Installs VBAR_EL1, brings up GICv3,
//                                   enables PPI 30 (ARM physical timer at
//                                   ~100Hz), and clears DAIF.I.
//   axion_irq_handler_aarch64()   — called by axion_irq_entry (assembler);
//                                   acknowledges GICv3, reloads the timer,
//                                   calls bridge_timer_irq_tick(), issues EOI.
//
// Compilation constraints (same as qemu_slice6_cpp_bridge.cpp):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=aarch64-pc-windows-msvc
//
// Only <stdint.h> is used; no C++ runtime, no hosted headers.

#include <stdint.h>

// ── Bridge callback (defined in qemu_slice6_cpp_bridge.cpp) ──────────────────

/// Called from the IRQ handler every timer tick (~100Hz).
extern "C" void bridge_timer_irq_tick() noexcept;

// ── GICv3 MMIO helpers ────────────────────────────────────────────────────────

#if defined(__aarch64__) && !defined(__APPLE__)

static inline void gicd_write32(uint64_t base, uint32_t off,
                                 uint32_t val) noexcept {
  *reinterpret_cast<volatile uint32_t*>(
      static_cast<uintptr_t>(base + off)) = val;
}

static inline uint32_t gicd_read32(uint64_t base, uint32_t off) noexcept {
  return *reinterpret_cast<const volatile uint32_t*>(
      static_cast<uintptr_t>(base + off));
}

// ── GICv3 system-register CPU interface (ICC_*_EL1) ──────────────────────────

static inline void icc_sre_write(uint64_t v) noexcept {
  __asm__ volatile("msr icc_sre_el1, %0\n\tisb" :: "r"(v) : "memory");
}

static inline void icc_pmr_write(uint64_t v) noexcept {
  __asm__ volatile("msr icc_pmr_el1, %0\n\tisb" :: "r"(v) : "memory");
}

static inline void icc_igrpen1_write(uint64_t v) noexcept {
  __asm__ volatile("msr icc_igrpen1_el1, %0\n\tisb" :: "r"(v) : "memory");
}

static inline uint64_t icc_iar1_read() noexcept {
  uint64_t v;
  __asm__ volatile("mrs %0, icc_iar1_el1" : "=r"(v));
  return v;
}

static inline void icc_eoir1_write(uint64_t v) noexcept {
  __asm__ volatile("msr icc_eoir1_el1, %0" :: "r"(v) : "memory");
}

// ── ARM generic timer (physical, EL0-accessible) ─────────────────────────────

static inline void cntp_tval_write(uint32_t tval) noexcept {
  __asm__ volatile("msr cntp_tval_el0, %0\n\tisb"
                   :: "r"(static_cast<uint64_t>(tval)) : "memory");
}

static inline void cntp_ctl_write(uint64_t ctl) noexcept {
  __asm__ volatile("msr cntp_ctl_el0, %0\n\tisb" :: "r"(ctl) : "memory");
}

#else  // hosted / non-AArch64 — all hardware access is a no-op

static inline void gicd_write32(uint64_t, uint32_t, uint32_t) noexcept {}
static inline uint32_t gicd_read32(uint64_t, uint32_t) noexcept { return 0u; }
static inline void icc_sre_write(uint64_t) noexcept {}
static inline void icc_pmr_write(uint64_t) noexcept {}
static inline void icc_igrpen1_write(uint64_t) noexcept {}
static inline uint64_t icc_iar1_read() noexcept { return 1023u; }  // spurious
static inline void icc_eoir1_write(uint64_t) noexcept {}
static inline void cntp_tval_write(uint32_t) noexcept {}
static inline void cntp_ctl_write(uint64_t) noexcept {}

#endif

// ── GICv3 / timer constants ───────────────────────────────────────────────────

// QEMU virt AArch64 addresses (qemu/hw/arm/virt.c).
static constexpr uint64_t kGicDistBase   = UINT64_C(0x0800'0000);
static constexpr uint64_t kGicRedistBase = UINT64_C(0x080A'0000);

// GICD register offsets.
static constexpr uint32_t kGicdCtlr       = 0x0000u;
static constexpr uint32_t kGicdCtlrAreNs  = 1u << 4;  // Affinity Routing NS
static constexpr uint32_t kGicdCtlrGrp1Ns = 1u << 1;  // EnableGrp1NS
static constexpr uint32_t kGicdCtlrRwp    = 1u << 31; // Register Write Pending

// GICR: LP frame at kGicRedistBase; SGI frame at kGicRedistBase + 0x10000.
static constexpr uint32_t kGicrWaker       = 0x0014u;
static constexpr uint32_t kGicrWakerSleep  = 1u << 1;  // ProcessorSleep
static constexpr uint32_t kGicrWakerChild  = 1u << 2;  // ChildrenAsleep
static constexpr uint32_t kGicrSgiOff      = 0x1'0000u; // SGI frame offset

// Within the SGI frame:
static constexpr uint32_t kGicrIgroupr0    = kGicrSgiOff + 0x0080u;
static constexpr uint32_t kGicrIsenabler0  = kGicrSgiOff + 0x0100u;
static constexpr uint32_t kGicrIpriorityrN = kGicrSgiOff + 0x0400u;

// ARM physical timer PPI:
//   INTID 30 = physical timer (PPI, private to each CPU).
static constexpr uint32_t kTimerIntid  = 30u;
static constexpr uint32_t kGicSpurious = 1023u;

// Timer reload: ~10 ms at QEMU's 62.5 MHz generic timer clock (62500000 Hz).
//   62 500 000 / 100 = 625 000 counts per 10 ms tick.
static constexpr uint32_t kTimerPeriod = 625'000u;

// ── Exception vector table symbol (aarch64_exception_vectors.S) ──────────────

extern "C" uint8_t axion_exception_vector_base[];

// ── Debug serial output (diagnosis only) ────────────────────────────────────
// Write a single character directly to PL011 DR at 0x09000000.
// No FIFO-full check — only used to emit step markers from within
// bridge_hw_init_aarch64() to locate hang points.

static inline void dbg_char(char c) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  // Spin on TX FIFO full (FR bit 5) before writing.
  const uint64_t pl011 = UINT64_C(0x09000000);
  while (*reinterpret_cast<const volatile uint32_t*>(
             static_cast<uintptr_t>(pl011 + 0x018u)) & (1u << 5u)) {
    __asm__ volatile("yield" ::: "memory");
  }
  *reinterpret_cast<volatile uint32_t*>(
      static_cast<uintptr_t>(pl011)) = static_cast<uint32_t>(c);
#else
  (void)c;
#endif
}

static inline void dbg_puts(const char* s) noexcept {
  while (*s) dbg_char(*s++);
}

// ── GICv3 distributor RWP spin ────────────────────────────────────────────────

static void gicd_wait_rwp() noexcept {
  for (int i = 0; i < 1000; ++i) {
    if (!(gicd_read32(kGicDistBase, kGicdCtlr) & kGicdCtlrRwp)) return;
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("isb");
#endif
  }
}

static void gicr_wait_children() noexcept {
  for (int i = 0; i < 1000; ++i) {
    if (!(gicd_read32(kGicRedistBase, kGicrWaker) & kGicrWakerChild)) return;
#if defined(__aarch64__) && !defined(__APPLE__)
    __asm__ volatile("isb");
#endif
  }
}

// ── Hardware init ─────────────────────────────────────────────────────────────
//
// Called once from qemu_cpp_bridge_entry() after the governance banner.
// Sets up the complete interrupt path:
//   VBAR_EL1 → GICv3 distributor → CPU0 redistributor → CPU interface
//   → ARM physical timer (PPI 30, ~100Hz) → DAIF.I clear.

extern "C" void bridge_hw_init_aarch64() noexcept {
  dbg_puts("[axion] gic-init: enter\r\n");

  // ── 1. Install exception vector table ──────────────────────────────────────
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile(
      "msr vbar_el1, %0\n\t"
      "isb"
      :: "r"(reinterpret_cast<uintptr_t>(axion_exception_vector_base))
      : "memory");
#endif
  dbg_puts("[axion] gic-init: vbar ok\r\n");

  // ── 2. Enable GICv3 distributor (ARE_NS, then EnableGrp1NS) ───────────────
  gicd_write32(kGicDistBase, kGicdCtlr, kGicdCtlrAreNs);
  dbg_puts("[axion] gic-init: gicd-are-ns written\r\n");
  gicd_wait_rwp();
  gicd_write32(kGicDistBase, kGicdCtlr, kGicdCtlrAreNs | kGicdCtlrGrp1Ns);
  dbg_puts("[axion] gic-init: gicd-grp1ns written\r\n");
  gicd_wait_rwp();
  dbg_puts("[axion] gic-init: gicd-rwp done\r\n");

  // ── 3. Wake CPU0 redistributor ─────────────────────────────────────────────
  const uint32_t waker = gicd_read32(kGicRedistBase, kGicrWaker);
  gicd_write32(kGicRedistBase, kGicrWaker, waker & ~kGicrWakerSleep);
  dbg_puts("[axion] gic-init: gicr-waker cleared\r\n");
  gicr_wait_children();
  dbg_puts("[axion] gic-init: gicr-children done\r\n");

  // ── 4. Set all SGIs / PPIs to Group 1 NS in the SGI frame ─────────────────
  gicd_write32(kGicRedistBase, kGicrIgroupr0, 0xFFFF'FFFFu);
  dbg_puts("[axion] gic-init: igroupr0 set\r\n");

  // ── 5. Enable PPI 30 (physical timer) in GICR_ISENABLER0 ──────────────────
  gicd_write32(kGicRedistBase, kGicrIsenabler0, 1u << kTimerIntid);
  dbg_puts("[axion] gic-init: isenabler0 set\r\n");

  // ── 6. Set priority for PPI 30 ─────────────────────────────────────────────
  //   GICR_IPRIORITYR: one byte per INTID.  INTID 30 → word [30/4]=7, byte 2.
  {
    const uint32_t reg_idx  = kTimerIntid / 4u;
    const uint32_t byte_off = kTimerIntid % 4u;
    const uint32_t off      = kGicrIpriorityrN + reg_idx * 4u;
    uint32_t cur            = gicd_read32(kGicRedistBase, off);
    cur &= ~(0xFFu << (byte_off * 8u));
    cur |=  (0xA0u << (byte_off * 8u));  // priority 0xA0 (below mask 0xFF)
    gicd_write32(kGicRedistBase, off, cur);
  }
  dbg_puts("[axion] gic-init: ppi-priority set\r\n");

  // ── 7. CPU interface: enable SRE, set priority mask, enable Group 1 ────────
  icc_sre_write(1u);       // enable system-register interface
  dbg_puts("[axion] gic-init: icc-sre ok\r\n");
  icc_pmr_write(0xFFu);    // accept all interrupt priorities
  dbg_puts("[axion] gic-init: icc-pmr ok\r\n");
  icc_igrpen1_write(1u);   // enable Group 1 interrupts at EL1
  dbg_puts("[axion] gic-init: icc-igrpen1 ok\r\n");

  // ── 8. Arm physical timer at ~100Hz ────────────────────────────────────────
  cntp_tval_write(kTimerPeriod);  // initial countdown
  dbg_puts("[axion] gic-init: cntp-tval armed\r\n");
  cntp_ctl_write(1u);             // ENABLE=1, IMASK=0
  dbg_puts("[axion] gic-init: cntp-ctl enabled\r\n");

  // ── 9. Clear DAIF.I — enable IRQs ─────────────────────────────────────────
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile("msr daifclr, #2\n\tisb" ::: "memory");
#endif
  dbg_puts("[axion] gic-init: irqs enabled\r\n");
}

// ── SVC trap stub ─────────────────────────────────────────────────────────────
// axion_svc_entry in aarch64_exception_vectors.S calls this for Lower-EL SVC.
// The freestanding bridge has no user-space; this stub satisfies the linker and
// returns safely if an SVC fires unexpectedly.

extern "C" void axion_kernel_handle_svc_trap_aarch64(void* /*frame*/) noexcept {}

// ── IRQ handler ───────────────────────────────────────────────────────────────
//
// Called by axion_irq_entry in aarch64_exception_vectors.S after saving
// caller-saved registers x0–x18, x30 (160 bytes).  Must be async-signal-safe
// and may not use any hosted C++ runtime services.

extern "C" void axion_irq_handler_aarch64() noexcept {
  const uint32_t intid = static_cast<uint32_t>(icc_iar1_read() & 0xFFFFFFu);

  if (intid == kGicSpurious) return;

  if (intid == kTimerIntid) {
    // Reload the physical timer for the next 10 ms period.
    cntp_tval_write(kTimerPeriod);
    // Notify the bridge (updates scheduler counters, advances ready threads).
    bridge_timer_irq_tick();
  }

  icc_eoir1_write(static_cast<uint64_t>(intid));
}
