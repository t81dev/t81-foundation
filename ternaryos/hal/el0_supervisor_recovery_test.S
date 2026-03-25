// ternaryos/hal/el0_supervisor_recovery_test.S
//
// Phase 23 (RFC-00CD) — Process K (tid=12, EL0 supervisor recovery tester).
//
// Loaded from CanonFS LBA 16 by canon_supervisor_recovery_query_load_and_run().
//
// Sequence:
//   1. Allocate 336 bytes on the EL0 stack.
//   2. Issue AcknowledgeThreadFault(target_tid=8) into rsp1 at sp+16.
//   3. Issue QuerySupervisorRecoveryStatus into rsp2 at sp+80.
//   4. SVC #2 — ExitThread(0).
//
// EL1 validates rsp1/rsp2 after the thread exits.

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global supervisor_recovery_k_entry

supervisor_recovery_k_entry:
    // rsp1(48) + rsp2(256), each preceded by a 16-byte request.
    sub     sp, sp, #336

    // Zero the full frame.
    mov     x9, #21
    mov     x10, sp
.Lzero_k:
    stp     xzr, xzr, [x10], #16
    subs    x9, x9, #1
    b.ne    .Lzero_k

    // Request 1: AcknowledgeThreadFault(target_tid=8) at sp+0, rsp1 at sp+16.
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    mov     w8, #KCALL_AcknowledgeThreadFault
    str     w8, [sp, #8]

    mov     w8, #8
    str     w8, [sp, #12]

    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // Request 2: QuerySupervisorRecoveryStatus at sp+64, rsp2 at sp+80.
    add     x11, sp, #64
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [x11]

    mov     w8, #0x0001
    movk    w8, #0x000C, lsl #16
    str     w8, [x11, #4]

    mov     w8, #KCALL_QuerySupervisorRecoveryStatus
    str     w8, [x11, #8]

    mov     x0, x11
    mov     x1, #12
    add     x2, x11, #16
    mov     x3, #256
    svc     #1

    mov     x0, #0
    svc     #2

.Lspin_k:
    wfe
    b       .Lspin_k
