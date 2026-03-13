#pragma once

// experimental/ternaryos/hal/hal_c_abi.h
//
// C-compatible HAL entry contract for freestanding guest stubs.
// This is the seam used by the VirtualBox EFI guest stub so it can hand off a
// boot context without depending on the C++ standard library.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TernaryOsMemoryRegion {
  uint64_t base_phys;
  uint64_t size_bytes;
  bool     writable;
  bool     executable;
} TernaryOsMemoryRegion;

typedef struct TernaryOsBootContext {
  const TernaryOsMemoryRegion* memory_map;
  size_t                       memory_map_len;
  uint64_t                     kernel_load_address;
  uint64_t                     stack_top;
  bool                         ethics_boot_required;
  const char*                  platform_id;
} TernaryOsBootContext;

int ternaryos_hal_main_c(const TernaryOsBootContext* ctx);
void* ternaryos_kernel_bootstrap_c(const TernaryOsBootContext* ctx);
void ternaryos_kernel_destroy_c(void* kernel_state);
int ternaryos_kernel_call_c(void* kernel_state,
                            const void* request_bytes,
                            size_t request_size,
                            void* response_bytes,
                            size_t response_size);

#ifdef __cplusplus
}  // extern "C"
#endif
