// ternaryos/hal/qemu_x86_64_bridge_irq.cpp
//
// Freestanding x86_64 hardware-interrupt wiring for the QEMU OVMF EFI bridge.
//
// Linked into BOOTX64.EFI alongside qemu_x86_64_cpp_bridge.cpp.
//
// Exported symbols:
//   bridge_hw_init_x86_64(tsc_freq_hz) — call once from bridge_entry() after
//       the governance banner.  Installs a 256-entry 64-bit IDT, then selects:
//         LAPIC path (preferred): reads IA32_APIC_BASE MSR, enables Local APIC,
//           calibrates LAPIC timer to 100Hz against the TSC, masks the 8259 PIC.
//           Returns true; caller emits LAPIC banner.
//         PIT fallback: programs 8259 PIC + PIT ch0 at 100Hz if LAPIC is
//           unavailable or calibration fails.  Returns false.
//   axion_lapic_timer_stub_x86_64() — LAPIC timer ISR (IDT 0x40); calls
//       bridge_timer_irq_tick_x86() then writes LAPIC EOI.
//   axion_timer_stub_x86_64()       — PIT fallback ISR (IDT 0x20); calls
//       bridge_timer_irq_tick_x86() then sends 8259 master EOI.
//
// Compilation constraints (same as qemu_x86_64_cpp_bridge.cpp):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=x86_64-pc-windows-msvc
//
// Only <stdint.h> is used; no C++ runtime, no hosted headers.

#include <stdint.h>

// ── Bridge callback (defined in qemu_x86_64_cpp_bridge.cpp) ──────────────────

extern "C" void bridge_timer_irq_tick_x86() noexcept;

// ── x86 port I/O helpers ─────────────────────────────────────────────────────

static inline void outb_irq(uint16_t port, uint8_t val) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("outb %0, %1" :: "a"(val), "Nd"(port) : "memory");
#else
  (void)port; (void)val;
#endif
}

/// Short I/O delay: write to port 0x80 (POST diagnostic port, no side-effects).
static inline void io_wait() noexcept { outb_irq(0x80u, 0u); }

// ── 64-bit IDT entry (16 bytes, interrupt gate) ───────────────────────────────

struct IdtEntry {
  uint16_t offset_lo;
  uint16_t selector;
  uint8_t  ist;
  uint8_t  type_attr;   // 0x8E = Present|DPL=0|64-bit interrupt gate
  uint16_t offset_mid;
  uint32_t offset_hi;
  uint32_t reserved;
} __attribute__((packed));

static_assert(sizeof(IdtEntry) == 16u, "IDT entry must be exactly 16 bytes");

struct __attribute__((packed)) IdtDescriptor {
  uint16_t limit;
  uint64_t base;
};

static_assert(sizeof(IdtDescriptor) == 10u, "IDTR must be exactly 10 bytes");

alignas(16) static IdtEntry s_idt[256];

static void idt_set_gate(uint8_t vec, void (*handler)(),
                          uint16_t sel, uint8_t type_attr) noexcept {
  const uint64_t addr = reinterpret_cast<uint64_t>(
      reinterpret_cast<uintptr_t>(handler));
  IdtEntry& e    = s_idt[vec];
  e.offset_lo    = static_cast<uint16_t>(addr & 0xFFFFu);
  e.selector     = sel;
  e.ist          = 0u;
  e.type_attr    = type_attr;
  e.offset_mid   = static_cast<uint16_t>((addr >> 16) & 0xFFFFu);
  e.offset_hi    = static_cast<uint32_t>((addr >> 32) & 0xFFFF'FFFFu);
  e.reserved     = 0u;
}

// ── LAPIC MMIO helpers ────────────────────────────────────────────────────────
// IA32_APIC_BASE MSR = 0x1B.  Bits [35:12] = LAPIC physical base (default
// 0xFEE00000 on all x86 platforms; OVMF maintains the mapping after EBS).

static constexpr uint32_t kLAPIC_EOI     = 0x0B0u;  // End of Interrupt (W: any)
static constexpr uint32_t kLAPIC_SVR     = 0x0F0u;  // Spurious Interrupt Vector
static constexpr uint32_t kLAPIC_TMR_LVT = 0x320u;  // LVT Timer
static constexpr uint32_t kLAPIC_TMR_ICR = 0x380u;  // Initial Count
static constexpr uint32_t kLAPIC_TMR_CCR = 0x390u;  // Current Count (R/O)
static constexpr uint32_t kLAPIC_TMR_DCR = 0x3E0u;  // Divide Configuration

// LAPIC register base — set once during init, read by the ISR for EOI.
static uint64_t s_lapic_base = 0u;

static inline uint32_t lapic_read(uint64_t base, uint32_t off) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  return *reinterpret_cast<const volatile uint32_t*>(
      static_cast<uintptr_t>(base + off));
#else
  (void)base; (void)off; return 0u;
#endif
}

static inline void lapic_write(uint64_t base, uint32_t off, uint32_t val) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  *reinterpret_cast<volatile uint32_t*>(
      static_cast<uintptr_t>(base + off)) = val;
#else
  (void)base; (void)off; (void)val;
#endif
}

// ── CPU feature helpers ───────────────────────────────────────────────────────

/// True if CPUID leaf 1 EDX bit 9 (APIC on-chip) is set.
static bool cpuid_has_apic() noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  uint32_t eax, edx;
  __asm__ volatile("cpuid" : "=a"(eax), "=d"(edx)
                           : "0"(1u), "c"(0u) : "ebx");
  return !!(edx & (1u << 9));
#else
  return false;
#endif
}

/// Read a 64-bit MSR.
static uint64_t rdmsr_irq(uint32_t msr) noexcept {
#if defined(__x86_64__) && defined(_WIN32)
  uint32_t lo, hi;
  __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
  return (static_cast<uint64_t>(hi) << 32) | lo;
#else
  (void)msr; return 0u;
#endif
}

/// Read TSC (for LAPIC timer calibration).
static uint64_t rdtsc_irq() noexcept {
#if defined(__x86_64__)
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<uint64_t>(hi) << 32) | lo;
#else
  return 0u;
#endif
}

// ── Timer ISRs ────────────────────────────────────────────────────────────────
//
// [[gnu::interrupt]] / __attribute__((interrupt)) cause Clang to generate
// IRETQ and save/restore the full caller-saved register set, making the
// function suitable as a 64-bit interrupt gate target.

// LAPIC timer ISR — IDT vector 0x40.
// Sends EOI to the Local APIC (write 0 to EOI register; PIC not involved).
extern "C" __attribute__((interrupt))
void axion_lapic_timer_stub_x86_64(void* /*frame*/) noexcept {
  bridge_timer_irq_tick_x86();
  lapic_write(s_lapic_base, kLAPIC_EOI, 0u);
}

// PIT fallback ISR — IDT vector 0x20.
// Sends EOI to the master 8259 PIC (port 0x20, command 0x20).
extern "C" __attribute__((interrupt))
void axion_timer_stub_x86_64(void* /*frame*/) noexcept {
  bridge_timer_irq_tick_x86();
  outb_irq(0x20u, 0x20u);
}

// ── Hardware init ─────────────────────────────────────────────────────────────
//
// Returns true  → LAPIC timer armed at 100Hz, IDT 0x40, 8259 PIC fully masked.
// Returns false → PIT ch0 100Hz armed, IDT 0x20, legacy 8259 path active.

extern "C" bool bridge_hw_init_x86_64(uint64_t tsc_freq_hz) noexcept {
  // ── Step 1: Read the live CS selector ─────────────────────────────────────
  uint16_t cs = 0x38u;
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("mov %%cs, %0" : "=r"(cs));
#endif

  // ── Step 2: Zero-fill IDT ─────────────────────────────────────────────────
  for (int i = 0; i < 256; ++i) {
    IdtEntry& e = s_idt[i];
    e.offset_lo = e.selector = 0u;
    e.ist       = e.type_attr = 0u;
    e.offset_mid = 0u;
    e.offset_hi  = 0u;
    e.reserved   = 0u;
  }

  // Vector 0x40 — LAPIC timer (preferred path).
  idt_set_gate(0x40u,
               reinterpret_cast<void(*)()>(axion_lapic_timer_stub_x86_64),
               cs, 0x8Eu);
  // Vector 0x20 — PIT/8259 fallback (active only when LAPIC unavailable).
  idt_set_gate(0x20u,
               reinterpret_cast<void(*)()>(axion_timer_stub_x86_64),
               cs, 0x8Eu);

  // ── Step 3: Load IDTR ─────────────────────────────────────────────────────
  IdtDescriptor idtd;
  idtd.limit = static_cast<uint16_t>(sizeof(s_idt) - 1u);
  idtd.base  = reinterpret_cast<uint64_t>(&s_idt[0]);
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("lidt %0" :: "m"(idtd) : "memory");
#endif

  // ── Step 4: Attempt LAPIC timer init ──────────────────────────────────────
  bool lapic_ok = false;

  if (cpuid_has_apic() && tsc_freq_hz > 0u) {
    // Read LAPIC base from IA32_APIC_BASE MSR; bits [35:12] are the PA.
    const uint64_t base = rdmsr_irq(0x1Bu) & UINT64_C(0xFFFFF000);
    if (base != 0u) {
      s_lapic_base = base;

      // Enable LAPIC: SVR bit 8 = Software Enable; bits [7:0] = spurious vec.
      lapic_write(base, kLAPIC_SVR, 0x1FFu);

      // Set divide-by-16 and start timer at a large initial count for calibration.
      // LVT: periodic (bit 17), masked (bit 16), vector 0x40.
      lapic_write(base, kLAPIC_TMR_DCR, 0x3u);        // ÷16
      lapic_write(base, kLAPIC_TMR_ICR, 0x7FFFFFFFu); // max count
      lapic_write(base, kLAPIC_TMR_LVT, 0x30040u);    // periodic|masked|0x40

      // Busy-wait for tsc_freq_hz / 100 TSC ticks ≈ 10 ms (one 100Hz period).
      const uint64_t delay = tsc_freq_hz / 100u;
      const uint64_t t0    = rdtsc_irq();
      while (rdtsc_irq() - t0 < delay) {
#if defined(__x86_64__)
        __asm__ volatile("pause" ::: "memory");
#endif
      }

      // LAPIC counts elapsed in 10 ms.
      const uint32_t ticks_per_period =
          0x7FFFFFFFu - lapic_read(base, kLAPIC_TMR_CCR);

      if (ticks_per_period > 0u) {
        // Reconfigure: periodic, unmasked, vector 0x40, calibrated count.
        lapic_write(base, kLAPIC_TMR_ICR, 0u);          // stop
        lapic_write(base, kLAPIC_TMR_LVT, 0x20040u);    // periodic|unmask|0x40
        lapic_write(base, kLAPIC_TMR_ICR, ticks_per_period); // arm

        lapic_ok = true;
      }
    }
  }

  // ── Step 5: PIC configuration ──────────────────────────────────────────────
  if (lapic_ok) {
    // LAPIC handles all timer interrupts — mask the entire 8259 PIC so that
    // no legacy interrupt lines can fire and interfere with LAPIC delivery.
    outb_irq(0x21u, 0xFFu);  // master: all IRQs masked
    io_wait();
    outb_irq(0xA1u, 0xFFu);  // slave: all IRQs masked
    io_wait();
  } else {
    // LAPIC unavailable or calibration failed — fall back to 8259 PIC + PIT.
    // Unmask only IRQ0 (PIT channel 0) on the master; mask all slave IRQs.
    outb_irq(0x21u, 0xFEu);
    io_wait();
    outb_irq(0xA1u, 0xFFu);
    io_wait();

    // Program PIT channel 0 at 100Hz (divisor 11932, clock 1.193182 MHz).
    // Control byte 0x34: ch0(00) | lo/hi(11) | mode2(010) | binary(0).
    static constexpr uint16_t kPitDivisor = 11932u;
    outb_irq(0x43u, 0x34u);
    io_wait();
    outb_irq(0x40u, static_cast<uint8_t>(kPitDivisor & 0xFFu));
    io_wait();
    outb_irq(0x40u, static_cast<uint8_t>(kPitDivisor >> 8));
    io_wait();
  }

  // ── Step 6: Enable interrupts ─────────────────────────────────────────────
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("sti" ::: "memory");
#endif

  return lapic_ok;
}
