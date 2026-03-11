// experimental/ternaryos/hal/virtualbox_armv8_efi_stub.c
//
// Freestanding ARMv8 VirtualBox EFI guest stub for the secondary developer
// lane. This does not redefine the official x86_64 roadmap target; it exists
// so Apple Silicon hosts can exercise the EFI handoff path as far as possible.

#include "hal_c_abi.h"
#include "axion_startup_history.h"
#include "axion_startup_shell.h"
#include "axion_startup_session.h"

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
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

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
  uint32_t Type;
  uint32_t Pad;
  uint64_t PhysicalStart;
  uint64_t VirtualStart;
  uint64_t NumberOfPages;
  uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

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

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void* Reset;
  EFI_STATUS(EFIAPI* OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* self, CHAR16* string);
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
#define EFI_WRITE_PROTECTED 8
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

static const EFI_GUID kEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const EFI_GUID kEfiSimpleFileSystemProtocolGuid = {
    0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const CHAR16 kMarkerPath[] = L"\\TERNOS\\efi-ran.txt";
static const CHAR16 kReportPath[] = L"\\TERNOS\\boot-report.txt";
static const CHAR16 kStartupStatusPath[] = L"\\TERNOS\\startup-status.txt";
static const CHAR16 kStartupShellPath[] = L"\\TERNOS\\startup-shell.txt";
static const CHAR16 kStartupSessionPath[] = L"\\TERNOS\\startup-session.txt";
static const CHAR16 kStartupHistoryPath[] = L"\\TERNOS\\startup-history.txt";
static const char kMarkerText[] = "TERNOS_ARMV8_EFI_EXECUTED\n";
static const char kBootBanner[] = "Axion ARMv8 EFI stub\r\n";

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

static void append_u64_hex(char* buffer,
                           unsigned long capacity,
                           unsigned long* cursor,
                           uint64_t value) {
  static const char kHexDigits[] = "0123456789ABCDEF";
  int               shift = 60;

  append_cstr(buffer, capacity, cursor, "0x");
  while (shift >= 0) {
    const uint64_t nibble = (value >> shift) & 0xFULL;
    append_char(buffer, capacity, cursor, kHexDigits[nibble]);
    shift -= 4;
  }
}

static void append_u64_dec(char* buffer,
                           unsigned long capacity,
                           unsigned long* cursor,
                           uint64_t value) {
  char digits[32];
  unsigned long index = 0;

  if (value == 0) {
    append_char(buffer, capacity, cursor, '0');
    return;
  }

  while (value > 0 && index < sizeof(digits)) {
    digits[index++] = (char)('0' + (value % 10ULL));
    value /= 10ULL;
  }

  while (index > 0) {
    append_char(buffer, capacity, cursor, digits[--index]);
  }
}

static void append_int_dec(char* buffer,
                           unsigned long capacity,
                           unsigned long* cursor,
                           int value) {
  if (value < 0) {
    append_char(buffer, capacity, cursor, '-');
    append_u64_dec(buffer, capacity, cursor, (uint64_t)(-(int64_t)value));
    return;
  }

  append_u64_dec(buffer, capacity, cursor, (uint64_t)value);
}

static void append_bool(char* buffer,
                        unsigned long capacity,
                        unsigned long* cursor,
                        bool value) {
  append_cstr(buffer, capacity, cursor, value ? "true" : "false");
}

static void ascii_to_char16(CHAR16* dest, unsigned long capacity, const char* text) {
  unsigned long i = 0;
  if (capacity == 0) {
    return;
  }
  while (i + 1 < capacity && text[i] != 0) {
    dest[i] = (CHAR16)((unsigned char)text[i]);
    ++i;
  }
  dest[i] = 0;
}

static void emit_boot_banner(EFI_SYSTEM_TABLE* system_table) {
  CHAR16 banner[64];
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con_out = 0;

  if (system_table == 0 || system_table->ConOut == 0) {
    return;
  }

  con_out = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*)system_table->ConOut;
  if (con_out->OutputString == 0) {
    return;
  }

  ascii_to_char16(banner, sizeof(banner) / sizeof(banner[0]), kBootBanner);
  con_out->OutputString(con_out, banner);
}

static EFI_STATUS open_root_volume(EFI_HANDLE image_handle,
                                   EFI_SYSTEM_TABLE* system_table,
                                   EFI_FILE_PROTOCOL** root_out) {
  if (system_table == 0 || system_table->BootServices == 0 ||
      system_table->BootServices->HandleProtocol == 0) {
    return EFI_LOAD_ERROR;
  }

  EFI_LOADED_IMAGE_PROTOCOL* loaded_image = 0;
  EFI_STATUS status = system_table->BootServices->HandleProtocol(
      image_handle, (EFI_GUID*)&kEfiLoadedImageProtocolGuid, (void**)&loaded_image);
  if (status != EFI_SUCCESS || loaded_image == 0 || loaded_image->DeviceHandle == 0) {
    return EFI_LOAD_ERROR;
  }

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = 0;
  status = system_table->BootServices->HandleProtocol(
      loaded_image->DeviceHandle, (EFI_GUID*)&kEfiSimpleFileSystemProtocolGuid, (void**)&fs);
  if (status != EFI_SUCCESS || fs == 0 || fs->OpenVolume == 0) {
    return EFI_LOAD_ERROR;
  }

  EFI_FILE_PROTOCOL* root = 0;
  status = fs->OpenVolume(fs, &root);
  if (status != EFI_SUCCESS || root == 0 || root->Open == 0 || root->Close == 0) {
    return EFI_LOAD_ERROR;
  }

  *root_out = root;
  return EFI_SUCCESS;
}

static EFI_STATUS write_root_file(EFI_FILE_PROTOCOL* root,
                                  const CHAR16* path,
                                  const char* text,
                                  unsigned long text_len) {
  EFI_STATUS status = EFI_LOAD_ERROR;
  if (root == 0 || root->Open == 0 || root->Close == 0) {
    return EFI_LOAD_ERROR;
  }

  EFI_FILE_PROTOCOL* file = 0;
  status = root->Open(root,
                      &file,
                      (CHAR16*)path,
                      EFI_OPEN_MODE_READ | EFI_OPEN_MODE_WRITE | EFI_OPEN_MODE_CREATE,
                      0);
  if (status != EFI_SUCCESS || file == 0 || file->Write == 0 || file->Close == 0) {
    return EFI_WRITE_PROTECTED;
  }

  UINTN bytes = text_len;
  status = file->Write(file, &bytes, (void*)text);
  file->Close(file);
  return status;
}

static EFI_STATUS write_execution_marker(EFI_HANDLE image_handle,
                                         EFI_SYSTEM_TABLE* system_table) {
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS         status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  status = write_root_file(root, kMarkerPath, kMarkerText, sizeof(kMarkerText) - 1);
  root->Close(root);
  return status;
}

static EFI_STATUS write_boot_report(EFI_HANDLE image_handle,
                                    EFI_SYSTEM_TABLE* system_table,
                                    const TernaryOsBootContext* ctx,
                                    int hal_result) {
  char report[768];
  unsigned long cursor = 0;
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  report[0] = 0;
  append_cstr(report, sizeof(report), &cursor, "AXION_ARMV8_BOOT_REPORT\n");
  append_cstr(report, sizeof(report), &cursor, "platform_id=");
  append_cstr(report, sizeof(report), &cursor, ctx->platform_id);
  append_cstr(report, sizeof(report), &cursor, "\nmemory_map_len=");
  append_u64_dec(report, sizeof(report), &cursor, ctx->memory_map_len);
  append_cstr(report, sizeof(report), &cursor, "\nkernel_load_address=");
  append_u64_hex(report, sizeof(report), &cursor, ctx->kernel_load_address);
  append_cstr(report, sizeof(report), &cursor, "\nstack_top=");
  append_u64_hex(report, sizeof(report), &cursor, ctx->stack_top);
  append_cstr(report, sizeof(report), &cursor, "\nethics_boot_required=");
  append_bool(report, sizeof(report), &cursor, ctx->ethics_boot_required);
  append_cstr(report, sizeof(report), &cursor, "\nhal_main_result=");
  append_int_dec(report, sizeof(report), &cursor, hal_result);
  append_cstr(report, sizeof(report), &cursor, "\n");

  status = write_root_file(root, kReportPath, report, ascii_length(report));
  root->Close(root);
  return status;
}

static EFI_STATUS write_startup_status(EFI_HANDLE image_handle,
                                       EFI_SYSTEM_TABLE* system_table,
                                       const TernaryOsBootContext* ctx) {
  char status_report[768];
  unsigned long cursor = 0;
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  status_report[0] = 0;
  append_cstr(status_report, sizeof(status_report), &cursor, "AXION_STARTUP_STATUS\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "os_name=Axion\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "platform_id=");
  append_cstr(status_report, sizeof(status_report), &cursor, ctx->platform_id);
  append_cstr(status_report, sizeof(status_report), &cursor, "\nphase=5\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "shell_mode=typed-builtins\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "storage_binding=virtualbox-ahci\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "display_binding=virtualbox-vmsvga\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "network_binding=virtualbox-e1000\n");
  append_cstr(status_report, sizeof(status_report), &cursor, "memory_map_len=");
  append_u64_dec(status_report, sizeof(status_report), &cursor, ctx->memory_map_len);
  append_cstr(status_report, sizeof(status_report), &cursor, "\n");

  status = write_root_file(root, kStartupStatusPath, status_report, ascii_length(status_report));
  root->Close(root);
  return status;
}

static EFI_STATUS write_startup_shell(EFI_HANDLE image_handle,
                                      EFI_SYSTEM_TABLE* system_table) {
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  status = write_root_file(root,
                           kStartupShellPath,
                           kGeneratedStartupShell,
                           ascii_length(kGeneratedStartupShell));
  root->Close(root);
  return status;
}

static EFI_STATUS write_startup_session(EFI_HANDLE image_handle,
                                        EFI_SYSTEM_TABLE* system_table) {
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  status = write_root_file(root,
                           kStartupSessionPath,
                           kGeneratedStartupSession,
                           ascii_length(kGeneratedStartupSession));
  root->Close(root);
  return status;
}

static EFI_STATUS write_startup_history(EFI_HANDLE image_handle,
                                        EFI_SYSTEM_TABLE* system_table) {
  EFI_FILE_PROTOCOL* root = 0;
  EFI_STATUS status = open_root_volume(image_handle, system_table, &root);
  if (status != EFI_SUCCESS) {
    return status;
  }

  status = write_root_file(root,
                           kStartupHistoryPath,
                           kGeneratedStartupHistory,
                           ascii_length(kGeneratedStartupHistory));
  root->Close(root);
  return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  TernaryOsBootContext ctx;
  ctx.memory_map = kArmv8VirtualBoxMemoryMap;
  ctx.memory_map_len = sizeof(kArmv8VirtualBoxMemoryMap) / sizeof(kArmv8VirtualBoxMemoryMap[0]);
  ctx.kernel_load_address = 0x0000000048000000ULL;
  ctx.stack_top = 0x0000000047FFF000ULL;
  ctx.ethics_boot_required = true;
  ctx.platform_id = kPlatformId;

  emit_boot_banner(system_table);

  const EFI_STATUS marker_status = write_execution_marker(image_handle, system_table);
  if (marker_status != EFI_SUCCESS) {
    return marker_status;
  }

  const int hal_result = ternaryos_hal_main_c(&ctx);
  const EFI_STATUS startup_status = write_startup_status(image_handle, system_table, &ctx);
  if (startup_status != EFI_SUCCESS) {
    return startup_status;
  }
  const EFI_STATUS startup_shell = write_startup_shell(image_handle, system_table);
  if (startup_shell != EFI_SUCCESS) {
    return startup_shell;
  }
  const EFI_STATUS startup_session = write_startup_session(image_handle, system_table);
  if (startup_session != EFI_SUCCESS) {
    return startup_session;
  }
  const EFI_STATUS startup_history = write_startup_history(image_handle, system_table);
  if (startup_history != EFI_SUCCESS) {
    return startup_history;
  }
  const EFI_STATUS report_status = write_boot_report(image_handle, system_table, &ctx, hal_result);
  if (report_status != EFI_SUCCESS) {
    return report_status;
  }

  return hal_result == 0 ? EFI_SUCCESS : EFI_LOAD_ERROR;
}
