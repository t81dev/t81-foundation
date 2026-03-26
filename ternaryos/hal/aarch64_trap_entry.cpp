// experimental/ternaryos/hal/aarch64_trap_entry.cpp
//
// Host-safe implementations of the AArch64 SVC trap entry helpers declared
// in aarch64_trap_entry.hpp.
//
// axion_kernel_install_exception_vectors() writes VBAR_EL1 on real AArch64
// hardware and is a documented no-op on non-AArch64 host builds so that the
// HAL test binary can call it without side-effects.

#include "aarch64_trap_entry.hpp"
#include "../kernel/kernel_main.hpp"

namespace t81::ternaryos::hal {

namespace {

/// Global KernelRuntimeState pointer used by axion_kernel_handle_svc_trap_aarch64().
/// Set via axion_kernel_set_kernel_state_for_trap_dispatch() before installing
/// the exception vector table.
kernel::KernelRuntimeState* g_trap_dispatch_state{nullptr};

}  // namespace

void axion_kernel_set_kernel_state_for_trap_dispatch(kernel::KernelRuntimeState* state) noexcept {
  g_trap_dispatch_state = state;
}

void axion_kernel_handle_svc_trap_aarch64(const AArch64TrapFrame* frame) noexcept {
  if (!g_trap_dispatch_state || !frame) {
    return;
  }
  const auto svc_frame = axion_kernel_svc_frame_from_aarch64(*frame);
  axion_kernel_handle_svc_trap(*g_trap_dispatch_state, svc_frame);
}

void axion_kernel_install_exception_vectors() noexcept {
  // Only execute on bare-metal AArch64 targets (QEMU / real hardware).
  // Excluded on macOS/Apple Silicon: the host OS runs at EL0 and VBAR_EL1
  // is privileged; attempting to write it would fault immediately.
#if defined(__aarch64__) && !defined(__APPLE__) && !defined(__MACH__)
  // axion_exception_vector_base is the symbol defined by the .balign 2048 /
  // .global directive in aarch64_exception_vectors.S.  Writing its address
  // into VBAR_EL1 installs the Axion exception vector table.
  extern "C" char axion_exception_vector_base[];
  asm volatile("msr vbar_el1, %0"
               :
               : "r"(static_cast<void*>(axion_exception_vector_base))
               : "memory");
#endif
  // Documented no-op on all other builds (host macOS, host x86-64, MSVC).
}

}  // namespace t81::ternaryos::hal
