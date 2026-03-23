// experimental/ternaryos/kernel/kernel_trap_shim.cpp
//
// Slice 4 — Syscall trap wiring (RFC-00B6 §5.2 / RFC-00B0 §3.4.2).
//
// Hosted simulation of the ARM `svc` exception entry path.
// On real AArch64:
//   - svc #0 → VBAR_EL1 + 0x400 → exception handler
//   - x0 = request_tva, x1 = response_tva (Axion calling convention)
//   - kernel dispatches axion_kernel_call_wire_tva(), returns via eret
//
// Here we accept an SvcTrapFrame directly and route it through the same
// wire-TVA path without replicating any dispatch logic.
//
// RFC-00B6 §5.2: encoding choice (TISC opcode, governed event, narrow trap
// shim) is deferred; the ABI shape (one entry, request+response TVA) is not.
// RFC-00B0 §3.4.2: trap handling stays in the HAL shadow dispatch table;
// the frozen TISC ISA is never mutated.

#include "kernel_trap_shim.hpp"
#include "kernel_abi_wire.hpp"
#include "kernel_runtime_state.hpp"

namespace t81::ternaryos::kernel {

SvcTrapResult axion_kernel_handle_svc_trap(KernelRuntimeState& state,
                                           const SvcTrapFrame& frame) noexcept {
  SvcTrapResult result{};

  // RFC-00B6 §5.2 — Axion convention: svc_imm must be 0 (single unified entry).
  // Any non-zero immediate is an unknown trap kind; reject without dispatch.
  // The caller observes svc_imm_rejected=true and dispatched=false. Writing a
  // structured response is deferred until the address-space resolver is
  // promoted to a public header (RFC encoding choice is explicitly deferred).
  if (frame.svc_imm != 0) {
    result.svc_imm_rejected = true;
    return result;
  }

  // Delegate through the existing wire-TVA path (RFC-00B6 §5.2 entry shape):
  //   request_tva  → read  KernelCallWireRequestBlock  from physical_page_storage
  //   response_tva → write KernelCallWireResponseBlock into physical_page_storage
  // Span validation, address-space resolution, and structured error responses
  // for invalid TVAs are all handled inside axion_kernel_call_wire_tva().
  const bool ok =
      axion_kernel_call_wire_tva(state, frame.request_tva, frame.response_tva);

  if (ok) {
    ++state.counters.syscall_trap_dispatches;
    result.trap_sequence = state.counters.syscall_trap_dispatches;
    result.dispatched = true;
  }

  return result;
}

}  // namespace t81::ternaryos::kernel
