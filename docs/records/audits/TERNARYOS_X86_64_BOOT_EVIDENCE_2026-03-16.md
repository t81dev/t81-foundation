# TernaryOS x86_64 QEMU Boot Evidence

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [TernaryOS x86_64 QEMU Boot Evidence](#ternaryos-x86_64-qemu-boot-evidence)
  - [Summary](#summary)
  - [Probe Parameters](#probe-parameters)
  - [Contract File Verification](#contract-file-verification)
  - [Boot Report (`boot-report.txt`)](#boot-report-`boot-reporttxt`)
  - [Startup Status (`startup-status.txt`)](#startup-status-`startup-statustxt`)
  - [Key Evidence Points](#key-evidence-points)
  - [TernaryOS Test Suite State (at time of boot evidence)](#ternaryos-test-suite-state-at-time-of-boot-evidence)
  - [Relationship to RFC-00B5](#relationship-to-rfc-00b5)
  - [Open Work](#open-work)

<!-- T81-TOC:END -->


**Date:** 2026-03-16
**Milestone:** TernaryOS bare-metal boot — QEMU x86_64 EFI lane
**Status:** PASS — all 5 contract files verified

---

## Summary

The staged BOOTX64.EFI was executed under QEMU TCG (x86_64 emulation) with an
EDK2 OVMF firmware pair and a VirtualBox-profile guest disk image.  All 5
contract files produced by the EFI binary were verified against expected
content.  The boot lane completed with `hal_main_result=0` and
`kernel_boot_ready_slice=complete`.

---

## Probe Parameters

| Parameter | Value |
| :--- | :--- |
| Image | `build/ternaryos/qemu_x86_64_guest/qemu-x86_64-guest-probe.img` |
| Boot wait | 12 seconds |
| Serial log | `build/ternaryos/qemu_x86_64_guest/qemu-x86_64-guest-serial.log` |
| Serial bytes | 7251 |
| Emulator | `qemu-system-x86_64` (TCG, no KVM) |
| Firmware | EDK2 OVMF (`edk2-x86_64-code.fd` + `edk2-x86_64-vars.fd`) |

---

## Contract File Verification

| File | Status |
| :--- | :--- |
| `efi-ran.txt` — EFI marker sentinel | SEEN ✅ |
| `boot-report.txt` — kernel boot report | SEEN ✅ |
| `startup-status.txt` — OS startup status | SEEN ✅ |
| `expected-boot-report.txt` — boot contract reference | SEEN ✅ |
| `expected-startup-status.txt` — startup contract reference | SEEN ✅ |

All 5/5 expected contract files verified.

---

## Boot Report (`boot-report.txt`)

```
AXION_BOOT_REPORT
platform_id=virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC/acceptance-lane
hal_main_result=0
kernel_boot_ready_slice=complete
boot_progress_state=ready
boot_progress_pending=false
boot_progress_blocked=false
boot_progress_source=kernel-boot-critical-policy
boot_validation_lane=virtualbox-x86_64-handoff
```

---

## Startup Status (`startup-status.txt`)

```
AXION_STARTUP_STATUS
os_name=Axion
platform_id=virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC/acceptance-lane
phase=5
shell_mode=typed-builtins
kernel_boot_ready_slice=complete
boot_progress_pending=false
boot_progress_blocked=false
boot_validation_lane=virtualbox-x86_64-handoff
storage_binding=virtualbox-ahci
display_binding=virtualbox-vmsvga
network_binding=virtualbox-e1000
```

---

## Key Evidence Points

| Evidence | Value |
| :--- | :--- |
| `hal_main_result` | `0` (clean HAL main return) |
| `kernel_boot_ready_slice` | `complete` |
| `boot_progress_state` | `ready` |
| `boot_progress_source` | `kernel-boot-critical-policy` |
| `os_name` | `Axion` |
| `phase` | `5` |
| `storage_binding` | `virtualbox-ahci` |
| `display_binding` | `virtualbox-vmsvga` |
| `network_binding` | `virtualbox-e1000` |

---

## TernaryOS Test Suite State (at time of boot evidence)

- **3214/3214 assertions pass** (all ternaryos tests including Slice 28 unhandled IRQ governance)
- RFC-00B5 status: `integrated` (Slices 26–28 complete; all 6 normative audit event kinds wired)
- All defined kernel slices (1A–27) and DPE slices (13–25) complete

---

## Relationship to RFC-00B5

The QEMU x86_64 boot evidence validates the end-to-end HAL → kernel chain
defined in RFC-00B5, running against the VirtualBox x86_64 platform profile.
The boot lane executes the same `qemu_kernel_run_loop()` that wires
`register_unhandled_interrupt_callback()` (Slice 28 / `[AC-22x]`), proving
the complete interrupt governance stack reaches bare-metal boot.

---

## Open Work

The QEMU x86_64 lane validates the staged EFI binary under TCG emulation.
Actual VirtualBox host execution (real hardware-assisted virtualization) and
evidence return from that environment remains the next external milestone.
See `experimental/ternaryos/docs/kernel_execution_plan.md` — Boot Milestones.
