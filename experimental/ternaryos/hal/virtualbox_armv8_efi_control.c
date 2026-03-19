// experimental/ternaryos/hal/virtualbox_armv8_efi_control.c
//
// Ultra-minimal ARMv8 EFI control application for the Apple Silicon developer
// lane. This does not touch the HAL bridge. It only attempts to leave a disk
// marker so we can tell whether VirtualBox ARM executed any EFI application at
// all.

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
  uint64_t _pad[24];
  EFI_STATUS(EFIAPI* HandleProtocol)(EFI_HANDLE handle, EFI_GUID* protocol, void** interface);
};

struct EFI_SYSTEM_TABLE {
  uint64_t _pad0[11];
  void* RuntimeServices;
  EFI_BOOT_SERVICES* BootServices;
};

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 1
#define EFI_WRITE_PROTECTED 8
#define EFI_OPEN_MODE_READ 0x0000000000000001ULL
#define EFI_OPEN_MODE_WRITE 0x0000000000000002ULL
#define EFI_OPEN_MODE_CREATE 0x8000000000000000ULL

static const EFI_GUID kEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const EFI_GUID kEfiSimpleFileSystemProtocolGuid = {
    0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static const CHAR16 kMarkerPath[] = L"\\TERNOS\\efi-ctrl-ran.txt";
static const char kMarkerText[] = "TERNOS_ARMV8_EFI_CONTROL_EXECUTED\n";

static EFI_STATUS write_execution_marker(EFI_HANDLE image_handle,
                                         EFI_SYSTEM_TABLE* system_table) {
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

  EFI_FILE_PROTOCOL* file = 0;
  status = root->Open(root,
                      &file,
                      (CHAR16*)kMarkerPath,
                      EFI_OPEN_MODE_READ | EFI_OPEN_MODE_WRITE | EFI_OPEN_MODE_CREATE,
                      0);
  if (status != EFI_SUCCESS || file == 0 || file->Write == 0 || file->Close == 0) {
    root->Close(root);
    return EFI_WRITE_PROTECTED;
  }

  UINTN bytes = sizeof(kMarkerText) - 1;
  status = file->Write(file, &bytes, (void*)kMarkerText);
  file->Close(file);
  root->Close(root);
  return status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  return write_execution_marker(image_handle, system_table);
}
