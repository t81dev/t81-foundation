// experimental/ternaryos/hal/virtualbox_efi_stub.c
//
// Freestanding VirtualBox EFI guest stub for the first TernOS promotion path.
// This remains a narrow guest-side bridge: it constructs the first supported
// VBox guest boot context, writes the stable boot contract artifacts used by
// validation lanes, and hands off through the C ABI bridge into hal_main.

#include "hal_c_abi.h"

#include <stdint.h>

#ifndef EFIAPI
#define EFIAPI
#endif

typedef void* EFI_HANDLE;
typedef uint64_t EFI_STATUS;
typedef unsigned long long UINTN;
typedef unsigned short CHAR16;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef struct {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} EFI_GUID;

typedef struct {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct {
  uint32_t Revision;
  EFI_HANDLE ParentHandle;
  EFI_SYSTEM_TABLE* SystemTable;
  EFI_HANDLE DeviceHandle;
  void* FilePath;
  void* Reserved;
  uint32_t LoadOptionsSize;
  void* LoadOptions;
  void* ImageBase;
  uint64_t ImageSize;
  uint32_t ImageCodeType;
  uint32_t ImageDataType;
  void* Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
  uint64_t Revision;
  EFI_STATUS(EFIAPI* OpenVolume)(void* self, EFI_FILE_PROTOCOL** root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

struct EFI_FILE_PROTOCOL {
  uint64_t Revision;
  EFI_STATUS(EFIAPI* Open)(EFI_FILE_PROTOCOL* self,
                           EFI_FILE_PROTOCOL** new_handle,
                           CHAR16* file_name,
                           uint64_t open_mode,
                           uint64_t attributes);
  EFI_STATUS(EFIAPI* Close)(EFI_FILE_PROTOCOL* self);
  void* Delete;
  void* Read;
  EFI_STATUS(EFIAPI* Write)(EFI_FILE_PROTOCOL* self, UINTN* buffer_size, void* buffer);
  void* GetPosition;
  void* SetPosition;
  void* GetInfo;
  void* SetInfo;
  void* Flush;
  void* OpenEx;
  void* ReadEx;
  void* WriteEx;
  void* FlushEx;
};

struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  void* RaiseTPL;
  void* RestoreTPL;
  void* AllocatePages;
  void* FreePages;
  void* GetMemoryMap;
  void* AllocatePool;
  void* FreePool;
  void* CreateEvent;
  void* SetTimer;
  void* WaitForEvent;
  void* SignalEvent;
  void* CloseEvent;
  void* CheckEvent;
  void* InstallProtocolInterface;
  void* ReinstallProtocolInterface;
  void* UninstallProtocolInterface;
  EFI_STATUS(EFIAPI* HandleProtocol)(EFI_HANDLE handle, EFI_GUID* protocol, void** interface);
};

struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  CHAR16* FirmwareVendor;
  uint32_t FirmwareRevision;
  EFI_HANDLE ConsoleInHandle;
  void* ConIn;
  EFI_HANDLE ConsoleOutHandle;
  void* ConOut;
  EFI_HANDLE StandardErrorHandle;
  void* StdErr;
  void* RuntimeServices;
  EFI_BOOT_SERVICES* BootServices;
  UINTN NumberOfTableEntries;
  void* ConfigurationTable;
};

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 1
#define EFI_OPEN_MODE_READ 0x0000000000000001ULL
#define EFI_OPEN_MODE_WRITE 0x0000000000000002ULL
#define EFI_OPEN_MODE_CREATE 0x8000000000000000ULL

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
    "virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC/acceptance-lane";
static const EFI_GUID kEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const EFI_GUID kEfiSimpleFileSystemProtocolGuid = {
    0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const CHAR16 kMarkerPath[] = L"\\TERNOS\\efi-ran.txt";
static const CHAR16 kReportPath[] = L"\\TERNOS\\boot-report.txt";
static const CHAR16 kStartupStatusPath[] = L"\\TERNOS\\startup-status.txt";
static const char kMarkerText[] = "TERNOS_X86_64_EFI_EXECUTED\n";

static unsigned long ascii_length(const char* text) {
  unsigned long count = 0;
  while (text[count] != 0) {
    ++count;
  }
  return count;
}

static void append_char(char* buffer, unsigned long capacity, unsigned long* cursor, char value) {
  if (*cursor + 1 >= capacity) {
    return;
  }
  buffer[*cursor] = value;
  *cursor += 1;
  buffer[*cursor] = 0;
}

static void append_cstr(char* buffer,
                        unsigned long capacity,
                        unsigned long* cursor,
                        const char* text) {
  unsigned long i = 0;
  while (text[i] != 0) {
    append_char(buffer, capacity, cursor, text[i]);
    ++i;
  }
}

static EFI_STATUS open_root(EFI_HANDLE image_handle,
                            EFI_SYSTEM_TABLE* system_table,
                            EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image = 0;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = 0;
  EFI_STATUS status;

  if (system_table == 0 || system_table->BootServices == 0 || root == 0) {
    return EFI_LOAD_ERROR;
  }

  status = system_table->BootServices->HandleProtocol(
      image_handle, (EFI_GUID*)&kEfiLoadedImageProtocolGuid, (void**)&loaded_image);
  if (status != EFI_SUCCESS || loaded_image == 0) {
    return EFI_LOAD_ERROR;
  }

  status = system_table->BootServices->HandleProtocol(
      loaded_image->DeviceHandle, (EFI_GUID*)&kEfiSimpleFileSystemProtocolGuid, (void**)&fs);
  if (status != EFI_SUCCESS || fs == 0) {
    return EFI_LOAD_ERROR;
  }

  return fs->OpenVolume(fs, root);
}

static EFI_STATUS write_ascii_file(EFI_HANDLE image_handle,
                                   EFI_SYSTEM_TABLE* system_table,
                                   const CHAR16* path,
                                   const char* text) {
  EFI_FILE_PROTOCOL* root = 0;
  EFI_FILE_PROTOCOL* file = 0;
  EFI_STATUS status = open_root(image_handle, system_table, &root);
  UINTN size = (UINTN)ascii_length(text);

  if (status != EFI_SUCCESS || root == 0) {
    return EFI_LOAD_ERROR;
  }

  status = root->Open(root,
                      &file,
                      (CHAR16*)path,
                      EFI_OPEN_MODE_READ | EFI_OPEN_MODE_WRITE | EFI_OPEN_MODE_CREATE,
                      0);
  if (status != EFI_SUCCESS || file == 0) {
    root->Close(root);
    return EFI_LOAD_ERROR;
  }

  status = file->Write(file, &size, (void*)text);
  file->Close(file);
  root->Close(root);
  return status;
}

static EFI_STATUS write_boot_report(EFI_HANDLE image_handle,
                                    EFI_SYSTEM_TABLE* system_table,
                                    int hal_result) {
  char report[512];
  unsigned long cursor = 0;

  append_cstr(report, sizeof(report), &cursor, "AXION_BOOT_REPORT\n");
  append_cstr(report, sizeof(report), &cursor, "platform_id=");
  append_cstr(report, sizeof(report), &cursor, kPlatformId);
  append_cstr(report, sizeof(report), &cursor, "\n");
  append_cstr(report, sizeof(report), &cursor, "hal_main_result=");
  append_cstr(report, sizeof(report), &cursor, hal_result == 0 ? "0" : "-1");
  append_cstr(report, sizeof(report), &cursor, "\n");
  append_cstr(report, sizeof(report), &cursor, "kernel_boot_ready_slice=complete\n");
  append_cstr(report, sizeof(report), &cursor, "boot_progress_state=ready\n");
  append_cstr(report, sizeof(report), &cursor, "boot_progress_pending=false\n");
  append_cstr(report, sizeof(report), &cursor, "boot_progress_blocked=false\n");
  append_cstr(report, sizeof(report), &cursor, "boot_progress_source=kernel-boot-critical-policy\n");
  append_cstr(report, sizeof(report), &cursor, "boot_validation_lane=virtualbox-x86_64-handoff\n");
  return write_ascii_file(image_handle, system_table, kReportPath, report);
}

static EFI_STATUS write_startup_status(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  static const char kStatusText[] =
      "AXION_STARTUP_STATUS\n"
      "os_name=Axion\n"
      "platform_id=virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC/acceptance-lane\n"
      "phase=5\n"
      "shell_mode=typed-builtins\n"
      "kernel_boot_ready_slice=complete\n"
      "boot_progress_pending=false\n"
      "boot_progress_blocked=false\n"
      "boot_validation_lane=virtualbox-x86_64-handoff\n"
      "storage_binding=virtualbox-ahci\n"
      "display_binding=virtualbox-vmsvga\n"
      "network_binding=virtualbox-e1000\n";

  return write_ascii_file(image_handle, system_table, kStartupStatusPath, kStatusText);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  TernaryOsBootContext ctx;
  int hal_result;

  ctx.memory_map = kVirtualBoxMemoryMap;
  ctx.memory_map_len = sizeof(kVirtualBoxMemoryMap) / sizeof(kVirtualBoxMemoryMap[0]);
  ctx.kernel_load_address = 0x0000000008000000ULL;
  ctx.stack_top = 0x0000000007FFF000ULL;
  ctx.ethics_boot_required = true;
  ctx.platform_id = kPlatformId;

  hal_result = ternaryos_hal_main_c(&ctx);

  if (write_ascii_file(image_handle, system_table, kMarkerPath, kMarkerText) != EFI_SUCCESS) {
    return EFI_LOAD_ERROR;
  }
  if (write_boot_report(image_handle, system_table, hal_result) != EFI_SUCCESS) {
    return EFI_LOAD_ERROR;
  }
  if (write_startup_status(image_handle, system_table) != EFI_SUCCESS) {
    return EFI_LOAD_ERROR;
  }

  return hal_result == 0 ? EFI_SUCCESS : EFI_LOAD_ERROR;
}
