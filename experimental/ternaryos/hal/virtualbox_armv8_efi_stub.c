// experimental/ternaryos/hal/virtualbox_armv8_efi_stub.c
//
// Freestanding ARMv8 VirtualBox EFI guest stub for the secondary developer
// lane. This does not redefine the official x86_64 roadmap target; it exists
// so Apple Silicon hosts can exercise the EFI handoff path as far as possible.

#include "hal_c_abi.h"

#include <stdint.h>

typedef void* EFI_HANDLE;
typedef uint64_t EFI_STATUS;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

#ifndef EFIAPI
#define EFIAPI
#endif

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 1

static const TernaryOsMemoryRegion kArmv8VirtualBoxMemoryMap[] = {
    {
        .base_phys = 0x0000000040000000ULL,
        .size_bytes = 128ULL * 1024ULL * 1024ULL,
        .writable = true,
        .executable = false,
    },
    {
        .base_phys = 0x0000000048000000ULL,
        .size_bytes = 4ULL * 1024ULL * 1024ULL,
        .writable = true,
        .executable = true,
    },
    {
        .base_phys = 0x00000000F0000000ULL,
        .size_bytes = 16ULL * 1024ULL * 1024ULL,
        .writable = false,
        .executable = false,
    },
};

static const char kPlatformId[] =
    "virtualbox-armv8:ARMv8Virtual/developer-lane";

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  (void)image_handle;
  (void)system_table;

  TernaryOsBootContext ctx = {
      .memory_map = kArmv8VirtualBoxMemoryMap,
      .memory_map_len = sizeof(kArmv8VirtualBoxMemoryMap) / sizeof(kArmv8VirtualBoxMemoryMap[0]),
      .kernel_load_address = 0x0000000048000000ULL,
      .stack_top = 0x0000000047FFF000ULL,
      .ethics_boot_required = 1,
      .platform_id = kPlatformId,
  };

  return ternaryos_hal_main_c(&ctx) == 0 ? EFI_SUCCESS : EFI_LOAD_ERROR;
}
