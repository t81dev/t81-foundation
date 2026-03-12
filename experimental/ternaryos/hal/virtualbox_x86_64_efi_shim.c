// experimental/ternaryos/hal/virtualbox_x86_64_efi_shim.c
//
// Temporary x86_64 developer-lane shim for producing a linkable BOOTX64.EFI
// for local QEMU diagnostics. This is intentionally not the real HAL handoff
// bridge used for final promotion proof on an external VirtualBox host.

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
