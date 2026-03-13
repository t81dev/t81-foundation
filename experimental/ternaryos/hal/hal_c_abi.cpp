// experimental/ternaryos/hal/hal_c_abi.cpp

#include "hal_c_abi.h"

#include "hal.hpp"
#include "../kernel/kernel_abi_wire.hpp"
#include "../kernel/kernel_main.hpp"

namespace t81::ternaryos::hal {
namespace {

BootContext from_c_boot_context(const TernaryOsBootContext& c_ctx) {
  BootContext ctx;
  ctx.kernel_load_address = c_ctx.kernel_load_address;
  ctx.stack_top = c_ctx.stack_top;
  ctx.ethics_boot_required = c_ctx.ethics_boot_required;
  if (c_ctx.platform_id != nullptr) {
    ctx.platform_id = c_ctx.platform_id;
  }

  ctx.memory_map.reserve(c_ctx.memory_map_len);
  for (std::size_t i = 0; i < c_ctx.memory_map_len; ++i) {
    const auto& region = c_ctx.memory_map[i];
    ctx.memory_map.push_back(MemoryRegion{
        .base_phys = region.base_phys,
        .size_bytes = region.size_bytes,
        .writable = region.writable,
        .executable = region.executable,
    });
  }
  return ctx;
}

}  // namespace
}  // namespace t81::ternaryos::hal

extern "C" int ternaryos_hal_main_c(const TernaryOsBootContext* ctx) {
  if (ctx == nullptr || (ctx->memory_map_len > 0 && ctx->memory_map == nullptr)) {
    return -1;
  }
  return t81::ternaryos::hal::hal_main(
      t81::ternaryos::hal::from_c_boot_context(*ctx));
}

extern "C" void* ternaryos_kernel_bootstrap_c(const TernaryOsBootContext* ctx) {
  if (ctx == nullptr || (ctx->memory_map_len > 0 && ctx->memory_map == nullptr)) {
    return nullptr;
  }

  auto state = t81::ternaryos::kernel::axion_kernel_bootstrap(
      t81::ternaryos::hal::from_c_boot_context(*ctx));
  if (!state.has_value()) {
    return nullptr;
  }

  return new t81::ternaryos::kernel::KernelRuntimeState(std::move(*state));
}

extern "C" void ternaryos_kernel_destroy_c(void* kernel_state) {
  delete static_cast<t81::ternaryos::kernel::KernelRuntimeState*>(kernel_state);
}

extern "C" int ternaryos_kernel_call_c(void* kernel_state,
                                       const void* request_bytes,
                                       size_t request_size,
                                       void* response_bytes,
                                       size_t response_size) {
  if (kernel_state == nullptr) {
    return -1;
  }
  auto* state =
      static_cast<t81::ternaryos::kernel::KernelRuntimeState*>(kernel_state);
  return t81::ternaryos::kernel::axion_kernel_call_wire_bytes(
             *state, request_bytes, request_size, response_bytes, response_size)
             ? 0
             : -1;
}
