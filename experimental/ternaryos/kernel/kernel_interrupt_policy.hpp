#pragma once

// RFC-00B5 §3.7 — Interrupt Policy Gate (Slice 27)
//
// evaluate_interrupt_policy() runs before every interrupt is dispatched to its
// source-specific handler.  It returns one of three verdicts:
//
//   Allow      — interrupt passes; dispatch proceeds normally.
//   Quarantine — rate limit just exceeded; source is marked quarantined for
//                all future deliveries and this interrupt is dropped.
//   Deny       — source is already quarantined; interrupt dropped without
//                touching any handler.
//
// record_interrupt_policy_event() writes the verdict to the kernel audit log
// and retains the last verdict on the KernelRuntimeState for test inspection.

#include "kernel_runtime_state.hpp"

namespace t81::ternaryos::kernel {

/// Evaluate the interrupt policy gate for the given already-delivered interrupt.
/// Called from axion_kernel_deliver_pending_interrupt() BEFORE the
/// source-specific switch.  May mutate state.interrupt_policy[src] rate-window
/// state.  Does NOT increment counters — the caller does that after inspecting
/// the verdict.
[[nodiscard]] InterruptPolicyVerdict evaluate_interrupt_policy(
    KernelRuntimeState& state,
    const KernelInterruptRecord& interrupt) noexcept;

/// Record a policy verdict in the audit log and retain it on state.
/// Always called immediately after evaluate_interrupt_policy().
void record_interrupt_policy_event(
    KernelRuntimeState& state,
    InterruptPolicyVerdict verdict,
    const KernelInterruptRecord& interrupt) noexcept;

}  // namespace t81::ternaryos::kernel
