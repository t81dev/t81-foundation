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

#include <cstring>

namespace t81::ternaryos::kernel {

SvcTrapResult axion_kernel_handle_svc_trap(KernelRuntimeState& state,
                                           const SvcTrapFrame& frame) noexcept {
  SvcTrapResult result{};

  // RFC-00B6 §5.2 — Axion convention: svc_imm must be 0 (single unified entry).
  // Any other immediate is an unknown trap kind; reject without dispatch.
  if (frame.svc_imm != 0) {
    result.svc_imm_rejected = true;
    // Attempt to write a structured wire error response so the caller can
    // inspect the rejection rather than hanging on an uninitialised buffer.
    // If the response TVA is unmapped we silently skip the write — the caller
    // will observe a stale/zero response, which is safe.
    const auto caller_as = resolve_current_caller_address_space(state);
    if (caller_as.has_value()) {
      KernelCallResult err_result{};
      err_result.status    = KernelCallStatus::InvalidRequest;
      err_result.rejection = KernelCallRejection::None;
      const auto response_block = axion_kernel_encode_wire_response(err_result);
      axion_kernel_write_address_space_bytes(
          state,
          *caller_as,
          frame.response_tva,
          reinterpret_cast<const std::byte*>(&response_block),
          sizeof(response_block));
    }
    return result;
  }

  // Delegate through the existing wire-TVA path (RFC-00B6 §5.2 entry shape).
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
