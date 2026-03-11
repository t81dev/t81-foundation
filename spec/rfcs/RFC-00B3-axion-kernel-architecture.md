# RFC-00B3: Axion Kernel Architecture

**Status:** draft
**Type:** standards-track
**Applies-To:** Axion kernel core, kernel/runtime boundary, HAL-to-kernel handoff
**Created:** 2026-03-11
**Updated:** 2026-03-11
**Author:** @t81dev
**Depends on:** RFC-00B0 (HAL), RFC-00B1 (MMU), RFC-00B2 (Drivers)
**Blocks:** Axion kernel integration after Phases 1-4 subsystem bring-up

---

## 1. Summary

This RFC defines the first implementation-facing architecture for the Axion
kernel as a runtime unit, not just a collection of subsystem prototypes.

Axion is a deterministic ternary-native kernel running on top of T81VM. The
kernel core owns:

- boot handoff from the HAL
- memory translation and access-fault classification
- thread scheduling and context switching
- CanonRef-safe IPC
- minimal device arbitration for the first supported platform profile
- governance and audit hooks at kernel boundaries

Everything else remains outside the kernel core unless explicitly pulled in by
later RFCs.

This RFC is intentionally narrower than a philosophical architecture note. It
defines the concrete boundary that the current codebase should converge toward.

---

## 2. Goals

- Define what "the Axion kernel" means in the current implementation.
- Specify the boundary between kernel core and higher-level services.
- Define the boot-to-kernel control path after `hal_main`.
- Use the new MMU permission/fault model as a kernel integration point.
- Provide acceptance criteria that can drive the next implementation passes.

---

## 3. Non-Goals

- POSIX compatibility
- Unix process semantics
- legacy binary ABI support
- fully general hardware support
- immediate bare-metal parity with the hosted/QEMU developer lanes

Axion is not attempting to replicate a conventional monolithic Unix kernel in
this phase.

---

## 4. Kernel Boundary

### 4.1 Kernel Core

The Axion kernel core currently includes these subsystems:

- HAL handoff and validated `BootContext`
- ternary page allocator
- ternary radix page table
- checked MMU translation and fault classification
- deterministic 81-slot scheduler
- TISC context switching
- CanonRef-safe IPC bus
- minimal device-arbitration seams for the first supported platform profile

### 4.2 Outside Kernel Core

The following remain above the kernel boundary for now:

- shell frontend and shell session model
- CanonStore presentation workflows
- build-time startup snapshot generators
- demo applications and review artifacts
- richer storage/network/display presentation layers

### 4.3 Device Responsibility Split

The kernel core may own minimal arbitration and capability checks for device
access, but not the full policy-rich user-facing presentation of those devices.

For the current platform profile:

- storage path begins with AHCI-shaped device arbitration
- display path begins with VMSVGA-shaped presentation ownership
- network path begins with E1000-shaped frame movement

Higher-level services may build on those seams, but the kernel must be able to
account for them.

---

## 5. Boot and Control Flow

### 5.1 Current flow

The current implemented flow is:

1. firmware/host stub constructs `BootContext`
2. `hal_main` validates context and performs ethics-first boot
3. HAL currently stubs the T81VM/kernel handoff

### 5.2 Target kernel flow

The next integration target is:

1. firmware/host stub constructs `BootContext`
2. `hal_main` validates context and performs ethics-first boot
3. `hal_main` transfers control to an Axion kernel entry routine
4. kernel entry initializes:
   - page allocator view
   - page table root
   - scheduler state
   - IPC state
   - device arbitration state for the current platform profile
5. kernel enters a deterministic main loop or dispatch loop

### 5.3 Required kernel-owned entrypoint

This RFC introduces the architectural requirement for a kernel-owned entry
function, conceptually:

```cpp
int axion_kernel_main(const hal::BootContext& boot);
```

The exact symbol name may differ, but the kernel must become a first-class
handoff target rather than leaving `hal_main` as the terminal stub.

---

## 6. Memory and Fault Model

### 6.1 Virtual memory structure

Axion uses the RFC-00B1 TVA model:

- 10-trit page offset
- 20-trit VPN
- 20-level ternary radix walk

### 6.2 Page-table leaf semantics

Each mapping leaf carries:

- `phys_base`
- `owner_pid`
- `readable`
- `writable`
- `executable`

### 6.3 Fault model

Kernel-integrated MMU behavior must distinguish:

- `InvalidTva`
- `Unmapped`
- `PermissionDenied`

The kernel must treat these as explicit events, not just null translations.

### 6.4 Next integration step

The next kernel-facing implementation milestone after this RFC is:

- a page-fault reporting path that consumes `mmu_translate_checked()`
- explicit fault records that can be surfaced to the scheduler/process layer

This is the first required bridge from MMU structure to kernel control flow.

---

## 7. Execution Model

### 7.1 Scheduling

The kernel scheduling model remains deterministic:

- fixed 81-slot run queue
- explicit save/preempt/select/restore cycle
- no heuristic time-slice policy in this phase

### 7.2 Thread model

Axion is thread-first for now.

This RFC does not require a full process table before kernel integration. The
first kernel runtime may continue to schedule TISC thread contexts directly.

### 7.3 Process model

Process/group semantics are deferred until:

- fault delivery exists
- memory ownership semantics are clearer
- syscall/service boundaries are defined

---

## 8. IPC and Object Identity

IPC remains CanonRef-first.

Kernel requirements:

- no raw pointer transfer across kernel-visible IPC boundaries
- CanonRef identity remains stable across IPC transfer
- IPC accounting must remain deterministic

The current FIFO inbox model is acceptable as the first kernel IPC substrate.

---

## 9. Governance Boundary

Governance remains part of kernel responsibility at the boundary, not merely a
shell or service-layer concern.

This means:

- ethics-first boot remains below userland
- fault, scheduling, and device transitions must remain audit-friendly
- future capability checks should attach to kernel-owned object and device paths

This RFC does not define a full capability model, but it reserves that
responsibility for the kernel core.

---

## 10. Initial Platform Scope

The kernel integration target remains aligned with the current practical lanes:

- acceptance lane: VirtualBox `x86_64`
- local developer lane: QEMU AArch64 + EDK2

First supported device profile:

- VBox EFI
- AHCI
- E1000
- VMSVGA
- HPET/IOAPIC

This RFC does not retarget the project away from that profile.

---

## 11. Acceptance Criteria

- [ ] `hal_main` hands off to a kernel-owned entry routine instead of ending at a stub.
- [ ] Kernel entry initializes the allocator, MMU, scheduler, and IPC substrate from `BootContext`.
- [ ] Checked MMU translation is consumed by a kernel-facing fault/reporting path.
- [ ] Fault records distinguish `InvalidTva`, `Unmapped`, and `PermissionDenied`.
- [ ] The scheduler can continue running deterministic thread dispatch after kernel entry initialization.
- [ ] CanonRef-safe IPC remains functional across the kernel-integrated runtime path.
- [ ] Device arbitration state is initialized for the first supported storage/display/network profile.
- [ ] Hosted and QEMU developer lanes continue to pass after kernel entry integration.

---

## 12. Recommended Next Steps

Implementation should proceed in this order:

1. introduce a kernel-owned entry routine after `hal_main`
2. add a kernel-facing page-fault/reporting path
3. define the first kernel runtime state object
4. connect scheduler and IPC initialization to that state
5. only then expand syscall/userland semantics further

This keeps the current subsystem work coherent and avoids shell/userland growth
outrunning the kernel boundary again.
