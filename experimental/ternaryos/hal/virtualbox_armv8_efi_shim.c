// experimental/ternaryos/hal/virtualbox_armv8_efi_shim.c
//
// Temporary ARMv8 developer-lane shim for producing a linkable BOOTAA64.EFI on
// Apple Silicon hosts. This is intentionally not the real HAL handoff path.
// The official promotion target remains the x86_64 VBox EFI route that must
// eventually link against the true C++ HAL bridge.

#include "hal_c_abi.h"

int ternaryos_hal_main_c(const TernaryOsBootContext* ctx) {
  if (ctx == 0) {
    return -1;
  }
  if (ctx->memory_map_len > 0 && ctx->memory_map == 0) {
    return -1;
  }
  if (ctx->platform_id == 0) {
    return -1;
  }
  return 0;
}
