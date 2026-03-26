// ternaryos/hal/qemu_x86_64_efi_stub.c
//
// UEFI entry point for QEMU x86_64 + OVMF (T81 slice6 developer lane).
//
// Three-phase boot path:
//   Phase 1 (this file)  — EFI environment: banner via ConOut, TSC frequency
//                          measurement via BS->Stall, ExitBootServices.
//   Phase 2              — qemu_x86_64_bare_kernel.c: COM1 UART init, probe.
//   Phase 3              — qemu_x86_64_cpp_bridge.cpp: T81 banner + t81> shell.
//
// Freestanding C only.  No libc, no libstdc++.
// Compiled: clang --target=x86_64-pc-windows-msvc -ffreestanding -nostdlib
// Linked:   lld-link /subsystem:efi_application /machine:x64 /entry:efi_main

#include <stdint.h>

// ── Minimal EFI type definitions ──────────────────────────────────────────────

typedef uint64_t  EFI_STATUS;
typedef void*     EFI_HANDLE;
typedef uint16_t  CHAR16;
typedef uint64_t  UINTN;

#define EFI_SUCCESS  0ULL
#define EFI_ERROR_BIT UINT64_C(0x8000000000000000)
#define EFI_INVALID_PARAMETER (EFI_ERROR_BIT | 2ULL)
#define EFI_BUFFER_TOO_SMALL (EFI_ERROR_BIT | 5ULL)

#if defined(__x86_64__) && defined(_WIN32)
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

typedef struct {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void*      Reset;
  EFI_STATUS (EFIAPI *OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*,
                                    CHAR16*);
  uint8_t    _pad[128];
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER  Hdr;
  void*             RaiseTPL;
  void*             RestoreTPL;
  void*             AllocatePages;
  void*             FreePages;
  EFI_STATUS      (EFIAPI *GetMemoryMap)(UINTN*, void*, UINTN*, UINTN*,
                                         uint32_t*);
  void*             AllocatePool;
  void*             FreePool;
  void*             CreateEvent;
  void*             SetTimer;
  void*             WaitForEvent;
  void*             SignalEvent;
  void*             CloseEvent;
  void*             CheckEvent;
  void*             InstallProtocolInterface;
  void*             ReinstallProtocolInterface;
  void*             UninstallProtocolInterface;
  void*             HandleProtocol;
  void*             Reserved;
  void*             RegisterProtocolNotify;
  void*             LocateHandle;
  void*             LocateDevicePath;
  void*             InstallConfigurationTable;
  void*             LoadImage;
  void*             StartImage;
  void*             Exit;
  void*             UnloadImage;
  EFI_STATUS      (EFIAPI *ExitBootServices)(EFI_HANDLE, UINTN);
  void*             GetNextMonotonicCount;
  EFI_STATUS      (EFIAPI *Stall)(UINTN);  // microseconds
  void*             SetWatchdogTimer;
  void*             ConnectController;
  void*             DisconnectController;
  void*             OpenProtocol;
  void*             CloseProtocol;
  void*             OpenProtocolInformation;
  void*             ProtocolsPerHandle;
  void*             LocateHandleBuffer;
  void*             LocateProtocol;
  void*             InstallMultipleProtocolInterfaces;
  void*             UninstallMultipleProtocolInterfaces;
  void*             CalculateCrc32;
  void*             CopyMem;
  void*             SetMem;
  void*             CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
  EFI_TABLE_HEADER              Hdr;
  CHAR16*                       FirmwareVendor;
  uint32_t                      FirmwareRevision;
  uint8_t                       _pad0[4];
  EFI_HANDLE                    ConsoleInHandle;
  void*                         ConIn;
  EFI_HANDLE                    ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
  EFI_HANDLE                    StandardErrorHandle;
  void*                         StdErr;
  void*                         RuntimeServices;
  EFI_BOOT_SERVICES*            BootServices;
} EFI_SYSTEM_TABLE;

// ── TSC helper (available before ExitBootServices) ────────────────────────────

static inline uint64_t rdtsc(void) {
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

// ── ConOut helper ─────────────────────────────────────────────────────────────

static void con_puts(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con, const char* s) {
  CHAR16 buf[2];
  buf[1] = 0;  // NUL terminator — never changes
  while (*s) {
    buf[0] = (CHAR16)(unsigned char)*s++;
    con->OutputString(con, buf);
  }
}

static void con_puthex(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con, uint64_t value) {
  static const char kHex[] = "0123456789ABCDEF";
  CHAR16 buf[19];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 16; ++i) {
    const uint32_t shift = (uint32_t)(15 - i) * 4u;
    buf[2 + i] = (CHAR16)kHex[(value >> shift) & 0xFu];
  }
  buf[18] = 0;
  con->OutputString(con, buf);
}

// ── Forward declaration (qemu_x86_64_bare_kernel.c) ──────────────────────────

void qemu_x86_64_bare_kernel_entry(uint64_t tsc_freq_hz);

// ── EFI entry point ───────────────────────────────────────────────────────────

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* ST) {
  EFI_BOOT_SERVICES*            BS  = ST->BootServices;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* co = ST->ConOut;

  // Phase 1 banner (EFI ConOut — appears before ExitBootServices).
  con_puts(co, "\r\nAxion QEMU x86_64 OVMF slice6\r\n\r\n");
  con_puts(co, "[axion] EFI phase: measure TSC\r\n");

  // ── Measure TSC frequency via EFI Stall ────────────────────────────────────
  // Stall sleeps for the requested number of microseconds.  Measuring the
  // TSC delta over 200 ms gives ~0.5% accuracy — sufficient for a wall-clock
  // uptime display.
  const UINTN kStallUs = 200000;  // 200 ms
  uint64_t tsc_freq_hz = UINT64_C(1000000000);  // conservative fallback
  if (BS->Stall != (void*)0) {
    const uint64_t tsc0 = rdtsc();
    const EFI_STATUS stall_status = BS->Stall(kStallUs);
    const uint64_t tsc1 = rdtsc();
    if (stall_status == EFI_SUCCESS) {
      // Extrapolate: delta / 0.2 s = cycles per second.
      tsc_freq_hz = (tsc1 - tsc0) * (1000000ULL / kStallUs);
    } else {
      con_puts(co, "[axion] EFI Stall failed: ");
      con_puthex(co, stall_status);
      con_puts(co, "\r\n");
    }
  }

  // ── ExitBootServices ───────────────────────────────────────────────────────
  // EFI requires passing the current memory-map key to ExitBootServices.
  // We call GetMemoryMap once to obtain the key, then immediately exit.
  static uint8_t mmap_buf[16384];
  UINTN    mmap_size = sizeof(mmap_buf);
  UINTN    mmap_key  = 0;
  UINTN    desc_size = 0;
  uint32_t desc_ver  = 0;

  con_puts(co, "[axion] EFI phase: ExitBootServices\r\n");
  EFI_STATUS exit_status = EFI_INVALID_PARAMETER;
  for (unsigned attempt = 0; attempt < 4; ++attempt) {
    mmap_size = sizeof(mmap_buf);
    const EFI_STATUS mmap_status = BS->GetMemoryMap(
        &mmap_size, mmap_buf, &mmap_key, &desc_size, &desc_ver);
    if (mmap_status != EFI_SUCCESS) {
      con_puts(co, "[axion] GetMemoryMap failed: ");
      con_puthex(co, mmap_status);
      if (mmap_status == EFI_BUFFER_TOO_SMALL) {
        con_puts(co, " (buffer too small)");
      }
      con_puts(co, "\r\n");
      return mmap_status;
    }

    exit_status = BS->ExitBootServices(ImageHandle, mmap_key);
    if (exit_status == EFI_SUCCESS) break;
    if (exit_status != EFI_INVALID_PARAMETER) break;
  }

  if (exit_status != EFI_SUCCESS) {
    con_puts(co, "[axion] ExitBootServices failed: ");
    con_puthex(co, exit_status);
    con_puts(co, "\r\n");
    return exit_status;
  }

  // ── Hand off to bare-metal kernel ─────────────────────────────────────────
  // From this point EFI services are unavailable.  All output goes through
  // the COM1 UART initialised in qemu_x86_64_bare_kernel_entry().
  qemu_x86_64_bare_kernel_entry(tsc_freq_hz);

  // Should not reach here on bare-metal; loop defensively.
  for (;;) { __asm__ volatile("hlt"); }
  return EFI_SUCCESS;
}
