// userland/experimental/hal/qemu_slice6_freestanding_crt.c
//
// Minimal freestanding CRT stubs for the QEMU slice6 EFI binary.
//
// The MSVC ABI (aarch64-pc-windows-msvc) does not inline __builtin_memset /
// __builtin_memcpy when the size is not a compile-time constant; it emits a
// call to the C-library function instead.  Since the EFI binary is linked
// with /nodefaultlib we must supply our own implementations.
//
// These are byte-loop fallbacks — not performance-critical; the data sizes
// involved (512-byte sectors, 4 KB pages) are small enough that loop overhead
// is negligible in a boot context.
//
// Compilation constraints: -ffreestanding -nostdlib (C, not C++).

#include <stddef.h>

void* memset(void* dest, int c, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    while (n--) *d++ = (unsigned char)c;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char*       d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char*       d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else if (d > s) {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}
