// ternaryos/hal/el0_ipc_test_a.S
//
// Phase 8 EL0 IPC test — Process A (tid=2).
//
// Loaded from CanonFS LBA 4 by canon_ipc_load_and_run().
// EReted from EL1 with:
//   ELR_EL1 = s_proc_code_page_pa  (PA of ipc_a_entry, identity-mapped)
//   SP_EL0  = el0_mmu_proc_stack_top()
//   SPSR_EL1 = 0x3C0  (EL0t, DAIF masked)
//
// Sequence:
//   1. sub sp, #64  — allocate 16B req + 48B rsp on proc stack.
//   2. Build SendMessage KernelCall request at sp+0 (16B):
//        magic=0x4B415152, version=1, bytes=16, kind=13(SendMessage)
//        ipc_dst=3 (target tid = Process B)
//   3. Zero response buffer at sp+16 (48B).
//   4. SVC #1 — KernelCall(SendMessage); response at sp+16.
//   5. SVC #2 — ExitThread(0) → ERET to EL1.
//
// Wire block layout (sp+0..sp+15):
//   [0:4]   magic   = 0x4B415152  (KAQR)
//   [4:6]   version = 1
//   [6:8]   bytes   = 16
//   [8:12]  kind    = 13  (SendMessage)
//   [12:16] ipc_dst = 3   (target tid = B)
//
// SVC #1 ABI: x0=req_tva, x1=req_size(16), x2=rsp_tva(sp+16), x3=rsp_size(48).
// Both TVAs lie within the proc stack page — el0_tva_valid passes.
//
// Position-independent code: no absolute addresses.
// All labels use PC-relative addressing (ADR generates ±1 MB range).
//
// Compilation constraints:
//   --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    // RFC-00BD: use named KernelCallKind constants; no literal ordinals.
    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global ipc_a_entry

ipc_a_entry:
    // ── Allocate 64-byte frame on EL0 process stack ───────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes = 6 × 8B stores) ────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build SendMessage KernelCallWireRequestBlock at sp+0..sp+15 ──────────
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

    // offset [8:12] kind = KCALL_SendMessage
    mov     w8, #KCALL_SendMessage
    str     w8, [sp, #8]

    // offset [12:16] ipc_dst = 3 (target tid = Process B)
    mov     w8, #3
    str     w8, [sp, #12]

    // ── SVC #1: KernelCall(SendMessage) ──────────────────────────────────────
    // x0 = req_tva (sp), x1 = req_size (16), x2 = rsp_tva (sp+16), x3 = 48
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #2: ExitThread(0) ─────────────────────────────────────────────────
    mov     x0, #0
    svc     #2

    // Should never reach here.
.Lspin_a:
    wfe
    b       .Lspin_a
