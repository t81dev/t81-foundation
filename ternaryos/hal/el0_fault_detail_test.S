// ternaryos/hal/el0_fault_detail_test.S
//
// Phase 21 (RFC-00CB) — Process I (tid=10, EL0 ReadFaultInbox tester).
//
// Loaded from CanonFS LBA 14 by canon_fault_detail_query_load_and_run().
//
// Sequence:
//   1. Allocate 512 bytes on the EL0 stack.
//   2. Build ReadFaultInbox request at sp+0 (16 bytes, kind=15, target_tid=8).
//   3. Zero a 496-byte response buffer at sp+16.
//   4. SVC #1 — KernelCall(ReadFaultInbox).
//   5. SVC #2 — ExitThread(0).
//
// EL1 validates the response block after the thread exits.

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global fault_detail_i_entry

fault_detail_i_entry:
    // 16-byte request + 496-byte response = 512-byte frame.
    sub     sp, sp, #512

    // Zero response buffer at sp+16..sp+511 (496 bytes).
    mov     x9, #31
    add     x10, sp, #16
.Lzero_i:
    stp     xzr, xzr, [x10], #16
    subs    x9, x9, #1
    b.ne    .Lzero_i

    // Build ReadFaultInbox request for tid=8.
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // version=1, bytes=16
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    mov     w8, #KCALL_ReadFaultInbox
    str     w8, [sp, #8]

    mov     w8, #8
    str     w8, [sp, #12]

    // SVC #1: KernelCall(ReadFaultInbox), rsp_size=496.
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #496
    svc     #1

    mov     x0, #0
    svc     #2

.Lspin_i:
    wfe
    b       .Lspin_i
