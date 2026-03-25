// ternaryos/hal/el0_supervisor_ack_test.S
//
// Phase 24 (RFC-00CE) — Process L (tid=13, EL0 supervisor fault-group ack tester).
//
// Loaded from CanonFS LBA 17 by canon_supervisor_ack_load_and_run().
//
// Sequence:
//   1. Allocate 672 bytes on the EL0 stack.
//   2. Issue AcknowledgeThreadFault(target_tid=8) into rsp1 at sp+16.
//   3. Issue QuerySupervisorRecoveryStatus into rsp2 at sp+80.
//   4. Issue AcknowledgeSupervisorFaultGroup(group_id=1) into rsp3 at sp+352.
//   5. Issue QuerySupervisorRecoveryStatus into rsp4 at sp+416.
//   6. SVC #2 — ExitThread(0).
//
// EL1 validates rsp1..rsp4 after the thread exits.

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global supervisor_ack_l_entry

supervisor_ack_l_entry:
    sub     sp, sp, #672

    mov     x9, #42
    mov     x10, sp
.Lzero_l:
    stp     xzr, xzr, [x10], #16
    subs    x9, x9, #1
    b.ne    .Lzero_l

    // Request 1: AcknowledgeThreadFault(target_tid=8), rsp1 at sp+16.
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

    // Request 2: QuerySupervisorRecoveryStatus, rsp2 at sp+80.
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

    // Request 3: AcknowledgeSupervisorFaultGroup(group_id=1), rsp3 at sp+352.
    add     x12, sp, #336
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [x12]
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [x12, #4]
    mov     w8, #KCALL_AcknowledgeSupervisorFaultGroup
    str     w8, [x12, #8]
    mov     w8, #1
    str     w8, [x12, #12]
    mov     x0, x12
    mov     x1, #16
    add     x2, x12, #16
    mov     x3, #48
    svc     #1

    // Request 4: QuerySupervisorRecoveryStatus, rsp4 at sp+416.
    add     x13, sp, #400
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [x13]
    mov     w8, #0x0001
    movk    w8, #0x000C, lsl #16
    str     w8, [x13, #4]
    mov     w8, #KCALL_QuerySupervisorRecoveryStatus
    str     w8, [x13, #8]
    mov     x0, x13
    mov     x1, #12
    add     x2, x13, #16
    mov     x3, #256
    svc     #1

    mov     x0, #0
    svc     #2

.Lspin_l:
    wfe
    b       .Lspin_l
