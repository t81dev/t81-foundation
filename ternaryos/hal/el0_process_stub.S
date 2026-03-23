// ternaryos/hal/el0_process_stub.S
//
// Phase 7 EL0 process entry stub — CanonFS-loaded process (tid=2).
//
// This stub is the "executable" read from the CanonFS virtio-blk image at
// LBA 3 and copied into s_el0_proc_code_page by canon_exec_load_and_run().
// The loader ERets here from EL1 with:
//   ELR_EL1 = s_proc_code_page_pa  (PA of proc_entry, identity-mapped)
//   SP_EL0  = el0_mmu_proc_stack_top()
//   SPSR_EL1 = 0x0 (EL0t, DAIF unmasked)
//
// The stub uses the same SVC ABI as axion_el0_init.S (Phase 6):
//
//   svc #0  GetThreadIdentity (direct, legacy)   →  x0 = tid
//   svc #1  KernelCall(x0,x1,x2,x3)             →  wire-format dispatch
//   svc #2  ExitThread(x0=exit_code)             →  ERET to EL1
//   svc #3  WriteSerial(x0=str_tva)             →  debug PL011 write
//
// Sequence (mirrors axion_el0_init.S for consistency):
//   1. sub sp, #64  — allocate 16B req + 48B rsp on the process stack.
//   2. Build GetThreadIdentity KernelCall request at sp+0 (16B).
//   3. Zero response buffer at sp+16 (48B).
//   4. SVC #1 — KernelCall(GetThreadIdentity); response at sp+16.
//   5. SVC #3 — WriteSerial(.Lmsg) → "[axion] el0: process loaded from CanonFS (tid=2)"
//   6. SVC #2 — ExitThread(0) → ERET to EL1.
//
// IMPORTANT — Position-independent code requirement:
//   This code is copied verbatim (as raw bytes from the T81X code section) into
//   s_el0_proc_code_page.  The EFI linker places it at some VA in the EFI image,
//   but at runtime it executes from proc_code_page_pa (a different PA/VA).
//   Therefore ALL addresses must be PC-relative:
//     - Stack: sub/add relative to SP (fine)
//     - .Lmsg: adr x0, .Lmsg  — PC-relative (fine, assembler generates ADR)
//   No absolute fixups, no GOT, no relocation.  The -mcmodel=tiny flag (or the
//   assembler's default for a bare .S file) keeps ADR within ±1 MB of PC, which
//   is guaranteed since .Lmsg is in the same 512-byte sector.
//
// Compilation constraints:
//   --target=aarch64-unknown-elf  -ffreestanding  -nostdlib
//   (compiled separately by qemu-boot.yml; llvm-objcopy -O binary extracts the
//   raw .text section for embedding in the T81X image at LBA 3)

    // RFC-00BD: use named KernelCallKind constants; no literal ordinals.
    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global proc_entry

proc_entry:
    // ── Allocate 64-byte frame on EL0 process stack ──────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes = 6 × 8B stores) ────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build compact KernelCallWireRequestBlock at sp+0..sp+15 ──────────────
    // offset [0:4]  magic = KCALL_REQ_MAGIC (kKernelAbiWireRequestMagic)
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // offset [4:6]  version = KCALL_WIRE_VERSION
    // offset [6:8]  bytes   = 16
    // packed 32-bit store: 0x00100001
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    // offset [8:12] kind = KCALL_GetThreadIdentity
    mov     w8, #KCALL_GetThreadIdentity
    str     w8, [sp, #8]

    // offset [12:16] pad = 0
    str     wzr, [sp, #12]

    // ── SVC #1: KernelCall(GetThreadIdentity) ────────────────────────────────
    // x0 = req_tva (sp), x1 = req_size (16), x2 = rsp_tva (sp+16), x3 = 48
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #3: WriteSerial — emit Phase 7 process banner ────────────────────
    adr     x0, .Lmsg
    svc     #3

    // ── SVC #2: ExitThread(0) ────────────────────────────────────────────────
    mov     x0, #0
    svc     #2

    // Should never reach here.
.Lspin:
    wfe
    b       .Lspin

    .balign 4
.Lmsg:
    .asciz  "[axion] el0: process loaded from CanonFS (tid=2)\r\n"

    // .size omitted — ELF-only directive; not valid for COFF/PE targets.
