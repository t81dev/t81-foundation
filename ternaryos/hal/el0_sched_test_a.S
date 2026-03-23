// ternaryos/hal/el0_sched_test_a.S
//
// Phase 9 cooperative scheduler test — Process A (tid=2, sender).
//
// Loaded from CanonFS LBA 6 by canon_sched_load_and_run(); placed at
// code-page offset 256 so it coexists with Process B (at offset 0).
// EReted from EL1 by the RFC-00BE scheduler after Process B parks:
//   ELR_EL1 = proc_code_pa + 256   (this entry point)
//   SP_EL0  = el0_mmu_proc_stack_top() − 256
//   SPSR_EL1 = 0x3C0  (EL0t, DAIF masked)
//
// Sequence:
//   1. sub sp, #64  — allocate 16B req + 48B rsp on proc stack.
//   2. Build SendMessage KernelCall request at sp+0 (16B):
//        magic=KAQR, version=1, bytes=16, kind=13(SendMessage), ipc_dst=3
//   3. Zero response buffer at sp+16 (48B).
//   4. SVC #1 — KernelCall(SendMessage, ipc_dst=3):
//        EL1 finds Process B (tid=3) in BlockedIpcReceive state,
//        writes sender_tid=2 to B's response buffer, marks B Runnable.
//   5. SVC #2 — ExitThread(0):
//        RFC-00BE fs_sched_exit_thread switches to B (now Runnable) → ERET.
//
// Position-independent: PC-relative addressing only.
// Compilation: --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    // RFC-00BD: named KernelCallKind constants — no literal ordinals.
    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global sched_a_entry

sched_a_entry:
    // ── Allocate 64-byte frame on EL0 process stack ───────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes) ─────────────────────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build SendMessage wire request at sp+0..sp+15 ────────────────────────
    // [0:4]   magic   = KAQR
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // [4:6] version=1  [6:8] bytes=16  → packed: 0x00100001
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    // [8:12] kind = SendMessage
    mov     w8, #KCALL_SendMessage
    str     w8, [sp, #8]

    // [12:16] ipc_dst = 3 (Process B)
    mov     w8, #3
    str     w8, [sp, #12]

    // ── SVC #1: KernelCall(SendMessage, ipc_dst=3) ───────────────────────────
    // EL1 wakes Process B and marks it Runnable; A continues uninterrupted.
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #2: ExitThread(0) ─────────────────────────────────────────────────
    // RFC-00BE: fs_sched_exit_thread finds B Runnable → ERET to B.
    mov     x0, #0
    svc     #2

    // Should never reach here.
.Lspin_a:
    wfe
    b       .Lspin_a
