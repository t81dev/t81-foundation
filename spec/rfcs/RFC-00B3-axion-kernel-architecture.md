# RFC-00B3: Axion Kernel Architecture

**Status:** accepted
**Type:** standards-track
**Applies-To:** Axion kernel core, kernel/runtime boundary, HAL-to-kernel handoff
**Created:** 2026-03-11
**Updated:** 2026-03-14
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

The current integration target is:

1. firmware/host stub constructs `BootContext`
2. `hal_main` validates context and performs ethics-first boot
3. `hal_main` transfers control to an Axion kernel entry routine
4. kernel entry initializes:
   - page allocator view
   - page table root
   - scheduler state
   - IPC state
   - device arbitration state for the current platform profile
   - process-group and supervisor fault-policy state
5. kernel enters a deterministic main loop or dispatch loop
6. service-facing runtime requests read kernel-owned state through a narrow
   deterministic request/result contract

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

#### 3.5.4 Current integration result

The MMU is now consumed through the kernel runtime:

- checked translation produces explicit fault records
- those records are queued and delivered through the deterministic kernel loop
- delivered faults route into per-thread and process-group policy state

This is the first implemented bridge from MMU structure to kernel control
flow.

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

Axion now has a small process-group policy layer over the thread model:

- each spawned thread belongs to exactly one kernel-owned process group
- delivered MMU faults route into per-thread inboxes and mark the owning group
  faulted/blocked
- explicit process-group acknowledgement gates recovery after a thread inbox is
  drained
- deterministic audit records are emitted for fault delivery, quarantine,
  group-fault entry, acknowledgement, and recovery

Richer supervisor/service policy remains deferred until:

- service boundaries are defined
- a kernel-visible supervisor object exists
- capability and syscall rules are ready to be specified

---

### 3.7 IPC and Object Identity

IPC remains CanonRef-first.

Kernel requirements:

- no raw pointer transfer across kernel-visible IPC boundaries
- CanonRef identity remains stable across IPC transfer
- IPC accounting must remain deterministic

The current FIFO inbox model is acceptable as the first kernel IPC substrate.

---

### 3.7.1 Immediate Execution Plan

The first service-facing runtime request/result contract is now implemented,
including deterministic behavior for healthy vs faulted groups and stable
diagnostics above kernel-owned state. The first narrow service-facing action is
also now implemented:

- supervisor fault-group acknowledgement through the same service boundary
- supervisor-facing recovery/report status through the same contract
- deterministic device claim/release requests through the same contract
- stable audit-summary and per-device ownership detail views through the same
  contract
- successful service lifecycle transitions now flow into that same
  deterministic audit stream without widening the runtime contract

The next implementation slice after it is:

1. stabilize the new kernel-owned service runtime model above the current
   supervisor/process-group boundary
2. preserve deterministic blocked-vs-healthy service behavior
3. keep lifecycle actions narrow before any broader boundary growth

That stabilization step specifically means:

- keep the existing request/result shapes stable where possible
- preserve deterministic request outcomes for healthy vs blocked services
- preserve stable diagnostics for runtime, process-group, supervisor, fault,
  audit, device ownership, and service state
- avoid widening the contract with unrelated runtime verbs before the service
  model is settled

That service-runtime convergence step is now implemented for the current
contract surface. The service layer now includes deterministic registration,
stable service detail and supervisor inventory views, deterministic service
unregister, deterministic service suspend/resume, and same-supervisor service
lifecycle control over managed services. That same layer now also carries
explicit service health transitions with stable unhealthy-state diagnostics.
Successful service lifecycle transitions are also now visible through the same
stable audit-summary surface.
Supervisor-owned service inventory now also retains the latest managed-service
lifecycle transition metadata so supervisors can observe convergence without a
broader control surface.
Each supervisor-owned service entry now also carries its own latest lifecycle
transition metadata so inventory polling stays aligned with service-detail
polling.
The compact supervisor-status view now also carries managed-service lifecycle
counts and latest-transition metadata so supervisor health checks do not have
to depend on the richer inventory request.
The supervisor-recovery view now also carries the same managed-service
lifecycle metadata so recovery polling stays aligned with service-runtime
state.
The fault-summary view now also carries the latest managed-service lifecycle
metadata so fault-side polling can stay aligned with the same convergence
surface.
The runtime-status view now also carries aggregate managed-service lifecycle
metadata so the top-level runtime summary stays aligned with the same stable
service state.
The audit-summary view now also carries explicit managed-service lifecycle
metadata so audit polling no longer needs to infer that state only from recent
events.
The device-summary view now also carries managed-service lifecycle metadata so
device polling stays aligned with the same service-runtime convergence
surface.
The service-status view now also carries explicit last-transition metadata so a
service can observe its own latest lifecycle state without consulting broader
supervisor summaries.
That supervisor/service-runtime convergence step is now complete for the
current contract. The next step is to keep that layer stable and shift kernel
work downward into process-memory ownership and pager integration before any
syscall, capability, or broader process ABI design.
The first process-memory ownership slice is now in place as well: process
groups now bind to explicit kernel-owned address-space objects, and the stable
diagnostic views expose address-space ownership plus mapped-page counts. That
gives pager work a concrete runtime object to target without widening the
public service contract.
The first pager-groundwork slice is now in place too: delivered `Unmapped`
faults mark the owning address space as pager-needed, while
`PermissionDenied` and `InvalidTva` faults remain explicit policy failures.
That pager-needed state is visible through the existing runtime,
process-group, service, supervisor, and fault diagnostics without widening the
contract. That internal pager surface now also has a deterministic loop-owned
handoff queue: pager-needed address spaces are handed off one at a time
through kernel-owned runtime state, and diagnostics distinguish handoff-pending
from handoff-dispatched state without adding a public pager ABI. Once the
missing mapping appears, that same kernel loop now clears pager-needed state
deterministically one address space at a time, so diagnostics can distinguish
pager-needed, handed-off, and resolved state. A first real kernel-owned pager
worker now also exists, with a FIFO inbox and one active work item, so
repeated pager-needed cycles can flow through a concrete internal consumer
without adding a public pager ABI. Repeated unresolved faults on a
worker-owned address space now coalesce instead of creating duplicate pager
work items, and diagnostics expose worker-owned state plus coalesced
pager-fault counts without widening the contract. Runtime and fault summaries
now also retain pending-handoff and worker-inbox high-water marks plus worker
activation counts, and HAL coverage proves deterministic FIFO backlog handling
across two queued address spaces. Those diagnostics now also retain worker
stall cycles plus the narrower backlog-blocked subset when FIFO ordering holds
queued work behind an unresolved active item. They now also retain the even
narrower ready-backlog case when queued work is already mappable behind that
stalled active item, including current ready-backlog depth and its retained
high-water mark. Those summaries now also retain the stalled active address
space alongside the ready queued address space it was holding behind FIFO, plus
the ordinal of the latest stall event that produced that relationship. The
retained blocked-side state also carries the exact stall ordinal that exposed
the ready queued address space, plus the ready-backlog depth observed at that
same deterministic stall event. Those summaries now also retain the last
activated address space and its activation ordinal after the worker goes idle.
They now also retain the last completed pager-worker address space and its
resolution ordinal after the worker goes idle.
They now also retain the last received pager-worker address space and its
handoff ordinal after the inbox drains.
They now also expose the active pager-worker handoff ordinal while work is in
flight and clear it again once the worker goes idle.
They now also expose the next queued pager-worker address space and handoff
ordinal at the head of the FIFO inbox.
When the worker is idle and the FIFO head is still unresolved, the kernel now
also selects the earliest already-ready queued item instead of activating the
blocked head first, but the same blocked head may be bypassed at most once
until it resolves; diagnostics retain both the latest blocked/promoted bypass
pair and the latest blocked/deferred pair for that bounded rule. After that
cap fires, the worker now remains parked until the blocked head becomes ready
instead of activating the same unresolved head into another deterministic stall;
parked cycles are now retained explicitly alongside the latest blocked/ready
pair observed during that parked state, while deferral counts track parked
episodes rather than every idle parked loop tick. Live parked-ready backlog
count and its retained high-water mark are now tracked separately from
ready-behind-active backlog state. Once that blocked head finally becomes
ready again, parked-resumption transitions are also retained explicitly with
their latest resumed blocked-head identity and resumption ordinal, plus the
count of still-ready queued work that remained behind that resumed head, along
with the latest queued address space and handoff ordinal retained there. The
resumed blocked head's own handoff ordinal is now retained too, and the kernel
also retains when that resumed blocked head later resolves, including the
queued work that remained behind it at that resolution point. The first
activation that follows that parked-head resolution is now retained too, and
that queued successor is now retained through its own resolution as well.
If that once-bypassed parked head remains unresolved for a fixed number of
repeated parked cycles, the kernel now terminalizes it, removes it from the
worker queue, and retains explicit terminal-failure diagnostics instead of
retrying indefinitely. Explicitly marked boot-critical address spaces now also
auto-map their missing page through a kernel-owned policy path, resolving that
blocked head before any ready-bypass or parked-terminal rule fires and
retaining explicit boot-critical resolution diagnostics. Runtime and fault
summaries now also expose explicit boot-progress/fail state for that internal
policy through boot-critical pending counts, boot-critical terminal counts,
and direct pending/blocked booleans. That closes the current internal
boot-ready kernel slice. The next kernel work is to preserve that private
pager surface while moving outward to external boot validation before any
public pager ABI or syscall design.

That does not block narrow RFC-00B5 convergence inside the existing kernel
boundary: explicit interrupt events may now enter kernel-owned runtime state
and be delivered through the deterministic loop without widening the public
service contract.

The working execution note for this slice is:

- `experimental/ternaryos/docs/kernel_execution_plan.md`

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

1. define the first kernel-visible supervisor/service layer above the current process-group boundary
2. extend active device arbitration beyond simple claim/release ownership
3. only then expand syscall/userland semantics further
4. defer pager/lazy-allocation work until the loop-owned fault path feeds a richer runtime consumer

## 7. Open Questions

- ~~Should device arbitration state live inside the first kernel state object or in a separate kernel-owned service registry?~~ Resolved: kernel-owned service registry with capability-checked arbitration actions.
- ~~How much authority should the first supervisor/service layer have beyond the current audit-only process-group gate?~~ Resolved: explicit `KernelCapabilityKind` capability model (RFC-00B6 §5.4); capability grants are kernel-seeded or delegated, scoped to process groups.
- When should thread-first scheduling grow into a fuller process/service model? Deferred to post-EL0/EL1 SVC roundtrip validation.
- How should later capability checks attach to CanonRef and device operations without destabilizing the current seams? Deferred; current seams are stable under RFC-00B6 §5.4 and RFC-00B7 §3.1.

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
- [x] Delivered MMU faults feed a process-group runtime policy boundary that preserves thread-local fault state.
- [x] Process-group acknowledgement gates thread recovery deterministically.
- [x] Audit-only governance records are emitted deterministically for fault delivery, quarantine, acknowledgement, and recovery.
- [x] Hosted and QEMU developer lanes continue to pass after kernel entry integration.
