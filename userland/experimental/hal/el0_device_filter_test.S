// userland/experimental/hal/el0_device_filter_test.S
//
// Phase 15 (RFC-00C4) — Process E (tid=6, per-device filter tester).
//
// Loaded from CanonFS LBA 11 by canon_device_filter_load_and_run().
//
// Sends WaitForDevice(device_id=30) — GICv3 PPI30 (physical timer INTID).
// The WaitForDevice SVC handler reads device_id from the 16-byte request and
// stores it in FsSchedThread.device_id.
// fs_sched_timer_device_wake(intid=30) matches (did==0 || did==intid) and
// transitions E from BlockedDeviceWait → Runnable.
// fs_sched_device_wait_loop() detects E as Runnable after wfi and ERets to it.
// E resumes here, calls ExitThread.
//
// Wire format (16 bytes, RFC-00BD / RFC-00C4):
//   [0:4]   magic       KAQR
//   [4:6]   version     1
//   [6:8]   bytes       16
//   [8:12]  kind        43  (WaitForDevice)
//   [12:16] device_id   30  (GICv3 PPI30 = physical timer)
//
// Position-independent: PC-relative addressing only.
// Compilation: --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    .include "kernelcall_abi.inc"

    .section .text, "ax"
    .balign 16
    .global wait_e_entry

wait_e_entry:
    // ── Allocate 64-byte frame on EL0 process stack ───────────────────────────
    sub     sp, sp, #64

    // ── Zero response buffer at sp+16 (48 bytes) ─────────────────────────────
    stp     xzr, xzr, [sp, #16]
    stp     xzr, xzr, [sp, #32]
    stp     xzr, xzr, [sp, #48]

    // ── Build WaitForDevice wire request at sp+0..sp+15 (16 bytes) ───────────
    // [0:4] magic = KAQR
    mov     w8, #KCALL_REQ_MAGIC_LO
    movk    w8, #KCALL_REQ_MAGIC_HI, lsl #16
    str     w8, [sp]

    // [4:8] version=1, bytes=16 → packed LE: 0x00100001
    mov     w8, #0x0001
    movk    w8, #0x0010, lsl #16
    str     w8, [sp, #4]

    // [8:12] kind = WaitForDevice (43)
    mov     w8, #KCALL_WaitForDevice
    str     w8, [sp, #8]

    // [12:16] device_id = 30 (GICv3 physical timer PPI30)
    mov     w8, #30
    str     w8, [sp, #12]

    // ── SVC #1: KernelCall(WaitForDevice, device_id=30) ──────────────────────
    // EL1 saves E context, marks BlockedDeviceWait(device_id=30), redirects
    // ERET to fs_sched_device_wait_loop (wfi idle, IRQs enabled).
    // Timer IRQ fires → fs_sched_timer_device_wake(30) → device_id match → Runnable.
    // device_wait_loop ERET → E resumes here.
    mov     x0, sp
    mov     x1, #16
    add     x2, sp, #16
    mov     x3, #48
    svc     #1

    // ── SVC #2: ExitThread ────────────────────────────────────────────────────
    mov     x0, #0
    svc     #2

    // Should never reach here.
.Lspin_e:
    wfe
    b       .Lspin_e
