// ternaryos/hal/el0_fault_ack_test.S
//
// Phase 22 (RFC-00CC) — Process J (tid=11, EL0 fault acknowledgement tester).
//
// Loaded from CanonFS LBA 15 by canon_fault_ack_load_and_run().
//
// Sequence:
//   1. Allocate 1088 bytes on the EL0 stack.
//   2. Issue ReadFaultInbox(target_tid=8) into rsp1 at sp+16.
//   3. Issue AcknowledgeThreadFault(target_tid=8) into rsp2 at sp+528.
//   4. Issue ReadFaultInbox(target_tid=8) again into rsp3 at sp+592.
//   5. SVC #2 — ExitThread(0).
//
// EL1 validates rsp1/rsp2/rsp3 after the thread exits.

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global fault_ack_j_entry

fault_ack_j_entry:
    // rsp1(496) + rsp2(48) + rsp3(496) with 16-byte requests before each.
    sub     sp, sp, #1088

    // Zero the full frame.
    mov     x9, #68
    mov     x10, sp
.Lzero_j:
    stp     xzr, xzr, [x10], #16
    subs    x9, x9, #1
    b.ne    .Lzero_j

    // Request 1: ReadFaultInbox(target_tid=8) at sp+0, rsp1 at sp+16.
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    mov     w8, #KCALL_ReadFaultInbox
    str     w8, [sp, #8]

    mov     w8, #8
    str     w8, [sp, #12]

    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #496
    svc     #1

    // Request 2: AcknowledgeThreadFault(target_tid=8) at sp+512, rsp2 at sp+528.
    add     x11, sp, #512
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [x11]

    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [x11, #4]

    mov     w8, #KCALL_AcknowledgeThreadFault
    str     w8, [x11, #8]

    mov     w8, #8
    str     w8, [x11, #12]

    mov     x0, x11
    mov     x1, #16
    add     x2, x11, #16
    mov     x3, #48
    svc     #1

    // Request 3: ReadFaultInbox(target_tid=8) at sp+576, rsp3 at sp+592.
    add     x12, sp, #576
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [x12]

    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [x12, #4]

    mov     w8, #KCALL_ReadFaultInbox
    str     w8, [x12, #8]

    mov     w8, #8
    str     w8, [x12, #12]

    mov     x0, x12
    mov     x1, #16
    add     x2, x12, #16
    mov     x3, #496
    svc     #1

    mov     x0, #0
    svc     #2

.Lspin_j:
    wfe
    b       .Lspin_j
