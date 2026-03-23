// ternaryos/hal/el0_sched_test_b.S
//
// Phase 9 cooperative scheduler test — Process B (tid=3, blocker).
//
// Loaded from CanonFS LBA 7 by canon_sched_load_and_run(); placed at
// code-page offset 0 and runs first.
// Initial ERET context from EL1:
//   ELR_EL1 = proc_code_pa + 0   (this entry point)
//   SP_EL0  = el0_mmu_proc_stack_top()
//   SPSR_EL1 = 0x3C0  (EL0t, DAIF masked)
//
// Sequence:
//   1. sub sp, #64  — allocate 16B req + 48B rsp on proc stack.
//   2. Build BlockOnIpcReceive request at sp+0 (12B minimum):
//        magic=KAQR, version=1, bytes=12, kind=42(BlockOnIpcReceive)
//   3. Zero response buffer at sp+16 (48B).
//   4. SVC #1 — KernelCall(BlockOnIpcReceive):
//        EL1 saves B context (ELR, SP_EL0, SPSR), marks B BlockedIpcReceive,
//        finds A Runnable → redirects ERET to A.  B is suspended here.
//        When A later calls SendMessage(ipc_dst=3):
//          EL1 writes sender_tid=2 to this response buffer at [36:40],
//          marks B Runnable.
//        When A calls ExitThread:
//          EL1 context-switches back to B (resumes at instruction after svc #1).
//   5. (B resumes here — response[36:40] = sender_tid = 2)
//   6. SVC #2 — ExitThread(0):
//        RFC-00BE: no Runnable threads remain → ERET to EL1.
//        EL1 emits "[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)".
//
// Position-independent: PC-relative addressing only.
// Compilation: --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    // RFC-00BD: named KernelCallKind constants — no literal ordinals.
    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global sched_b_entry

sched_b_entry:
    // ── Allocate 64-byte frame on EL0 process stack ───────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes) ─────────────────────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build BlockOnIpcReceive wire request at sp+0..sp+11 ──────────────────
    // [0:4]   magic   = KAQR
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // [4:6] version=1  [6:8] bytes=12  → packed: 0x000C0001
    mov     w8, #0x0001
    movk    w8, #0x000C, lsl #16
    str     w8, [sp, #4]

    // [8:12] kind = BlockOnIpcReceive
    mov     w8, #KCALL_BlockOnIpcReceive
    str     w8, [sp, #8]

    // ── SVC #1: KernelCall(BlockOnIpcReceive) ────────────────────────────────
    // x0=req_tva(sp), x1=req_size(12), x2=rsp_tva(sp+16), x3=rsp_size(48).
    // EL1 parks B and ERets to Process A.  B resumes here after A's SendMessage
    // + ExitThread; response[36:40] = sender_tid = 2 on return.
    mov     x0, sp
    mov     x1, #12
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #2: ExitThread(0) ─────────────────────────────────────────────────
    // RFC-00BE: no Runnable threads remain → fs_sched_exit_thread returns to EL1.
    mov     x0, #0
    svc     #2

    // Should never reach here.
.Lspin_b:
    wfe
    b       .Lspin_b
