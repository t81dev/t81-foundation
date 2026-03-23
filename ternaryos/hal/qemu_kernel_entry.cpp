// experimental/ternaryos/hal/qemu_kernel_entry.cpp
//
// QEMU virt AArch64 bare-metal boot trampoline — hardware init, ARM timer,
// IRQ C handler (axion_irq_handler_aarch64), and kernel event loop.

#include "qemu_kernel_entry.hpp"
#include "aarch64_trap_entry.hpp"
#include "../dev/gicv3.hpp"
#include "../dev/pl011_uart.hpp"
#include "../kernel/kernel_main.hpp"

// ── File-scope state (shared between the namespace impl and extern "C") ───────
// These must be at file scope (not inside a named namespace) so that the
// extern "C" IRQ handler — which the assembler calls with no C++ namespace
// context — can reach them without qualification tricks.

/// Timer reload value initialised by qemu_hardware_init().
static uint32_t s_timer_period_counts = t81::ternaryos::hal::kQemuDefaultTimerPeriod;

// ── ARM generic timer MMIO helpers ───────────────────────────────────────────

#if defined(__aarch64__) && !defined(__APPLE__)

static inline void arm_timer_start(uint32_t tval) noexcept {
  __asm__ volatile(
      "msr cntp_tval_el0, %0\n\t"
      "msr cntp_ctl_el0,  %1\n\t"
      "isb"
      :: "r"(static_cast<uint64_t>(tval)),
         "r"(static_cast<uint64_t>(1u))   // ENABLE=1, IMASK=0
      : "memory");
}

static inline void arm_timer_reload(uint32_t tval) noexcept {
  __asm__ volatile(
      "msr cntp_tval_el0, %0\n\t"
      "isb"
      :: "r"(static_cast<uint64_t>(tval)) : "memory");
}

#else

[[maybe_unused]] static inline void arm_timer_start(uint32_t) noexcept {}
[[maybe_unused]] static inline void arm_timer_reload(uint32_t) noexcept {}

#endif

// ── C-linkage IRQ dispatcher ─────────────────────────────────────────────────
// Declared extern "C" at file scope so the assembler symbol axion_irq_handler_aarch64
// resolves without mangling.  Must appear before any namespace that would wrap it.

extern "C" void axion_irq_handler_aarch64() noexcept {
  using namespace t81::ternaryos;

  const uint32_t intid = dev::gicv3_acknowledge();
  if (intid == dev::kGicSpuriousIntid) return;

  hal::HardwareInterrupt hw{};
  // timestamp_ns = 0 → dispatch_interrupt() stamps it from monotonic clock

  if (intid == hal::kQemuTimerPhysIntId) {
    arm_timer_reload(s_timer_period_counts);
    hw.source  = hal::InterruptSource::Timer;
    hw.payload = intid;
  } else {
    hw.source  = hal::InterruptSource::Unknown;
    hw.payload = intid;
  }

  hal::dispatch_interrupt(hw);
  dev::gicv3_eoi(intid);
}

// ── Serial shell helpers ──────────────────────────────────────────────────────

namespace {

/// Format a uint64_t into buf (null-terminated).  Returns pointer to buf.
static const char* u64_str(uint64_t v, char* buf, int bufsz) noexcept {
  buf[bufsz - 1] = '\0';
  int i = bufsz - 2;
  if (v == 0) { buf[i--] = '0'; }
  while (v > 0 && i >= 0) { buf[i--] = static_cast<char>('0' + (v % 10)); v /= 10; }
  return &buf[i + 1];
}

/// Dispatch a complete (CR-terminated) line entered at the t81> prompt.
static void shell_dispatch(const char* line,
                           const t81::ternaryos::kernel::KernelRuntimeState& state,
                           uint64_t loop_step) noexcept {
  using namespace t81::ternaryos::dev;

  // Skip leading whitespace.
  while (*line == ' ' || *line == '\t') ++line;

  char nb[24];

  if (line[0] == '\0') {
    // Empty line — just re-display prompt (handled by caller).

  } else if (__builtin_strcmp(line, "help") == 0) {
    pl011_puts(kQemuVirtPl011Base,
      "  help     — this message\r\n"
      "  version  — T81 build info\r\n"
      "  status   — kernel counters and governance state\r\n"
      "  policy   — Axion policy summary\r\n");

  } else if (__builtin_strcmp(line, "version") == 0) {
    pl011_puts(kQemuVirtPl011Base,
      "  T81 Foundation v1.9.2  |  TISC ISA v1.9.0 (frozen)\r\n"
      "  Axion policy kernel     |  CanonHash81 deterministic traces\r\n"
      "  Platform: ");
    pl011_puts(kQemuVirtPl011Base, state.platform_id.c_str());
    pl011_puts(kQemuVirtPl011Base, "\r\n");

  } else if (__builtin_strcmp(line, "status") == 0) {
    pl011_puts(kQemuVirtPl011Base, "  [kernel]\r\n");

    pl011_puts(kQemuVirtPl011Base, "    threads       : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.thread_runtime.size(), nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

    pl011_puts(kQemuVirtPl011Base, "    loop steps    : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(loop_step, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

    pl011_puts(kQemuVirtPl011Base, "    sched ticks   : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.counters.scheduler_ticks, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

    pl011_puts(kQemuVirtPl011Base, "    faults        : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.counters.faults_delivered, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

    pl011_puts(kQemuVirtPl011Base, "    ipc sent/recv : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.counters.ipc_messages_sent, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, " / ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.counters.ipc_messages_received, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

    pl011_puts(kQemuVirtPl011Base, "    canonfs       : ");
    pl011_puts(kQemuVirtPl011Base,
      state.published_executable_canonfs ? "mounted\r\n" : "offline\r\n");

  } else if (__builtin_strcmp(line, "policy") == 0) {
    pl011_puts(kQemuVirtPl011Base, "  [axion policy engine]\r\n");
    pl011_puts(kQemuVirtPl011Base, "    state         : ready\r\n");
    pl011_puts(kQemuVirtPl011Base, "    mode          : fail-closed\r\n");
    pl011_puts(kQemuVirtPl011Base, "    policy faults : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.counters.policy_faults, nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");
    pl011_puts(kQemuVirtPl011Base, "    process groups: ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.process_groups.size(), nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");
    pl011_puts(kQemuVirtPl011Base, "    supervisors   : ");
    pl011_puts(kQemuVirtPl011Base, u64_str(state.supervisors.size(), nb, sizeof nb));
    pl011_puts(kQemuVirtPl011Base, "\r\n");

  } else {
    pl011_puts(kQemuVirtPl011Base, "  unknown command: ");
    pl011_puts(kQemuVirtPl011Base, line);
    pl011_puts(kQemuVirtPl011Base, "\r\n  type 'help' for available commands\r\n");
  }
}

}  // namespace

// ── Namespace implementation ──────────────────────────────────────────────────

namespace t81::ternaryos::hal {

bool qemu_hardware_init(const QemuProfile& profile,
                        uint32_t timer_period_counts) noexcept {
  s_timer_period_counts = timer_period_counts;

  using namespace dev;

  // 1. Serial console first so every subsequent message is visible.
  pl011_init(kQemuVirtPl011Base);
  pl011_puts(kQemuVirtPl011Base, "[axion] QEMU virt AArch64 hardware init\n");

  (void)profile;  // device base addresses come from the driver headers directly

  // 2. GICv3: distributor + CPU0 redistributor + EL1 CPU interface.
  gicv3_init(kQemuVirtGicDistBase, kQemuVirtGicRedistBase);
  pl011_puts(kQemuVirtPl011Base, "[axion] GICv3 online\n");

  // 3. Enable ARM generic timer physical PPI (PPI 14 → INTID 30).
  gicv3_set_priority(kQemuVirtGicDistBase, kQemuTimerPhysIntId, 0xA0u);
  gicv3_enable_ppi(kQemuVirtGicRedistBase, 14u);
  pl011_puts(kQemuVirtPl011Base, "[axion] timer PPI 14 (INTID 30) enabled\n");

  // 4. Install exception vectors (writes VBAR_EL1; no-op on macOS/hosted).
  axion_kernel_install_exception_vectors();
  pl011_puts(kQemuVirtPl011Base, "[axion] VBAR_EL1 installed\n");

  // 5. Arm the physical timer.
  arm_timer_start(timer_period_counts);
  pl011_puts(kQemuVirtPl011Base, "[axion] ARM timer armed\n");

#if defined(__aarch64__) && !defined(__APPLE__)
  return true;
#else
  return false;
#endif
}

void qemu_kernel_run_loop(kernel::KernelRuntimeState& state,
                          uint32_t timer_period_counts,
                          uint64_t max_steps) noexcept {
  (void)timer_period_counts;  // reload is handled inside the IRQ handler

  using namespace kernel;

  // Wire the timer source into the kernel: each tick advances the clock and
  // queues the interrupt for delivery in the step loop below.
  register_interrupt_handler(
      InterruptSource::Timer,
      [&state](const HardwareInterrupt& hw) {
        axion_kernel_record_interrupt(state, hw);
        axion_kernel_tick(state);
      });

  // RFC-00B5 §3.5 (Slice 28): wire the unhandled-interrupt governance callback.
  // Interrupts with no registered source handler are audited as
  // UnhandledInterruptDropped rather than silently discarded.
  register_unhandled_interrupt_callback(
      [&state](const HardwareInterrupt& hw) {
        axion_kernel_record_unhandled_interrupt(state, hw);
      });

  const bool run_forever = (max_steps == 0);
  uint64_t steps = 0;

  while (run_forever || steps < max_steps) {
    axion_kernel_deliver_pending_interrupt(state);
    axion_kernel_deliver_pending_fault(state);
    axion_kernel_run_pager_policy(state);
    axion_kernel_step(state);
    ++steps;

    // ── PL011 RX: line-buffered command shell ────────────────────────────
    // Poll without blocking so the kernel event loop is never stalled.
    // Accumulate into a static line buffer; dispatch on CR/LF.
    {
      using namespace dev;

      // Static line buffer — persistent across event-loop iterations.
      static char s_line[64];
      static int  s_line_len = 0;

      while (pl011_rx_ready(kQemuVirtPl011Base)) {
        const int c = pl011_getchar(kQemuVirtPl011Base);
        if (c < 0) break;

        if (c == '\r' || c == '\n') {
          // Terminate buffer, dispatch, re-display prompt.
          s_line[s_line_len] = '\0';
          pl011_puts(kQemuVirtPl011Base, "\r\n");
          shell_dispatch(s_line, state, steps);
          s_line_len = 0;
          pl011_puts(kQemuVirtPl011Base, "t81> ");
          pl011_flush(kQemuVirtPl011Base);

        } else if (c == 127 || c == '\b') {
          // Backspace: erase last character.
          if (s_line_len > 0) {
            --s_line_len;
            pl011_puts(kQemuVirtPl011Base, "\b \b");
          }

        } else if (s_line_len < static_cast<int>(sizeof(s_line)) - 1) {
          // Printable character — echo and accumulate.
          s_line[s_line_len++] = static_cast<char>(c);
          pl011_putchar(kQemuVirtPl011Base, static_cast<char>(c));
        }
        // Drop characters when buffer is full.
      }
    }

    // Bare-metal: yield to next IRQ.  Hosted: tight loop (bounded by max_steps).
#if defined(__aarch64__) && !defined(__APPLE__)
    if (run_forever) {
      __asm__ volatile("wfi");
    }
#endif
  }
}

}  // namespace t81::ternaryos::hal
