// userland/experimental/hal/qemu_x86_64_bridge_irq.cpp
//
// Freestanding x86_64 hardware-interrupt wiring for the QEMU OVMF EFI bridge.
//
// Linked into BOOTX64.EFI alongside qemu_x86_64_cpp_bridge.cpp.
//
// Exported symbols:
//   bridge_hw_init_x86_64()       — call once from qemu_x86_64_cpp_bridge_entry()
//                                   after the governance banner is printed.
//                                   Reads the live CS selector, installs a
//                                   256-entry 64-bit IDT, programs the 8259 PIC
//                                   (unmask IRQ0 / mask all else), programs the
//                                   PIT at 100Hz (divisor 11932), and executes
//                                   STI.
//   axion_timer_stub_x86_64()     — [[gnu::interrupt]] ISR for IDT vector 0x20;
//                                   calls bridge_timer_irq_tick_x86() then sends
//                                   PIC master EOI (0x20 → port 0x20).
//
// Compilation constraints (same as qemu_x86_64_cpp_bridge.cpp):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -fno-use-cxa-atexit
//   -nostdlib  --target=x86_64-pc-windows-msvc
//
// Only <stdint.h> is used; no C++ runtime, no hosted headers.

#include <stdint.h>

// ── Bridge callback (defined in qemu_x86_64_cpp_bridge.cpp) ──────────────────

/// Called from the timer ISR every ~100Hz tick.
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
  uint16_t offset_lo;   // handler VA bits [15:0]
  uint16_t selector;    // 64-bit code segment selector
  uint8_t  ist;         // IST index (0 = legacy stack switching)
  uint8_t  type_attr;   // 0x8E = Present|DPL=0|64-bit interrupt gate
  uint16_t offset_mid;  // handler VA bits [31:16]
  uint32_t offset_hi;   // handler VA bits [63:32]
  uint32_t reserved;
} __attribute__((packed));

static_assert(sizeof(IdtEntry) == 16u, "IDT entry must be exactly 16 bytes");

struct [[gnu::packed]] IdtDescriptor {
  uint16_t limit;
  uint64_t base;
};

static_assert(sizeof(IdtDescriptor) == 10u, "IDTR must be exactly 10 bytes");

// Static IDT — 256 × 16 bytes = 4 KiB.
// alignas(16) satisfies the LIDT alignment recommendation.
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

// ── Timer ISR ─────────────────────────────────────────────────────────────────
//
// [[gnu::interrupt]] causes Clang to generate IRETQ and save/restore the
// caller-saved register set (RAX, RCX, RDX, RSI, RDI, R8–R11), making this
// function suitable as a 64-bit interrupt gate target.
//
// The function receives a pointer to the CPU-pushed interrupt frame but does
// not inspect it (void*).

[[gnu::interrupt]] extern "C"
void axion_timer_stub_x86_64(void* /*frame*/) noexcept {
  bridge_timer_irq_tick_x86();
  // Send End-of-Interrupt to the master 8259 PIC (port 0x20, command 0x20).
  outb_irq(0x20u, 0x20u);
}

// ── Hardware init ─────────────────────────────────────────────────────────────
//
// Called once from qemu_x86_64_cpp_bridge_entry() after the T81 governance
// banner and before entering the HLT-based event loop.

extern "C" void bridge_hw_init_x86_64() noexcept {
  // ── Step 1: Read the current code-segment selector ─────────────────────────
  // OVMF exits boot services with a flat 64-bit code segment (typically 0x38
  // or similar).  Read CS at runtime to avoid hardcoding.
  uint16_t cs = 0x38u;
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("mov %%cs, %0" : "=r"(cs));
#endif

  // ── Step 2: Zero-fill and populate the IDT ─────────────────────────────────
  for (int i = 0; i < 256; ++i) {
    IdtEntry& e = s_idt[i];
    e.offset_lo = e.selector = 0u;
    e.ist       = e.type_attr = 0u;
    e.offset_mid = 0u;
    e.offset_hi  = 0u;
    e.reserved   = 0u;
  }

  // Vector 0x20 — PIT timer (IRQ 0 remapped by OVMF to 0x20–0x2F).
  // 0x8E = Present(1) | DPL=00 | 0 | Type=1110 (64-bit interrupt gate).
  idt_set_gate(0x20u,
               reinterpret_cast<void(*)()>(axion_timer_stub_x86_64),
               cs, 0x8Eu);

  // Load the IDT register.
  IdtDescriptor idtd;
  idtd.limit = static_cast<uint16_t>(sizeof(s_idt) - 1u);
  idtd.base  = reinterpret_cast<uint64_t>(&s_idt[0]);
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("lidt %0" :: "m"(idtd) : "memory");
#endif

  // ── Step 3: Configure 8259 PIC masks ──────────────────────────────────────
  // OVMF has already remapped the PIC: master → 0x20, slave → 0x28.
  // Unmask only IRQ0 (PIT) on the master; mask all slave interrupts.
  outb_irq(0x21u, 0xFEu);  // master IMR: bit 0 = IRQ0 unmasked
  io_wait();
  outb_irq(0xA1u, 0xFFu);  // slave IMR: all masked
  io_wait();

  // ── Step 4: Program PIT channel 0 at 100Hz ────────────────────────────────
  // PIT input clock: 1 193 182 Hz.  Divisor for 100Hz: 11932.
  // Control byte 0x34: ch0(00) | lobyte/hibyte(11) | mode2(010) | binary(0).
  static constexpr uint16_t kPitDivisor = 11932u;
  outb_irq(0x43u, 0x34u);
  io_wait();
  outb_irq(0x40u, static_cast<uint8_t>(kPitDivisor & 0xFFu));
  io_wait();
  outb_irq(0x40u, static_cast<uint8_t>(kPitDivisor >> 8));
  io_wait();

  // ── Step 5: Enable interrupts ─────────────────────────────────────────────
#if defined(__x86_64__) && defined(_WIN32)
  __asm__ volatile("sti" ::: "memory");
#endif
}
