# Axion Kernel Engineering Follow-On Plan

Date: 2026-03-13
Basis: [kernel_architecture_audit.md](kernel_architecture_audit.md)

## Purpose

This plan turns the architecture audit into a concrete delivery sequence. The
goal is to reduce kernel structural risk first, then open the path to real ABI
and target bring-up work. The immediate priority is not new surface area. It is
controlled decomposition of the current kernel monolith while preserving all
existing behavior and tests.

## Current Position

The kernel is already proving:

- HAL-to-kernel handoff
- runtime-owned allocator, page table, scheduler, IPC, and device arbitration
- deterministic fault, interrupt, and pager event flow
- process-group, supervisor, and service control-plane behavior
- strong runtime/fault/audit/device/status diagnostics

The original structural issue was concentration of logic in:

- `experimental/ternaryos/kernel/kernel_runtime.cpp`

That concentration has now been reduced materially. The kernel runtime has been
split into:

- `kernel_runtime.cpp` for top-level coordination, access checks, IPC, and entry
  glue
- `kernel_views.cpp` for read-only runtime/status view building
- `kernel_queries.cpp` for service-facing query handling
- `kernel_actions.cpp` for service-facing mutation handling
- `kernel_faults.cpp` for fault recording, delivery, quarantine, and recovery
- `kernel_interrupts.cpp` for interrupt intake and delivery bookkeeping
- `kernel_pager.cpp` for pager handoff and worker orchestration
- `kernel_lifecycle.cpp` for bootstrap and thread/group/supervisor lifecycle
- `kernel_runtime_utils.cpp` for shared audit and device-claim helpers

## Immediate Goal

Extract kernel responsibilities into explicit subsystem-oriented units without
changing semantics.

Success condition for the first follow-on slice:

- all existing `ternaryos` tests still pass unchanged
- no public behavior change
- `kernel_runtime.cpp` becomes smaller and more obviously orchestration-only

Current status:

- achieved for the implementation split
- remaining work is consolidation, documentation alignment, and deciding
  whether to split the shared header surface
- the first ABI hardening lane is now materially implemented under RFC-00B6,
  including typed kernel-call dispatch, thread identity/spawn/termination,
  same-supervisor execution control, process-group-scoped capabilities,
  supervisor-scoped capability mutation, service lifecycle control,
  supervisor recovery control and inspection, process-group memory
  inspection, boot-critical address-space control, and explicit rejection
  taxonomy for missing supervisor, foreign supervisor scope, foreign address
  space, and missing boot-critical control values
- the canonical transport path is now present too:
  - `axion_kernel_call(...)`
  - `axion_kernel_call_wire(...)`
  - `axion_kernel_call_wire_bytes(...)`
  - `ternaryos_kernel_bootstrap_c(...)`
  - `ternaryos_kernel_destroy_c(...)`
  - `ternaryos_kernel_call_c(...)`

## ABI Status

RFC-00B6 is no longer only a planning document.

Implemented kernel-call slice:

- `Yield`
- `SpawnThreadInCallerGroup`
- `SpawnThreadUnderSupervisor`
- `GetThreadIdentity`
- `ExitThread`
- `SendMessage`
- `ReceiveMessage`
- `ReadFaultInbox`
- `AcknowledgeThreadFault`
- `AcknowledgeSupervisorFaultGroup`
- `QueryProcessGroupMemory`
- `SetAddressSpaceBootCritical`
- `QueryRuntimeStatus`
- `QueryFaultSummary`
- `QuerySupervisorStatus`
- `QuerySupervisorRecoveryStatus`
- `QuerySupervisorCapabilityInventory`
- `QuerySupervisorDelegationSummary`
- `QueryCapabilityTransitionHistory`
- `QueryCapabilities`
- `QueryDelegatedCapabilities`
- `QueryCapabilityRecord`
- `GrantCapability`
- `RevokeCapability`
- `RevokeDelegatedCapabilities`
- `RegisterService`
- `QueryServiceStatus`
- `SuspendService`
- `ResumeService`
- `MarkServiceUnhealthy`
- `MarkServiceHealthy`

Current follow-on priority is no longer ABI existence. It is ABI scope
discipline:

- keep rejection taxonomy precise
- keep supervisor scope rules consistent across read and control paths
- avoid reopening internal-only control shortcuts now that a typed ABI path
  exists
- preserve the kernel-owned capability-management model now that capability
  records, supervisor transition history, and sequence-based revocation are all
  live on the ABI
- preserve the newer provenance rules as well: kernel-seeded and delegated
  capabilities now carry different semantics and should not collapse back into
  anonymous capability bits
- keep the new wire and C-bridge layers narrow so follow-on syscall work
  extends one boundary instead of reopening parallel entry paths

## Recommended Execution Order

### Slice 1: Documentation Wiring

Objective:

- make the audit and this plan first-class repo documents

Done when:

- `docs/README.md` references both documents
- progress/review docs reference both documents where appropriate

### Slice 2: Diagnostics Extraction

Objective:

- move pure status/view construction out of `kernel_runtime.cpp`

Candidate responsibilities to extract:

- runtime status view construction
- process-group view construction
- supervisor and recovery view construction
- service status and inventory view construction
- fault summary construction
- audit summary construction
- device summary construction

Likely file shape:

- `experimental/ternaryos/kernel/kernel_views.hpp`
- `experimental/ternaryos/kernel/kernel_views.cpp`

Reason:

- this is the lowest-risk extraction because it is largely read-only over
  runtime state

Success condition:

- no behavior changes
- all tests still pass

Status:

- complete

### Slice 3: Fault and Audit Extraction

Objective:

- move fault recording, quarantine, group blocking, and audit record helpers
  into their own manager/helper unit

Candidate responsibilities:

- `record_fault(...)`
- audit record append helpers
- thread quarantine and recovery helpers
- process-group and supervisor acknowledgement helpers

Likely file shape:

- `experimental/ternaryos/kernel/kernel_faults.hpp`
- `experimental/ternaryos/kernel/kernel_faults.cpp`

Reason:

- fault and recovery behavior is already one of the strongest coherent
  subsystems; it should become a real implementation boundary

Status:

- complete

### Slice 4: Pager Extraction

Objective:

- isolate internal pager-worker logic from the main kernel step loop

Candidate responsibilities:

- pager fault state recording
- handoff queuing
- worker activation policy
- ready-bypass policy
- parked/terminal/boot-critical resolution bookkeeping
- pager resolution application

Likely file shape:

- `experimental/ternaryos/kernel/kernel_pager.hpp`
- `experimental/ternaryos/kernel/kernel_pager.cpp`

Reason:

- pager logic is currently the densest policy cluster in the kernel

Status:

- complete

### Slice 5: Service Control Extraction

Objective:

- isolate service and supervisor control-plane logic from the runtime loop

Candidate responsibilities:

- service registration/unregistration
- suspend/resume
- health transitions
- service request validation
- service action validation
- service lifecycle audit transitions

Likely file shape:

- `experimental/ternaryos/kernel/kernel_services.hpp`
- `experimental/ternaryos/kernel/kernel_services.cpp`

Status:

- complete in implementation shape, but split across `kernel_queries.cpp` and
  `kernel_actions.cpp` rather than a single `kernel_services.cpp`

### Slice 6: Interrupt Extraction

Objective:

- isolate kernel-owned interrupt queue intake, delivery, and accounting

Candidate responsibilities:

- interrupt recording
- interrupt delivery bookkeeping
- per-source accounting
- interrupt audit correlation retention

Likely file shape:

- `experimental/ternaryos/kernel/kernel_interrupts.hpp`
- `experimental/ternaryos/kernel/kernel_interrupts.cpp`

Status:

- complete

### Slice 7: Lifecycle / Utility Consolidation

Objective:

- move bootstrap, spawn paths, and remaining shared helpers out of the
  coordinator file

Implemented shape:

- `experimental/ternaryos/kernel/kernel_lifecycle.cpp`
- `experimental/ternaryos/kernel/kernel_runtime_utils.cpp`

Status:

- complete

## Consolidation Decision

The remaining shared header surface in `kernel_main.hpp` should not be split
yet.

Reason:

- the extracted `.cpp` units still share one concrete runtime-state contract
- there is not yet a stable public/private subsystem API boundary
- splitting the header now would mostly create churn, not architectural clarity

Recommended trigger for a header split:

- when runtime-state ownership is narrowed enough that subsystem-local types
  and helpers can move behind dedicated headers without forcing circular
  includes or duplicating view/state definitions

## Post-Refactor Strategic Paths

Once the kernel is structurally decomposed, the next major path should be
chosen explicitly.

### Path A: Syscall / Capability Boundary

Best if the goal is real userland progression.

Deliverables:

- minimal syscall ABI
- capability/authority model
- kernel/user request boundary
- migration path from current service-control API

### Path B: Real Target Interrupt and Driver Bring-Up

Best if the goal is external execution proof.

Deliverables:

- target interrupt-controller implementation
- timer-driven scheduling
- one real end-to-end device lane

### Path C: Real Process / Address-Space Execution Model

Best if the goal is VM and task maturity.

Deliverables:

- per-address-space page tables
- executable/task lifecycle
- teardown and resource accounting

## Recommended Next Strategic Path

Recommended after structural decomposition:

1. diagnostics extraction
2. fault/pager/service extraction
3. syscall/capability boundary definition

Reason:

- the current service layer is the closest existing seam to a future kernel ABI
- real userland and real services are blocked until that boundary exists
- target bring-up will still matter, but ABI clarity will prevent more
  monolith growth while doing it

## Concrete Near-Term Task List

1. Update docs to reflect the completed subsystem split.
2. Keep `kernel_main.hpp` unified until a real subsystem API boundary exists.
3. RFC-00B6 now defines the minimum syscall/capability boundary to pursue next.
4. Decide whether the next implementation step is implementing that ABI or real target
   bring-up.

## Completion Markers

This follow-on plan should be considered complete when:

- `kernel_runtime.cpp` is mostly orchestration and top-level flow
- subsystem-specific logic has named implementation boundaries
- existing test behavior remains unchanged
- the next ABI or target bring-up milestone can proceed without further
  monolith expansion
