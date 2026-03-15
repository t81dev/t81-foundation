#include "kernel_interrupt_policy.hpp"
#include "kernel_main.hpp"

namespace t81::ternaryos::kernel {

InterruptPolicyVerdict evaluate_interrupt_policy(
    KernelRuntimeState& state,
    const KernelInterruptRecord& interrupt) noexcept {

  const uint8_t src_key = static_cast<uint8_t>(interrupt.source);
  auto& src = state.interrupt_policy[src_key];  // default-constructs if absent

  // Already quarantined — deny without further accounting.
  if (src.quarantined) {
    return InterruptPolicyVerdict::Deny;
  }

  // No rate limit configured — always allow.
  if (src.config.max_per_window == 0 || src.config.window_size == 0) {
    return InterruptPolicyVerdict::Allow;
  }

  // Rate-limit check: advance or reset the sliding window.
  const uint64_t current_cycle = state.counters.loop_iterations;
  if ((current_cycle - src.window_start_cycle) >= src.config.window_size) {
    src.recent_count       = 0;
    src.window_start_cycle = current_cycle;
  }
  ++src.recent_count;

  if (src.recent_count > src.config.max_per_window) {
    // Threshold crossed — quarantine source and drop this interrupt.
    src.quarantined = true;
    return InterruptPolicyVerdict::Quarantine;
  }

  return InterruptPolicyVerdict::Allow;
}

void record_interrupt_policy_event(
    KernelRuntimeState& state,
    InterruptPolicyVerdict verdict,
    const KernelInterruptRecord& interrupt) noexcept {

  // Always retain the last verdict on state for test inspection.
  state.last_interrupt_policy_verdict  = verdict;
  state.last_interrupt_policy_source   = interrupt.source;
  state.last_interrupt_policy_sequence = interrupt.sequence;

  // Emit an audit record only for security-significant outcomes (Quarantine
  // and Deny).  Allow is the normal path and must NOT displace the
  // InterruptDelivered audit record that precedes this call.
  if (verdict == InterruptPolicyVerdict::Allow) return;

  const KernelAuditEventKind kind =
      (verdict == InterruptPolicyVerdict::Quarantine)
          ? KernelAuditEventKind::InterruptPolicyQuarantine
          : KernelAuditEventKind::InterruptPolicyDeny;

  record_audit_event(state,
                     kind,
                     KernelRuntimeState::kKernelTid,
                     KernelRuntimeState::kKernelProcessGroup);
}

}  // namespace t81::ternaryos::kernel
