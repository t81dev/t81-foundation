// userland/experimental/hal/axion_el0_init.S
//
// Phase 6 EL0 entry stub for the Axion QEMU AArch64 EFI bridge.
//
// Proves the EL0 → SVC → EL1 dispatch roundtrip using the Phase 6 KernelCall
// SVC bridge (RFC-00BC §"Phase 6").  The bridge's run_el0_init() ERets here
// from EL1 and expects execution to return to EL1 after svc #2 is processed.
//
// Phase 6 SVC ABI (defined by qemu_slice6_bridge_irq.cpp and
// qemu_slice6_el0_svc_bridge.cpp):
//
//   svc #0  GetThreadIdentity        →  x0 = tid (direct, unchanged from Phase 4)
//   svc #1  KernelCall(x0,x1,x2,x3) →  x0=req_tva x1=req_sz x2=rsp_tva x3=rsp_sz
//                                        routes to el0_svc_kernel_call_dispatch()
//   svc #2  ExitThread(x0=exit_code) →  proper thread teardown; ERET to EL1
//   svc #3  WriteSerial(x0=str_tva)  →  debug-only PL011 write (AXION_DEBUG_SERIAL)
//
// Sequence:
//   1. Allocate 64 bytes on EL0 stack for compact wire req (16B) + rsp (48B).
//   2. Build a GetThreadIdentity KernelCall request at sp+0 (16 bytes):
//        magic=0x4B415152, version=1, bytes=16, kind=10(GetThreadIdentity)
//   3. Zero the response buffer at sp+16 (48 bytes).
//   4. SVC #1 (KernelCall) with x0=sp, x1=16, x2=sp+16, x3=48.
//   5. SVC #3 (WriteSerial) with x0 = address of .Lmsg.
//   6. SVC #2 (ExitThread) with x0=0 — ERET returns to EL1.
//
// Stack layout (sp points to top of the 4 KB EL0 stack page after Phase 5):
//   [sp+0 ..sp+15]:  compact KernelCallWireRequestBlock (16B)
//   [sp+16..sp+63]:  compact KernelCallWireResponseBlock (48B)
// Total stack use: 64 B, well within the 4 KB EL0 stack page.
//
// TVA validity:
//   req  [sp, sp+16):   stack page — el0_tva_valid returns 1.
//   rsp  [sp+16, sp+64): stack page — el0_tva_valid returns 1.
//   .Lmsg VA:           code page  — el0_tva_valid returns 1.
//
// This stub is linked into BOOTAA64.EFI alongside:
//   qemu_slice6_cpp_bridge.cpp    — calls run_el0_init(); resumes the shell
//   qemu_slice6_bridge_irq.cpp    — SVC dispatcher (SVC #0-3)
//   qemu_slice6_el0_svc_bridge.cpp — KernelCall (SVC #1) dispatcher
//   aarch64_exception_vectors.S   — axion_svc_entry saves the trap frame and
//                                   calls axion_kernel_handle_svc_trap_aarch64()
//
// Compilation constraints (same as other slice6 freestanding TUs):
//   -ffreestanding  -fno-exceptions  -fno-rtti  -nostdlib
//   --target=aarch64-pc-windows-msvc

    .section .text, "ax"
    .balign 16
    .global axion_el0_entry

axion_el0_entry:
    // ── Allocate stack frame: 64 bytes (16B req + 48B rsp) ───────────────────
    // AArch64 EL0 SP must remain 16-byte aligned.
    // stack_top = s_stack_page_pa + 4096 (set by run_el0_init via el0_mmu_stack_top).
    // After sub: sp = stack_top - 64 = s_stack_page_pa + 4032 (16B aligned).
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes = 6 × 8B) ───────────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build compact KernelCallWireRequestBlock at sp+0..sp+15 ──────────────
    //
    // offset [0:4]  magic = 0x4B415152 (kKernelAbiWireRequestMagic)
    //   LE bytes in memory: 0x52, 0x51, 0x41, 0x4B
    mov     w8, #0x5152
    movk    w8, #0x4B41, lsl #16
    str     w8, [sp]

    // offset [4:6]  version = 1  (uint16_t LE → 0x01, 0x00)
    // offset [6:8]  bytes   = 16 (uint16_t LE → 0x10, 0x00)
    // Pack both into a single 32-bit store: 0x00100001
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    // offset [8:12] kind = 10 (GetThreadIdentity; Yield=0 … GetThreadIdentity=10)
    mov     w8, #10
    str     w8, [sp, #8]

    // offset [12:16] pad = 0 (reserved field)
    str     wzr, [sp, #12]

    // ── SVC #1: KernelCall(GetThreadIdentity) ────────────────────────────────
    // x0 = req_tva (sp+0), x1 = req_size (16),
    // x2 = rsp_tva (sp+16), x3 = rsp_size (48).
    // Both TVAs are within the EL0 stack page → el0_tva_valid passes.
    // On return: response written at sp+16; caller_tid at sp+16+44 = sp+60.
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #3: WriteSerial — emit Phase 6 EL0 init banner ───────────────────
    // x0 = VA of .Lmsg (code page, el0_tva_valid returns 1).
    adr     x0, .Lmsg
    svc     #3

    // ── SVC #2: ExitThread (exit_code = 0) ───────────────────────────────────
    // The SVC dispatcher patches ELR_EL1 = g_axion_el1_return_pc and
    // SPSR_EL1 = 0x5 (EL1h, DAIF unmasked); ERET resumes EL1 execution.
    mov     x0, #0
    svc     #2

    // Should never reach here.  Park in a WFE loop to avoid corrupting state.
.Lspin:
    wfe
    b       .Lspin

    .balign 4
.Lmsg:
    .asciz  "[axion] el0: init OK (tid=1)\r\n"

    // .size omitted — ELF-only directive; not valid for COFF/PE targets.
