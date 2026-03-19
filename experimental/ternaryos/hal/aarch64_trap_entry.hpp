#pragma once
// experimental/ternaryos/hal/aarch64_trap_entry.hpp
//
// AArch64 SVC exception entry interface for the Axion kernel.
//
// This header defines:
//   AArch64TrapFrame  — the saved-context layout written by the assembly
//                       entry stub in aarch64_exception_vectors.S.
//   Inline helpers    — ESR_EL1 decoding and SvcTrapFrame bridge.
//   Declarations      — axion_kernel_set_kernel_state_for_trap_dispatch(),
//                       axion_kernel_handle_svc_trap_aarch64(),
//                       axion_kernel_install_exception_vectors().
//
// The struct layout is verified by static_assert and by [AC-22n] at runtime.
// The assembly in aarch64_exception_vectors.S must be kept in sync with the
// field offsets defined here; any change to AArch64TrapFrame must be
// accompanied by a matching change to that file.

#include "../kernel/kernel_trap_shim.hpp"

#include <cstddef>
#include <cstdint>

namespace t81::ternaryos::hal {

/// Minimal AArch64 saved context written by the synchronous exception entry
/// in aarch64_exception_vectors.S (offset 0x400: Lower EL, AArch64).
///
/// Save sequence (280 bytes = 35 × 8):
///
///   sub  sp, sp, #280
///   stp  x0,  x1,  [sp,   #0]   // x[ 0]..x[ 1]  offsets   0.. 15
///   ...                           // ...            ...
///   stp  x28, x29, [sp, #224]   // x[28]..x[29]  offsets 224..239
///   str  x30,      [sp, #240]   // x[30]          offset  240
///   mrs  x9, sp_el0;  str x9,  [sp, #248]   // sp_el0  offset 248
///   mrs  x9, elr_el1; str x9,  [sp, #256]   // elr_el1 offset 256
///   mrs  x9, spsr_el1;str x9,  [sp, #264]   // spsr_el1 offset 264
///   mrs  x9, esr_el1; str x9,  [sp, #272]   // esr_el1 offset 272
struct AArch64TrapFrame {
    uint64_t x[31];     ///< x0..x30 (x30 = link register).  Offsets 0..240.
    uint64_t sp_el0;    ///< Saved EL0 stack pointer.         Offset 248.
    uint64_t elr_el1;   ///< Exception link register (return address). Offset 256.
    uint64_t spsr_el1;  ///< Saved program status register.   Offset 264.
    uint64_t esr_el1;   ///< Exception syndrome register.     Offset 272.
                        ///<   SVC immediate lives in bits [15:0] (ISS field).
};

static_assert(sizeof(AArch64TrapFrame) == 35 * 8,
              "AArch64TrapFrame must be 35 * 8 = 280 bytes");
static_assert(offsetof(AArch64TrapFrame, sp_el0) == 31 * 8,
              "sp_el0 must be at offset 31 * 8 = 248");
static_assert(offsetof(AArch64TrapFrame, elr_el1) == 32 * 8,
              "elr_el1 must be at offset 32 * 8 = 256");
static_assert(offsetof(AArch64TrapFrame, spsr_el1) == 33 * 8,
              "spsr_el1 must be at offset 33 * 8 = 264");
static_assert(offsetof(AArch64TrapFrame, esr_el1) == 34 * 8,
              "esr_el1 must be at offset 34 * 8 = 272");

// ── ESR_EL1 helpers ──────────────────────────────────────────────────────────

/// Extract the SVC immediate from ESR_EL1.
///
/// For an SVC instruction from AArch64 state the EC field is 0x15 (bits
/// [31:26]) and the ISS field carries the 16-bit SVC immediate in bits [15:0].
[[nodiscard]] inline uint32_t
aarch64_svc_imm_from_esr(uint64_t esr_el1) noexcept {
    return static_cast<uint32_t>(esr_el1 & 0xFFFFu);
}

/// Extract the exception class (EC) from ESR_EL1 (bits [31:26]).
[[nodiscard]] inline uint32_t
aarch64_ec_from_esr(uint64_t esr_el1) noexcept {
    return static_cast<uint32_t>((esr_el1 >> 26) & 0x3Fu);
}

/// EC value for SVC from AArch64 state (ARM DDI 0487, Table D17-2).
inline constexpr uint32_t kAArch64EcSvc64 = 0x15u;

// ── SvcTrapFrame bridge ──────────────────────────────────────────────────────

/// Convert the raw AArch64 trap frame into the kernel's SvcTrapFrame:
///   request_tva  ← x0
///   response_tva ← x1
///   svc_imm      ← ESR_EL1[15:0]
///
/// This is the single authoritative translation between the assembly save
/// convention and the hosted kernel ABI (RFC-00B6 §5.2).
[[nodiscard]] inline kernel::SvcTrapFrame
axion_kernel_svc_frame_from_aarch64(const AArch64TrapFrame& raw) noexcept {
    return kernel::SvcTrapFrame{
        .request_tva  = raw.x[0],
        .response_tva = raw.x[1],
        .svc_imm      = static_cast<uint16_t>(aarch64_svc_imm_from_esr(raw.esr_el1)),
    };
}

// ── Dispatch and install interface ──────────────────────────────────────────

/// Register the KernelRuntimeState to use for SVC trap dispatch.
/// Must be called (with a non-null state) before installing the exception
/// vector table.  Passing nullptr disarms the dispatch path.
void axion_kernel_set_kernel_state_for_trap_dispatch(
    kernel::KernelRuntimeState* state) noexcept;

/// Called by the AArch64 exception entry (axion_svc_entry in
/// aarch64_exception_vectors.S) with a pointer to the saved AArch64TrapFrame.
/// Converts the frame to SvcTrapFrame and delegates to
/// axion_kernel_handle_svc_trap().
///
/// Safe to call on non-AArch64 host builds with a synthetic frame for testing.
/// Null frame or null registered state is handled gracefully (no-op).
void axion_kernel_handle_svc_trap_aarch64(
    const AArch64TrapFrame* frame) noexcept;

/// Write the Axion exception vector table base address into VBAR_EL1.
///
/// On AArch64 builds this executes `msr vbar_el1, <addr>` where <addr> is
/// the address of axion_exception_vector_base from aarch64_exception_vectors.S.
/// On non-AArch64 host builds this is a documented no-op; it may be called
/// freely in tests without side-effects.
void axion_kernel_install_exception_vectors() noexcept;

}  // namespace t81::ternaryos::hal
