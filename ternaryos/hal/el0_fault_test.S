// ternaryos/hal/el0_fault_test.S
//
// Phase 18 (RFC-00C7) — Process G (tid=8, fault containment tester).
//
// Loaded from CanonFS LBA 12 by canon_fault_contain_load_and_run().
//
// Deliberately dereferences virtual address 0x0 (never mapped at EL0) to
// trigger a Data Abort.  ESR_EL1 EC = 0x24 (Data Abort from Lower EL),
// FAR_EL1 = 0x0000000000000000.
//
// The EL1 fault handler (RFC-00C7 fs_sched_fault_handler) must:
//   1. Detect EC=0x24 (not SVC) via ESR_EL1[31:26].
//   2. Mark tid=8 as Faulted; record kGovThreadFault in the gov ring.
//   3. Redirect ERET to g_axion_el1_return_pc — system stays alive.
//
// Position-independent: PC-relative addressing only.
// Compilation: --target=aarch64-unknown-elf  -ffreestanding  -nostdlib

    .section .text, "ax"
    .balign 16
    .global fault_g_entry

fault_g_entry:
    // Load from virtual address 0x0 — unmapped at EL0 in every L3 table.
    // This triggers a Data Abort synchronous exception at EL1:
    //   ESR_EL1.EC  = 0x24  (Data Abort from Lower EL, AArch64)
    //   ESR_EL1.ISV = set (load instruction)
    //   FAR_EL1     = 0x0000000000000000
    ldr     x0, [xzr]

    // Fault containment failed if execution reaches here.
    // Call ExitThread to hand back control to EL1 without hanging.
    mov     x0, #0
    svc     #2

.Lspin_g:
    wfe
    b       .Lspin_g
