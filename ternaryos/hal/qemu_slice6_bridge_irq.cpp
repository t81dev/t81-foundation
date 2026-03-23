// ternaryos/hal/qemu_slice6_bridge_irq.cpp
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

// RFC-00C2/C4: wake BlockedDeviceWait threads matching the fired INTID.
// Defined in qemu_slice6_el0_svc_bridge.cpp; async-signal-safe.
extern "C" void fs_sched_timer_device_wake(uint32_t intid) noexcept;

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
  // ── 1. Install exception vector table ──────────────────────────────────────
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile(
      "msr vbar_el1, %0\n\t"
      "isb"
      :: "r"(reinterpret_cast<uintptr_t>(axion_exception_vector_base))
      : "memory");
#endif

  // ── 2. Enable GICv3 distributor (ARE_NS, then EnableGrp1NS) ───────────────
  gicd_write32(kGicDistBase, kGicdCtlr, kGicdCtlrAreNs);
  gicd_wait_rwp();
  gicd_write32(kGicDistBase, kGicdCtlr, kGicdCtlrAreNs | kGicdCtlrGrp1Ns);
  gicd_wait_rwp();

  // ── 3. Wake CPU0 redistributor ─────────────────────────────────────────────
  const uint32_t waker = gicd_read32(kGicRedistBase, kGicrWaker);
  gicd_write32(kGicRedistBase, kGicrWaker, waker & ~kGicrWakerSleep);
  gicr_wait_children();

  // ── 4. Set all SGIs / PPIs to Group 1 NS in the SGI frame ─────────────────
  gicd_write32(kGicRedistBase, kGicrIgroupr0, 0xFFFF'FFFFu);

  // ── 5. Enable PPI 30 (physical timer) in GICR_ISENABLER0 ──────────────────
  gicd_write32(kGicRedistBase, kGicrIsenabler0, 1u << kTimerIntid);

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

  // ── 7. CPU interface: enable SRE, set priority mask, enable Group 1 ────────
  icc_sre_write(1u);       // enable system-register interface
  icc_pmr_write(0xFFu);    // accept all interrupt priorities
  icc_igrpen1_write(1u);   // enable Group 1 interrupts at EL1

  // ── 8. Arm physical timer at ~100Hz ────────────────────────────────────────
  cntp_tval_write(kTimerPeriod);  // initial countdown
  cntp_ctl_write(1u);             // ENABLE=1, IMASK=0

  // ── 9. Clear DAIF.I — enable IRQs ─────────────────────────────────────────
#if defined(__aarch64__) && !defined(__APPLE__)
  __asm__ volatile("msr daifclr, #2\n\tisb" ::: "memory");
#endif
}

// ── SVC trap frame (must match axion_svc_entry layout in aarch64_exception_vectors.S)
//   x[0..30]  : offsets   0..247  (31 × 8 bytes)
//   sp_el0    : offset  248
//   elr_el1   : offset  256
//   spsr_el1  : offset  264
//   esr_el1   : offset  272

struct AArch64TrapFrameSimple {
  uint64_t x[31];      //   0..247
  uint64_t sp_el0;     // 248
  uint64_t elr_el1;    // 256
  uint64_t spsr_el1;   // 264
  uint64_t esr_el1;    // 272
};

// Written by run_el0_init() (qemu_slice6_cpp_bridge.cpp) before ERET to EL0.
// Read by the ExitThread SVC (#2) handler below to redirect ERET back to EL1.
extern "C" uint64_t g_axion_el1_return_pc = 0;

// Phase 5: TVA validation — defined in qemu_slice6_el0_mmu.cpp.
// Returns 1 iff [va, va+size) lies entirely within the EL0-mapped VA range.
extern "C" int el0_tva_valid(uint64_t va, uint64_t size) noexcept;

// Phase 6: KernelCall dispatcher — defined in qemu_slice6_el0_svc_bridge.cpp.
// Handles SVC #1 (KernelCall): reads the wire request block from TVA,
// dispatches based on kind, writes the wire response block back.
extern "C" void el0_svc_kernel_call_dispatch(void* frame_ptr) noexcept;

// Phase 9 (RFC-00BE): cooperative scheduler ExitThread hook.
// Marks current running thread Exited and context-switches to the next
// Runnable thread (or returns to EL1 if none remain).
extern "C" void fs_sched_exit_thread(void* frame_ptr) noexcept;

// ── PL011 helpers for the SVC dispatcher ─────────────────────────────────────
// Mirrors the pl011 helpers in qemu_slice6_cpp_bridge.cpp; duplicated here so
// this translation unit remains self-contained (no cross-TU static calls).

static constexpr uint64_t kSvcPl011Base = UINT64_C(0x09000000);

static void svc_pl011_putchar(char c) noexcept {
#if defined(__aarch64__) && !defined(__APPLE__)
  volatile uint32_t* fr = reinterpret_cast<volatile uint32_t*>(
      static_cast<uintptr_t>(kSvcPl011Base + 0x018u));
  volatile uint32_t* dr = reinterpret_cast<volatile uint32_t*>(
      static_cast<uintptr_t>(kSvcPl011Base + 0x000u));
  while (*fr & (1u << 5u)) { __asm__ volatile("yield" ::: "memory"); }
  *dr = static_cast<uint32_t>(static_cast<unsigned char>(c));
#else
  (void)c;
#endif
}

static void svc_pl011_puts(const char* s) noexcept {
  while (*s) svc_pl011_putchar(*s++);
}

// Print a 64-bit value as 16 hex digits (debug diagnostic helper).
static void svc_pl011_puthex64(uint64_t v) noexcept {
  static const char kHex[] = "0123456789abcdef";
  for (int i = 60; i >= 0; i -= 4)
    svc_pl011_putchar(kHex[(v >> i) & 0xFu]);
}

// ── SVC dispatcher ────────────────────────────────────────────────────────────
//
// Called by axion_svc_entry (aarch64_exception_vectors.S) for every SVC taken
// from Lower EL (AArch64).  The trap frame is the 280-byte block saved on the
// EL1 stack; modifying elr_el1/spsr_el1 redirects the ERET destination.
//
// Phase 6 SVC ABI (RFC-00BC §"Phase 6 SVC ABI version table"):
//   svc #0  GetThreadIdentity            →  x0 = tid (unchanged from Phase 4)
//   svc #1  KernelCall(x0,x1,x2,x3)     →  x0=req_tva x1=req_sz x2=rsp_tva x3=rsp_sz
//                                            routes to el0_svc_kernel_call_dispatch()
//   svc #2  ExitThread(x0=exit_code)     →  thread teardown; ERET to EL1
//   svc #3  WriteSerial(x0=str_tva)      →  debug PL011 write (AXION_DEBUG_SERIAL)
//
// SVC #1 (KernelCall) replaces Phase 4 WriteSerial; the single narrow entry
// point follows RFC-00B6 §5.1 — one governed channel for all kernel operations.
// SVC #2 (ExitThread) replaces Phase 4 ExitToEL1; semantically identical in
// the freestanding bridge (patches ELR_EL1 / SPSR_EL1) but carries the correct
// kernel API name for Phase 7+ thread lifecycle management.
// SVC #3 (WriteSerial) is demoted to debug-only per RFC-00BC D1.

extern "C" void axion_kernel_handle_svc_trap_aarch64(void* frame_ptr) noexcept {
  auto* f = static_cast<AArch64TrapFrameSimple*>(frame_ptr);

  // ESR_EL1[15:0] = SVC immediate (EC=0x15 for SVC from AArch64 EL0).
  const uint32_t imm = static_cast<uint32_t>(f->esr_el1 & 0xFFFFu);

  switch (imm) {
    case 0u:  // GetThreadIdentity → x0 = tid (kernel thread = 1)
      f->x[0] = 1u;
      break;

    case 1u:  // KernelCall(x0=req_tva, x1=req_size, x2=rsp_tva, x3=rsp_size)
      // Route to the Phase 6 freestanding KernelCall dispatcher.  Both TVAs
      // are Phase 5 TVA-validated inside el0_svc_kernel_call_dispatch().
      el0_svc_kernel_call_dispatch(f);
      break;

    case 2u:  // ExitThread(x0 = exit_code)
      // Phase 9 (RFC-00BE): delegate to the cooperative scheduler hook.
      // When the scheduler is active it context-switches to the next Runnable
      // thread; when inactive it redirects ERET to g_axion_el1_return_pc
      // (same as the Phase 6–8 behaviour).
      fs_sched_exit_thread(f);
      break;

    case 3u: {  // WriteSerial(x0 = const char* str_tva) — debug-only
      // TVA-validated before dereferencing.  The 1-byte probe covers the
      // pointer itself; the null terminator must also be in the mapped region
      // or a translation fault occurs (the intended Phase 5 isolation behavior).
      const uint64_t va = f->x[0];
      if (!el0_tva_valid(va, 1u)) {
        svc_pl011_puts("[axion] svc#3: TVA denied\r\n");
        break;
      }
      svc_pl011_puts(reinterpret_cast<const char*>(
          static_cast<uintptr_t>(va)));
      break;
    }

    default:
      // Unexpected exception from lower EL — dump ESR so we can diagnose.
      svc_pl011_puts("[axion] TRAP: esr=");
      svc_pl011_puthex64(f->esr_el1);
      svc_pl011_puts(" elr=");
      svc_pl011_puthex64(f->elr_el1);
      svc_pl011_puts("\r\n");
      // Halt: redirect ERET to EL1 halt loop (prevent infinite fault).
      f->elr_el1  = g_axion_el1_return_pc;
      f->spsr_el1 = 0x5u;  // EL1h, DAIF all unmasked
      break;
  }
}

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
    // RFC-00C2: transition any BlockedDeviceWait threads to Runnable so that
    // fs_sched_device_wait_loop() detects them after wfi returns.
    fs_sched_timer_device_wake(kTimerIntid);  // RFC-00C4: pass INTID 30
  }

  icc_eoir1_write(static_cast<uint64_t>(intid));
}
