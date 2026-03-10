// experimental/ternaryos/hal/virtualbox_efi_stub.c
//
// Freestanding VirtualBox EFI guest stub for the first TernOS promotion path.
// This is intentionally minimal: it constructs the first supported VBox guest
// boot context and hands off through the C ABI bridge into hal_main.

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

void* memcpy(void* dest, const void* src, unsigned long n) {
  unsigned char*       dst = (unsigned char*)dest;
  const unsigned char* in = (const unsigned char*)src;
  unsigned long        i = 0;
  while (i < n) {
    dst[i] = in[i];
    ++i;
  }
  return dest;
}

static const TernaryOsMemoryRegion kVirtualBoxMemoryMap[] = {
    {
        .base_phys = 0x0000000000100000ULL,
        .size_bytes = 128ULL * 1024ULL * 1024ULL,
        .writable = true,
        .executable = false,
    },
    {
        .base_phys = 0x0000000008000000ULL,
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
    "virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC";

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  (void)image_handle;
  (void)system_table;

  TernaryOsBootContext ctx;
  ctx.memory_map = kVirtualBoxMemoryMap;
  ctx.memory_map_len = sizeof(kVirtualBoxMemoryMap) / sizeof(kVirtualBoxMemoryMap[0]);
  ctx.kernel_load_address = 0x0000000008000000ULL;
  ctx.stack_top = 0x0000000007FFF000ULL;
  ctx.ethics_boot_required = true;
  ctx.platform_id = kPlatformId;

  return ternaryos_hal_main_c(&ctx) == 0 ? EFI_SUCCESS : EFI_LOAD_ERROR;
}
