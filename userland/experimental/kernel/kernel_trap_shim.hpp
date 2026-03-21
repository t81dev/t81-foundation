#pragma once

// experimental/ternaryos/kernel/kernel_trap_shim.hpp
//
// Slice 4 — Syscall trap wiring (RFC-00B6 §5.2 / RFC-00B0 §3.4.2).
//
// Models the ARM `svc` exception entry path in the hosted simulation.
// On real AArch64 hardware the CPU would:
//   1. save EL0 context (x0–x30, SP_EL0, ELR_EL1, SPSR_EL1)
//   2. jump to VBAR_EL1 + 0x400 (synchronous exception from EL0/SP_EL0)
//   3. kernel exception handler decodes `svc #imm`; imm == 0 means Axion kernel call
//   4. x0 = request_tva, x1 = response_tva (Axion calling convention)
//   5. calls axion_kernel_call_wire_tva(state, request_tva, response_tva)
//   6. returns via `eret`
//
// In the hosted simulation, callers construct an SvcTrapFrame directly and
// pass it to axion_kernel_handle_svc_trap(). The shim enforces the svc_imm==0
// convention, validates both TVAs exist in the caller address space, and
// delegates to axion_kernel_call_wire_tva().
//
// RFC-00B6 §5.2: "one logical entrypoint: kernel_call(request_ptr, response_ptr)"
// RFC-00B5 §3.6: continuation is a scheduler decision after the trap handler.
// RFC-00B0 §3.4.2: TISC has no trap-return opcode; the HAL shadow dispatch
//   table catches hardware traps before any TISC-visible action.

#include "kernel_abi.hpp"

#include <cstdint>

namespace t81::ternaryos::kernel {

struct KernelRuntimeState;

/// Decoded ARM SVC trap frame — the minimal context needed to route
/// a user-mode `svc` exception into the typed kernel-call path.
struct SvcTrapFrame {
  /// TVA of the KernelCallWireRequestBlock in the caller's address space.
  /// Corresponds to x0 at the point of the `svc` instruction.
  uint64_t request_tva{0};

  /// TVA of the KernelCallWireResponseBlock in the caller's address space.
  /// Corresponds to x1 at the point of the `svc` instruction.
  uint64_t response_tva{0};

  /// SVC immediate field (low 16 bits of the `svc #imm` encoding).
  /// Axion convention: imm == 0 is the single unified kernel-call entry.
  /// Any non-zero value is rejected with a structured wire error response.
  uint16_t svc_imm{0};
};

/// Result returned by axion_kernel_handle_svc_trap().
struct SvcTrapResult {
  /// True if the trap was dispatched through axion_kernel_call_wire_tva().
  bool dispatched{false};

  /// True if the trap was rejected because svc_imm != 0.
  bool svc_imm_rejected{false};

  /// Monotone trap-dispatch sequence number (from state.counters).
  /// Zero if the trap was rejected before dispatch.
  uint64_t trap_sequence{0};
};

/// Handle an ARM SVC trap in the hosted simulation.
///
/// Enforces the Axion kernel-call convention (svc_imm == 0), then delegates
/// to axion_kernel_call_wire_tva() which reads the wire request block from
/// physical_page_storage, dispatches through axion_kernel_call(), and writes
/// the wire response block back.
///
/// On svc_imm rejection, writes a structured InvalidRequest wire response to
/// response_tva if that TVA is currently mapped and writable.
SvcTrapResult axion_kernel_handle_svc_trap(KernelRuntimeState& state,
                                           const SvcTrapFrame& frame) noexcept;

}  // namespace t81::ternaryos::kernel
