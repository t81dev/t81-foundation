// experimental/ternaryos/hal/aarch64_exception_vectors.S
//
// AArch64 VBAR_EL1 exception vector table for the Axion kernel.
//
// Per ARM Architecture Reference Manual (ARM DDI 0487):
//   VBAR_EL1 must be 2 KiB aligned (bits [10:0] = 0).
//   The vector table has 16 slots of 128 bytes (32 instructions) each:
//
//   Offset 0x000  Current EL, SP0,    Synchronous
//   Offset 0x080  Current EL, SP0,    IRQ/vIRQ
//   Offset 0x100  Current EL, SP0,    FIQ/vFIQ
//   Offset 0x180  Current EL, SP0,    SError/vSError
//   Offset 0x200  Current EL, SPx,    Synchronous
//   Offset 0x280  Current EL, SPx,    IRQ/vIRQ
//   Offset 0x300  Current EL, SPx,    FIQ/vFIQ
//   Offset 0x380  Current EL, SPx,    SError/vSError
//   Offset 0x400  Lower EL (AArch64), Synchronous  ← SVC dispatch
//   Offset 0x480  Lower EL (AArch64), IRQ/vIRQ
//   Offset 0x500  Lower EL (AArch64), FIQ/vFIQ
//   Offset 0x580  Lower EL (AArch64), SError/vSError
//   Offset 0x600  Lower EL (AArch32), Synchronous
//   Offset 0x680  Lower EL (AArch32), IRQ/vIRQ
//   Offset 0x700  Lower EL (AArch32), FIQ/vFIQ
//   Offset 0x780  Lower EL (AArch32), SError/vSError
//
// Only the Lower EL (AArch64) Synchronous slot at 0x400 is functional; it
// branches to axion_svc_entry which saves AArch64TrapFrame and calls
// axion_kernel_handle_svc_trap_aarch64().  All other slots branch to the
// axion_vec_unhandled infinite-loop stub.
//
// The save frame layout in axion_svc_entry must match AArch64TrapFrame in
// aarch64_trap_entry.hpp (verified by static_assert and [AC-22n]).

    .section .axion_vectors, "ax"
    .balign 2048
    .global axion_exception_vector_base
axion_exception_vector_base:

// ── Current EL, SP0 ──────────────────────────────────────────────────────────

    .balign 128
axion_vec_cur_sp0_sync:
    b   axion_vec_unhandled

    .balign 128
axion_vec_cur_sp0_irq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_cur_sp0_fiq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_cur_sp0_serror:
    b   axion_vec_unhandled

// ── Current EL, SPx ──────────────────────────────────────────────────────────

    .balign 128
axion_vec_cur_spx_sync:
    b   axion_vec_unhandled

    .balign 128
axion_vec_cur_spx_irq:
    b   axion_irq_entry         // GIC IRQ while kernel runs at EL1/SPx

    .balign 128
axion_vec_cur_spx_fiq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_cur_spx_serror:
    b   axion_vec_unhandled

// ── Lower EL, AArch64 ────────────────────────────────────────────────────────
// Offset 0x400: SVC from AArch64 user space — the Axion kernel-call entry.

    .balign 128
axion_vec_lower_aarch64_sync:
    b   axion_svc_entry         // real handler lives below the vector table

    .balign 128
axion_vec_lower_aarch64_irq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_lower_aarch64_fiq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_lower_aarch64_serror:
    b   axion_vec_unhandled

// ── Lower EL, AArch32 ────────────────────────────────────────────────────────

    .balign 128
axion_vec_lower_aarch32_sync:
    b   axion_vec_unhandled

    .balign 128
axion_vec_lower_aarch32_irq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_lower_aarch32_fiq:
    b   axion_vec_unhandled

    .balign 128
axion_vec_lower_aarch32_serror:
    b   axion_vec_unhandled

// ── Unhandled exception: deliberate infinite loop ────────────────────────────
// A real production kernel would log and halt; the loop here lets a debugger
// catch unexpected exception vectors without silently corrupting state.

    .balign 4
    .global axion_vec_unhandled
axion_vec_unhandled:
    b   axion_vec_unhandled

// ── SVC entry: save AArch64TrapFrame, dispatch, restore, ERET ────────────────
//
// This routine is branched to from axion_vec_lower_aarch64_sync (offset 0x400)
// and lies outside the 2 KiB vector table so it is not constrained to 128 bytes.
//
// Stack frame written here (280 bytes = 35 × 8) must match AArch64TrapFrame
// in aarch64_trap_entry.hpp:
//
//   [sp +   0]  x0           [sp +   8]  x1
//   [sp +  16]  x2           [sp +  24]  x3
//   [sp +  32]  x4           [sp +  40]  x5
//   [sp +  48]  x6           [sp +  56]  x7
//   [sp +  64]  x8           [sp +  72]  x9
//   [sp +  80]  x10          [sp +  88]  x11
//   [sp +  96]  x12          [sp + 104]  x13
//   [sp + 112]  x14          [sp + 120]  x15
//   [sp + 128]  x16          [sp + 136]  x17
//   [sp + 144]  x18          [sp + 152]  x19
//   [sp + 160]  x20          [sp + 168]  x21
//   [sp + 176]  x22          [sp + 184]  x23
//   [sp + 192]  x24          [sp + 200]  x25
//   [sp + 208]  x26          [sp + 216]  x27
//   [sp + 224]  x28          [sp + 232]  x29
//   [sp + 240]  x30  (lr)
//   [sp + 248]  sp_el0
//   [sp + 256]  elr_el1
//   [sp + 264]  spsr_el1
//   [sp + 272]  esr_el1

    .balign 16
    .global axion_svc_entry
axion_svc_entry:
    // Allocate frame.
    sub  sp, sp, #280

    // Save general-purpose registers x0..x30.
    stp  x0,  x1,  [sp,   #0]
    stp  x2,  x3,  [sp,  #16]
    stp  x4,  x5,  [sp,  #32]
    stp  x6,  x7,  [sp,  #48]
    stp  x8,  x9,  [sp,  #64]
    stp  x10, x11, [sp,  #80]
    stp  x12, x13, [sp,  #96]
    stp  x14, x15, [sp, #112]
    stp  x16, x17, [sp, #128]
    stp  x18, x19, [sp, #144]
    stp  x20, x21, [sp, #160]
    stp  x22, x23, [sp, #176]
    stp  x24, x25, [sp, #192]
    stp  x26, x27, [sp, #208]
    stp  x28, x29, [sp, #224]
    str  x30,      [sp, #240]

    // Save system registers (use x9 as scratch — already saved above).
    mrs  x9, sp_el0
    str  x9,       [sp, #248]
    mrs  x9, elr_el1
    str  x9,       [sp, #256]
    mrs  x9, spsr_el1
    str  x9,       [sp, #264]
    mrs  x9, esr_el1
    str  x9,       [sp, #272]

    // Call C handler: axion_kernel_handle_svc_trap_aarch64(frame).
    // x0 = pointer to the frame we just saved on the EL1 stack.
    mov  x0, sp
    bl   axion_kernel_handle_svc_trap_aarch64

    // Restore system registers before restoring GPRs.
    ldr  x9,       [sp, #264]
    msr  spsr_el1, x9
    ldr  x9,       [sp, #256]
    msr  elr_el1,  x9
    ldr  x9,       [sp, #248]
    msr  sp_el0,   x9

    // Restore general-purpose registers.
    ldp  x0,  x1,  [sp,   #0]
    ldp  x2,  x3,  [sp,  #16]
    ldp  x4,  x5,  [sp,  #32]
    ldp  x6,  x7,  [sp,  #48]
    ldp  x8,  x9,  [sp,  #64]
    ldp  x10, x11, [sp,  #80]
    ldp  x12, x13, [sp,  #96]
    ldp  x14, x15, [sp, #112]
    ldp  x16, x17, [sp, #128]
    ldp  x18, x19, [sp, #144]
    ldp  x20, x21, [sp, #160]
    ldp  x22, x23, [sp, #176]
    ldp  x24, x25, [sp, #192]
    ldp  x26, x27, [sp, #208]
    ldp  x28, x29, [sp, #224]
    ldr  x30,      [sp, #240]

    // Deallocate frame and return from exception.
    add  sp, sp, #280
    eret

// ── IRQ entry: save caller-saved regs, call C handler, restore, ERET ─────────
//
// Branched to from axion_vec_cur_spx_irq (offset 0x280) when a GIC IRQ fires
// while the kernel is running at EL1 with SP_EL1 (the normal execution mode).
// The ARM generic timer physical PPI (INTID 30) is the primary source.
//
// We save only the AAPCS64 caller-saved registers (x0–x18) plus the link
// register (x30/LR) since the C handler may clobber them.  Callee-saved
// registers (x19–x29) are preserved by the C ABI.
//
// Frame layout (160 bytes):
//   [sp +   0]  x0    [sp +   8]  x1
//   [sp +  16]  x2    [sp +  24]  x3
//   [sp +  32]  x4    [sp +  40]  x5
//   [sp +  48]  x6    [sp +  56]  x7
//   [sp +  64]  x8    [sp +  72]  x9
//   [sp +  80]  x10   [sp +  88]  x11
//   [sp +  96]  x12   [sp + 104]  x13
//   [sp + 112]  x14   [sp + 120]  x15
//   [sp + 128]  x16   [sp + 136]  x17
//   [sp + 144]  x18   [sp + 152]  x30 (LR)

    .balign 16
    .global axion_irq_entry
axion_irq_entry:
    sub  sp, sp, #160

    stp  x0,  x1,  [sp,   #0]
    stp  x2,  x3,  [sp,  #16]
    stp  x4,  x5,  [sp,  #32]
    stp  x6,  x7,  [sp,  #48]
    stp  x8,  x9,  [sp,  #64]
    stp  x10, x11, [sp,  #80]
    stp  x12, x13, [sp,  #96]
    stp  x14, x15, [sp, #112]
    stp  x16, x17, [sp, #128]
    stp  x18, x30, [sp, #144]

    bl   axion_irq_handler_aarch64

    ldp  x18, x30, [sp, #144]
    ldp  x16, x17, [sp, #128]
    ldp  x14, x15, [sp, #112]
    ldp  x12, x13, [sp,  #96]
    ldp  x10, x11, [sp,  #80]
    ldp  x8,  x9,  [sp,  #64]
    ldp  x6,  x7,  [sp,  #48]
    ldp  x4,  x5,  [sp,  #32]
    ldp  x2,  x3,  [sp,  #16]
    ldp  x0,  x1,  [sp,   #0]

    add  sp, sp, #160
    eret
