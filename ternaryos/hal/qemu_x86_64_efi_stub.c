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

typedef struct {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void*      Reset;
  EFI_STATUS (*OutputString)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
  uint8_t    _pad[128];
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER  Hdr;
  uint8_t           _pad0[24];
  void*             RaiseTPL;
  void*             RestoreTPL;
  uint8_t           _pad1[64];
  void*             AllocatePages;
  void*             FreePages;
  EFI_STATUS      (*GetMemoryMap)(UINTN*, void*, UINTN*, UINTN*, uint32_t*);
  void*             AllocatePool;
  void*             FreePool;
  uint8_t           _pad2[64];
  void*             CreateEvent;
  void*             SetTimer;
  void*             WaitForEvent;
  uint8_t           _pad3[24];
  void*             SignalEvent;
  void*             CloseEvent;
  void*             CheckEvent;
  uint8_t           _pad4[96];
  EFI_STATUS      (*ExitBootServices)(EFI_HANDLE, UINTN);
  uint8_t           _pad5[8];
  EFI_STATUS      (*Stall)(UINTN);  // microseconds
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

// ── Forward declaration (qemu_x86_64_bare_kernel.c) ──────────────────────────

void qemu_x86_64_bare_kernel_entry(uint64_t tsc_freq_hz);

// ── EFI entry point ───────────────────────────────────────────────────────────

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* ST) {
  EFI_BOOT_SERVICES*            BS  = ST->BootServices;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* co = ST->ConOut;

  // Phase 1 banner (EFI ConOut — appears before ExitBootServices).
  con_puts(co, "\r\nAxion QEMU x86_64 OVMF slice6\r\n\r\n");

  // ── Measure TSC frequency via EFI Stall ────────────────────────────────────
  // Stall sleeps for the requested number of microseconds.  Measuring the
  // TSC delta over 200 ms gives ~0.5% accuracy — sufficient for a wall-clock
  // uptime display.
  const UINTN kStallUs = 200000;  // 200 ms
  const uint64_t tsc0 = rdtsc();
  BS->Stall(kStallUs);
  const uint64_t tsc1 = rdtsc();
  // Extrapolate: delta / 0.2 s = cycles per second.
  const uint64_t tsc_freq_hz = (tsc1 - tsc0) * (1000000ULL / kStallUs);

  // ── ExitBootServices ───────────────────────────────────────────────────────
  // EFI requires passing the current memory-map key to ExitBootServices.
  // We call GetMemoryMap once to obtain the key, then immediately exit.
  uint8_t  mmap_buf[4096];
  UINTN    mmap_size = sizeof(mmap_buf);
  UINTN    mmap_key  = 0;
  UINTN    desc_size = 0;
  uint32_t desc_ver  = 0;

  BS->GetMemoryMap(&mmap_size, mmap_buf, &mmap_key, &desc_size, &desc_ver);
  BS->ExitBootServices(ImageHandle, mmap_key);

  // ── Hand off to bare-metal kernel ─────────────────────────────────────────
  // From this point EFI services are unavailable.  All output goes through
  // the COM1 UART initialised in qemu_x86_64_bare_kernel_entry().
  qemu_x86_64_bare_kernel_entry(tsc_freq_hz);

  // Should not reach here on bare-metal; loop defensively.
  for (;;) { __asm__ volatile("hlt"); }
  return EFI_SUCCESS;
}
