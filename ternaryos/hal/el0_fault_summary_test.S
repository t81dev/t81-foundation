// ternaryos/hal/el0_fault_summary_test.S
//
// Phase 20 (RFC-00CA) — Process H (tid=9, EL0 QueryFaultSummary tester).
//
// Loaded from CanonFS LBA 13 by canon_fault_summary_query_load_and_run().
//
// Sequence:
//   1. Allocate 256 bytes on the EL0 stack.
//   2. Build QueryFaultSummary request at sp+0 (12 bytes, kind=21).
//   3. Zero a 240-byte response buffer at sp+16.
//   4. SVC #1 — KernelCall(QueryFaultSummary).
//   5. SVC #2 — ExitThread(0).
//
// EL1 validates the response block after the thread exits.

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global fault_summary_h_entry

fault_summary_h_entry:
    // 16-byte request + 240-byte response = 256-byte frame.
    sub     sp, sp, #256

    // Zero response buffer at sp+16..sp+255 (240 bytes).
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]
    stp     xzr, xzr, [sp, #64]
    stp     xzr, xzr, [sp, #80]
    stp     xzr, xzr, [sp, #96]
    stp     xzr, xzr, [sp, #112]
    stp     xzr, xzr, [sp, #128]
    stp     xzr, xzr, [sp, #144]
    stp     xzr, xzr, [sp, #160]
    stp     xzr, xzr, [sp, #176]
    stp     xzr, xzr, [sp, #192]
    stp     xzr, xzr, [sp, #208]
    stp     xzr, xzr, [sp, #224]
    stp     xzr, xzr, [sp, #240]

    // Build QueryFaultSummary request.
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // version=1, bytes=12
    mov     w8, #0x0001
    movk    w8, #0x000C, lsl #16
    str     w8, [sp, #4]

    mov     w8, #KCALL_QueryFaultSummary
    str     w8, [sp, #8]

    // SVC #1: KernelCall(QueryFaultSummary), rsp_size=240.
    mov     x0, sp
    mov     x1, #12
    add     x2, sp, #16
    mov     x3, #240
    svc     #1

    mov     x0, #0
    svc     #2

.Lspin_h:
    wfe
    b       .Lspin_h
