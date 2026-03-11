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
- active device arbitration for the first supported platform profile
- governance and audit hooks at kernel boundaries

Everything else remains outside the kernel core unless explicitly pulled in by
later RFCs.

This RFC is intentionally narrower than a philosophical architecture note. It
defines the concrete boundary that the current codebase should converge toward.

---

## 2. Motivation

The current Axion tree has real kernel subsystems, but they still behave too
much like adjacent prototypes. HAL, MMU, scheduler, IPC, and device seams need
an explicit runtime boundary so the next work is kernel integration rather than
more disconnected subsystem growth.

## 3. Proposal

### 3.1 Goals

- Define what "the Axion kernel" means in the current implementation.
- Specify the boundary between kernel core and higher-level services.
- Define the boot-to-kernel control path after `hal_main`.
- Use the new MMU permission/fault model as a kernel integration point.
- Provide acceptance criteria that can drive the next implementation passes.

### 3.2 Non-Goals

- POSIX compatibility
- Unix process semantics
- legacy binary ABI support
- fully general hardware support
- immediate bare-metal parity with the hosted/QEMU developer lanes

Axion is not attempting to replicate a conventional monolithic Unix kernel in
this phase.

---

### 3.3 Kernel Boundary

#### 3.3.1 Kernel Core

The Axion kernel core currently includes these subsystems:

- HAL handoff and validated `BootContext`
- ternary page allocator
- ternary radix page table
- checked MMU translation and fault classification
- deterministic 81-slot scheduler
- TISC context switching
- CanonRef-safe IPC bus
- minimal device-arbitration seams for the first supported platform profile

#### 3.3.2 Outside Kernel Core

The following remain above the kernel boundary for now:

- shell frontend and shell session model
- CanonStore presentation workflows
- build-time startup snapshot generators
- demo applications and review artifacts
- richer storage/network/display presentation layers

#### 3.3.3 Device Responsibility Split

The kernel core may own minimal arbitration and capability checks for device
access, but not the full policy-rich user-facing presentation of those devices.

For the current platform profile:

- storage path begins with AHCI-shaped device arbitration
- display path begins with VMSVGA-shaped presentation ownership
- network path begins with E1000-shaped frame movement

Higher-level services may build on those seams, but the kernel must be able to
account for them.

---

### 3.4 Boot and Control Flow

#### 3.4.1 Current flow

The current implemented flow is:

1. firmware/host stub constructs `BootContext`
2. `hal_main` validates context and performs ethics-first boot
3. `hal_main` transfers control to `axion_kernel_main(...)`
4. kernel runtime bootstrap validates and summarizes initial kernel state

#### 3.4.2 Target kernel flow

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

#### 3.4.3 Required kernel-owned entrypoint

This RFC introduces the architectural requirement for a kernel-owned entry
function. The first implementation now exists, conceptually:

```cpp
int axion_kernel_main(const hal::BootContext& boot);
```

The exact symbol name may differ, but the kernel must become a first-class
handoff target rather than leaving `hal_main` as the terminal stub.

---

### 3.5 Memory and Fault Model

#### 3.5.1 Virtual memory structure

Axion uses the RFC-00B1 TVA model:

- 10-trit page offset
- 20-trit VPN
- 20-level ternary radix walk

#### 3.5.2 Page-table leaf semantics

Each mapping leaf carries:

- `phys_base`
- `owner_pid`
- `readable`
- `writable`
- `executable`

#### 3.5.3 Fault model

Kernel-integrated MMU behavior must distinguish:

- `InvalidTva`
- `Unmapped`
- `PermissionDenied`

The kernel must treat these as explicit events, not just null translations.

#### 3.5.4 Next integration step

The next kernel-facing implementation milestone after this RFC is:

- a page-fault reporting path that consumes `mmu_translate_checked()`
- explicit fault records that can be surfaced to the scheduler/process layer

This is the first required bridge from MMU structure to kernel control flow.

---

### 3.6 Execution Model

#### 3.6.1 Scheduling

The kernel scheduling model remains deterministic:

- fixed 81-slot run queue
- explicit save/preempt/select/restore cycle
- no heuristic time-slice policy in this phase

#### 3.6.2 Thread model

Axion is thread-first for now.

This RFC does not require a full process table before kernel integration. The
first kernel runtime may continue to schedule TISC thread contexts directly.

#### 3.6.3 Process model

Process/group semantics are deferred until:

- fault delivery exists
- memory ownership semantics are clearer
- syscall/service boundaries are defined

---

### 3.7 IPC and Object Identity

IPC remains CanonRef-first.

Kernel requirements:

- no raw pointer transfer across kernel-visible IPC boundaries
- CanonRef identity remains stable across IPC transfer
- IPC accounting must remain deterministic

The current FIFO inbox model is acceptable as the first kernel IPC substrate.

---

### 3.8 Governance Boundary

Governance remains part of kernel responsibility at the boundary, not merely a
shell or service-layer concern.

This means:

- ethics-first boot remains below userland
- fault, scheduling, and device transitions must remain audit-friendly
- future capability checks should attach to kernel-owned object and device paths

This RFC does not define a full capability model, but it reserves that
responsibility for the kernel core.

---

### 3.9 Initial Platform Scope

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

## 4. Determinism / Safety Considerations

- Kernel entry must preserve the ethics-first boot boundary below userland.
- MMU faults must be explicit and deterministic, not implicit misses.
- Scheduler behavior must remain insertion-order deterministic during integration.
- CanonRef-based IPC must remain the only object-transfer path across kernel-visible message boundaries.
- Device arbitration must stay narrow so platform-specific drift does not leak into userland contracts.

## 5. Compatibility

- Hosted and QEMU developer lanes remain valid while kernel entry integration evolves.
- The existing subsystem APIs should be preserved where possible during the first kernel-runtime pass.
- This RFC does not force an immediate process/syscall redesign.
- The first supported platform profile remains VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC.

## 6. Implementation Plan

1. attach the current loop-owned fault delivery path to a fuller runtime policy/process boundary
2. extend active device arbitration beyond simple claim/release ownership
3. only then expand syscall/userland semantics further
4. defer pager/lazy-allocation work until the loop-owned fault path feeds a real runtime consumer

## 7. Open Questions

- What should the first concrete kernel-runtime state object contain?
- Should device arbitration state live inside the first kernel state object or in a separate kernel-owned service registry?
- When should thread-first scheduling grow into a fuller process model?
- How should later capability checks attach to CanonRef and device operations without destabilizing the current seams?

## 8. Acceptance Criteria

- [x] `hal_main` hands off to a kernel-owned entry routine instead of ending at a stub.
- [x] Kernel entry initializes the allocator, MMU, scheduler, and IPC substrate from `BootContext`.
- [x] Checked MMU translation is consumed by a kernel-facing fault/reporting path.
- [x] Fault records distinguish `InvalidTva`, `Unmapped`, and `PermissionDenied`.
- [x] The scheduler can continue running deterministic thread dispatch after kernel entry initialization.
- [x] CanonRef-safe IPC remains functional across the kernel-integrated runtime path.
- [x] Device arbitration state is initialized for the first supported storage/display/network profile.
- [x] The kernel runtime can progress through a deterministic loop step with runtime counters.
- [x] Active device claim/release arbitration works for the first supported profile.
- [x] Recorded MMU faults are delivered through the kernel loop in deterministic FIFO order.
- [x] Hosted and QEMU developer lanes continue to pass after kernel entry integration.
