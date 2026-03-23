// userland/experimental/hal/el0_wait_test.S
//
// Phase 11 (RFC-00C0) — Process C (tid=4, WaitForDevice tester).
//
// Loaded from CanonFS LBA 8 by canon_identity_load_and_run(); placed at
// code-page offset 0 with sp = el0_mmu_proc_stack_top().
// Initial ERET context from EL1:
//   ELR_EL1  = proc_code_pa + 0   (this entry point)
//   SP_EL0   = el0_mmu_proc_stack_top()
//   SPSR_EL1 = 0x3C0  (EL0t, DAIF masked)
//
// Sequence:
//   1. sub sp, #64  — allocate 16B req + 48B rsp on proc stack.
//   2. Build WaitForDevice request at sp+0 (12B minimum):
//        magic=KAQR, version=1, bytes=12, kind=43 (WaitForDevice)
//   3. Zero response buffer at sp+16 (48B).
//   4. SVC #1 — KernelCall(WaitForDevice):
//        EL1 saves C context (ELR, SP_EL0, SPSR), marks C BlockedDeviceWait,
//        no Runnable threads → redirects ERET to g_axion_el1_return_pc (EL1).
//        EL1 then calls fs_sched_wake_device(4u): C → Runnable.
//        EL1 calls fs_sched_mark_running(4u) + run_proc_entry(resume_elr, sp).
//        C resumes here (instruction after svc #1).
//   5. SVC #2 — ExitThread(0):
//        RFC-00BE: no Runnable threads remain → ERET to EL1.
//        EL1 checks obs ring: fs_obs_find(4, WaitForDevice, 0) → true.
//        EL1 emits "[axion] el0: device wake OK (WaitForDevice tid=4)".
//
// Position-independent: PC-relative addressing only.
// Compilation: --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    // RFC-00BD: named KernelCallKind constants — no literal ordinals.
    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global wait_c_entry

wait_c_entry:
    // ── Allocate 64-byte frame on EL0 process stack ───────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes) ─────────────────────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build WaitForDevice wire request at sp+0..sp+11 ──────────────────────
    // [0:4]   magic   = KAQR
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // [4:6] version=1  [6:8] bytes=12  → packed: 0x000C0001
    mov     w8, #0x0001
    movk    w8, #0x000C, lsl #16
    str     w8, [sp, #4]

    // [8:12] kind = WaitForDevice
    mov     w8, #KCALL_WaitForDevice
    str     w8, [sp, #8]

    // ── SVC #1: KernelCall(WaitForDevice) ────────────────────────────────────
    // x0=req_tva(sp), x1=req_size(12), x2=rsp_tva(sp+16), x3=rsp_size(48).
    // EL1 parks C and returns to EL1 (no other Runnable threads).
    // EL1 calls fs_sched_wake_device(4), then re-ERets to C's resume point.
    // C resumes here.
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
.Lspin_c:
    wfe
    b       .Lspin_c
