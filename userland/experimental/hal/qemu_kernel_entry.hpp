#pragma once

// experimental/ternaryos/hal/qemu_kernel_entry.hpp
//
// QEMU virt AArch64 bare-metal boot trampoline.
//
// Ties together the three hardware drivers (GICv3, PL011, ARM generic timer)
// and the Axion kernel event loop for a fully self-contained QEMU virt guest.
//
// Execution flow after ExitBootServices:
//
//   qemu_hardware_init()
//     1. pl011_init()                — serial console before anything else
//     2. gicv3_init()                — distributor + redistributor + CPU interface
//     3. gicv3_enable_ppi(14)        — ARM generic timer physical PPI → INTID 30
//     4. axion_kernel_install_exception_vectors() — write VBAR_EL1
//     5. arm_timer_start()           — arm CNTP with configured period
//
//   qemu_kernel_run_loop()
//     - Registers the Timer HAL handler to drive axion_kernel_tick()
//     - Enters the kernel step loop; spins on WFI on bare-metal
//     - On hosted builds runs max_steps iterations for testability
//
// The assembly stub axion_irq_entry (aarch64_exception_vectors.S offset 0x280)
// calls axion_irq_handler_aarch64() (defined here, C linkage) to acknowledge
// the GIC interrupt, reload the timer, dispatch via the HAL handler table,
// then perform EOI.

#include "qemu_platform.hpp"
#include "../kernel/kernel_runtime_state.hpp"

#include <cstdint>

namespace t81::ternaryos::hal {

// ── ARM generic timer constants ──────────────────────────────────────────────

/// QEMU virt AArch64 CNTFRQ_EL0 is 62.5 MHz by default.
inline constexpr uint32_t kQemuCntFreqHz = 62'500'000u;

/// Default timer period: 10 ms at 62.5 MHz = 625 000 counts.
inline constexpr uint32_t kQemuDefaultTimerPeriod = 625'000u;

// ── Hardware initialisation ──────────────────────────────────────────────────

/// Perform QEMU virt AArch64 hardware initialisation at EL1.
///
/// Must be called after ExitBootServices (EFI boot services are gone).
/// Sequence: PL011 console → GICv3 → exception vectors → ARM timer.
///
/// @param profile           QEMU virt profile (used for device addresses).
/// @param timer_period_counts  CNTP_TVAL reload value (counts at CNTFRQ_EL0).
///                             Defaults to kQemuDefaultTimerPeriod (10 ms).
///
/// Returns true on AArch64 bare-metal non-Apple builds where MMIO is live.
/// Returns false (no-op) on hosted macOS / x86 builds — safe to call anyway.
bool qemu_hardware_init(
    const QemuProfile& profile = QemuProfile{},
    uint32_t timer_period_counts = kQemuDefaultTimerPeriod) noexcept;

// ── Kernel event loop ────────────────────────────────────────────────────────

/// Run the Axion kernel event loop driven by ARM generic timer IRQs.
///
/// Registers a HAL Timer interrupt handler that calls axion_kernel_tick() and
/// axion_kernel_record_interrupt() each time the physical timer fires, then
/// processes pending interrupts/faults/pager work via axion_kernel_step().
///
/// @param state               Bootstrapped KernelRuntimeState.
/// @param timer_period_counts Timer reload period (must match qemu_hardware_init).
/// @param max_steps           Stop after this many steps (0 = run forever).
///                            max_steps > 0 is for hosted unit tests only.
///
/// On bare-metal (max_steps == 0) this function never returns.
/// Between steps the kernel executes WFI to yield to the next IRQ.
void qemu_kernel_run_loop(
    kernel::KernelRuntimeState& state,
    uint32_t timer_period_counts = kQemuDefaultTimerPeriod,
    uint64_t max_steps = 0) noexcept;

}  // namespace t81::ternaryos::hal

// ── C-linkage IRQ dispatcher (called from axion_irq_entry in .S) ─────────────

/// Called by the AArch64 IRQ trampoline (axion_irq_entry, offset 0x280 in the
/// VBAR_EL1 vector table).  Must be extern "C" to avoid name mangling.
///
/// Sequence:
///   1. gicv3_acknowledge()     — read ICC_IAR1_EL1 → INTID
///   2. Spurious check (1023)   — return immediately if spurious
///   3. Timer path (INTID 30)   — reload CNTP_TVAL_EL0, source = Timer
///   4. dispatch_interrupt()    — route through HAL handler table
///   5. gicv3_eoi(intid)        — write ICC_EOIR1_EL1
extern "C" void axion_irq_handler_aarch64() noexcept;
