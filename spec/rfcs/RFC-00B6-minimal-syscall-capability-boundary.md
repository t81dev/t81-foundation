# RFC-00B6: Minimal Syscall and Capability Boundary

**Status:** draft
**Type:** standards-track
**Applies-To:** Axion kernel ABI, kernel/user boundary, capability checks, service-facing execution contract
**Created:** 2026-03-12
**Updated:** 2026-03-13
**Author:** @t81dev
**Depends on:** RFC-00B1 (MMU), RFC-00B3 (Axion Kernel Architecture), RFC-00B4 (Axion Userland Service Contract), RFC-00B5 (Governed Event Interrupt Model)
**Blocks:** first real userland execution path, non-demo service runtime, executable/kernel ABI convergence

---

## 1. Summary

This RFC defines the smallest kernel ABI Axion should implement next.

The goal is not a general-purpose syscall table. The goal is a narrow,
deterministic, capability-checked boundary that lets userland code:

- identify itself to the kernel
- exchange messages with kernel-managed endpoints
- observe and acknowledge fault state
- request memory mappings through explicit kernel objects
- claim or release explicitly delegated device capabilities
- terminate or yield without relying on ad hoc demo-side control paths

This RFC intentionally narrows the problem from "design userland" to "define
the minimum contract that stops kernel growth from staying in internal-only
APIs."

## 2. Motivation

The kernel refactor is now far enough along that the main architectural blocker
is not implementation concentration. It is the lack of a real kernel/user
boundary.

Today, the kernel exposes rich internal service queries and actions, but those
surfaces are still effectively in-kernel control APIs. They are not yet a
minimal ABI for real userland tasks.

Without a narrow syscall/capability boundary:

- service behavior keeps being modeled as internal kernel actions
- shell and demo layers cannot migrate onto a real execution boundary
- future executable loading would have no stable request surface
- device and memory access rules remain implicit in internal state

This RFC defines the smallest boundary that solves those problems without
overcommitting to a large or Unix-like syscall design.

## 3. Goals

- define a minimal Axion kernel ABI
- keep all operations deterministic and auditable
- make authority explicit through capabilities, not ambient privilege
- preserve the process-group, supervisor, and fault model already present in
  the kernel
- provide a migration path from internal service actions to real user-visible
  kernel requests

## 4. Non-Goals

- POSIX compatibility
- file descriptor semantics
- fork/exec emulation
- signals
- general virtual filesystem design
- final executable object format
- dynamic linking
- a full userspace libc contract

## 5. Proposal

### 5.1 ABI Model

Axion should expose a small, message-oriented syscall boundary instead of a
wide trap table.

Conceptually:

1. a user thread places a canonical request block in shared memory
2. the thread invokes a single kernel entry operation
3. the kernel validates:
   - calling thread identity
   - process-group state
   - attached capabilities
   - request shape and payload bounds
4. the kernel executes exactly one request deterministically
5. the kernel writes a canonical response block
6. continuation returns through the normal scheduler/kernel flow

This keeps the public ABI narrow while allowing request kinds to evolve inside
typed payloads.

The current implementation now covers three nested layers of that boundary:

- a typed in-kernel dispatcher: `axion_kernel_call(...)`
- a fixed-size canonical wire block layer:
  - `KernelCallWireRequestBlock`
  - `KernelCallWireResponseBlock`
  - `axion_kernel_call_wire(...)`
- a raw byte-span bridge and exported hosted C entrypoint:
  - `axion_kernel_call_wire_bytes(...)`
  - `axion_kernel_call_wire_tva(...)`
  - `ternaryos_kernel_bootstrap_c(...)`
  - `ternaryos_kernel_destroy_c(...)`
  - `ternaryos_kernel_call_c(...)`
  - `ternaryos_kernel_call_tva_c(...)`

### 5.2 Kernel Entry Shape

This RFC does not require a finalized instruction encoding, but it does require
one logical entrypoint:

```text
kernel_call(request_ptr, response_ptr)
```

Where:

- `request_ptr` points to a user-readable request block
- `response_ptr` points to a user-writable response block
- both pointers must lie in the caller's address space
- the kernel validates both regions before touching payloads

The kernel may later map this logical operation onto a TISC opcode, a governed
event, or a narrow trap shim. That encoding choice is deferred. The ABI shape
is not.

Current implementation note:

- the logical entrypoint is now represented concretely by
  `ternaryos_kernel_bootstrap_c(const TernaryOsBootContext* ctx)`
  `→ opaque kernel_state handle`
  `→ ternaryos_kernel_call_c(...)`
  `→ ternaryos_kernel_destroy_c(void* kernel_state)`
- the current hosted C bridge therefore owns an explicit runtime-handle
  lifecycle in addition to the raw request/response call
- the logical entrypoint is still represented concretely by
  `ternaryos_kernel_call_c(void* kernel_state, const void* request_bytes,
  size_t request_size, void* response_bytes, size_t response_size)`
- this is still a hosted/runtime bridge, not a final hardware trap path
- request and response payloads currently use fixed-size wire blocks rather
  than variable-length user memory objects
- the current mapped-memory bridge is intentionally narrow: it copies those
  fixed wire blocks through mapped TVA pages backed by a kernel-owned hosted
  page store, not a full userspace object model
- the current mapped-memory bridge now derives the caller address space from
  the running thread/runtime context rather than trusting a caller-supplied
  address-space identifier
- the current mapped-memory bridge now also validates request and response
  spans explicitly before dispatch
- invalid request spans now return a structured wire response with
  `KernelCallStatus::InvalidRequest`,
  `KernelCallRejection::InvalidAddressSpaceSpan`, and a retained MMU fault
  record when the response span is still writable
- invalid response spans still fail the bridge outright because the kernel has
  no safe place to write a structured error block in that case

### 5.3 Minimum Request Families

The first ABI revision should support only these request families.

Implemented today through the typed and wire ABI:

- `Yield`
- `SpawnThreadInCallerGroup`
- `SpawnThreadUnderSupervisor`
- `RegisterThreadEntryDescriptor`
- `RegisterExecutableObject`
- `QueryExecutableObject`
- `SpawnThreadFromExecutableObject`
- `SpawnThreadFromEntryDescriptor`
- `SpawnThreadForService`
- `GetThreadIdentity`
- `QueryThreadExecutionState`
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
- `QuerySupervisorServiceStatus`
- `QuerySupervisorServiceInventory`
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

#### 5.3.1 Thread / Execution

- `Yield`
- `SpawnThreadInCallerGroup`
- `SpawnThreadUnderSupervisor`
- `RegisterThreadEntryDescriptor`
- `RegisterExecutableObject`
- `QueryExecutableObject`
- `SpawnThreadFromExecutableObject`
- `SpawnThreadFromEntryDescriptor`
- `SpawnThreadForService`
- `QueryThreadExecutionState`
- `ExitThread`
- `GetThreadIdentity`

Reason:

- these are the minimum operations needed to stop relying on internal-only
  scheduler/demo control paths
- current implementation detail:
  both spawn calls now accept a compact initial thread descriptor carrying
  `pc`, `sp`, `register0`, `label`, and halted/active state, and that
  descriptor is preserved across the typed ABI, fixed-size wire blocks, and
  exported hosted C bridge
- current implementation detail:
  process groups can now register named reusable thread entry descriptors,
  CanonRef-backed executable objects, and services can now retain an entry
  descriptor at service-registration time for later execution through
  `SpawnThreadForService`
- current implementation detail:
  the executable-object path is intentionally still narrow: registration binds
  a CanonRef identity to a stored spawn descriptor, query returns that stored
  execution descriptor, and spawn reuses it through the typed, fixed-size
  wire, and hosted C ABI paths
- current implementation detail:
  executable-object register/query/spawn responses on the fixed-size wire and
  hosted C layers now carry that stored execution descriptor too, not only
  the CanonRef identity
- current implementation detail:
  `RegisterService` can now also bind a service to one of those registered
  executable objects by CanonRef, and the service register/query/spawn paths
  preserve that backing executable identity
- current implementation detail:
  stored service entry descriptors are now exposed through the typed
  service/status control plane and through fixed-size wire / hosted C
  service register/query responses
- current implementation detail:
  supervisor-scoped service inventory is now exposed through the typed ABI and
  carried through fixed-size wire / hosted C responses as a bounded set of
  compact service entries
- current implementation detail:
  wire and hosted C coverage now verifies that those bounded supervisor
  service entries preserve service names and stored entry descriptors
- current implementation detail:
  supervisors can now query a single managed service directly through a
  dedicated service-status path, including when ordinary service status is
  deferred by unhealthy state
- current implementation detail:
  `QueryThreadExecutionState` returns the seeded execution context for the
  caller or a same-supervisor target thread, including `pc`, `sp`,
  `register0`, `label`, and running/halted/active state

#### 5.3.2 IPC

- `SendMessage`
- `ReceiveMessage`
- `PollEndpoint`

Reason:

- message passing is already the closest kernel-native service seam
- this makes IPC the primary user-visible composition primitive

#### 5.3.3 Fault / Recovery

- `ReadFaultInbox`
- `AcknowledgeThreadFault`
- `ReadProcessGroupFaultState`

Reason:

- the kernel already has explicit fault inboxes and recovery gates
- userland supervisors need a real boundary to participate in that model

#### 5.3.4 Memory Object Requests

- `MapObject`
- `UnmapRange`
- `QueryAddressSpace`

Reason:

- user code needs a controlled way to request memory presence
- this preserves the current pager/address-space direction without exposing raw
  page-table mutation

#### 5.3.5 Service / Endpoint Registration

- `RegisterServiceEndpoint`
- `UnregisterServiceEndpoint`
- `QueryServiceEndpoint`

Reason:

- current service actions are still kernel-internal
- the real public seam should be endpoint-oriented rather than service-state
  mutation first

#### 5.3.6 Device Arbitration

- `ClaimDevice`
- `ReleaseDevice`
- `QueryDevice`

Reason:

- device ownership is already modeled explicitly in the kernel
- this is the narrowest way to expose it without inventing a larger driver ABI

### 5.4 Capability Model

Every request above must be capability-checked.

Capabilities are not file descriptors and not ambient process flags. They are
kernel-issued, unforgeable authority tokens bound to a process group.

The first capability classes should be:

- `CapIpcEndpoint(endpoint_id)`
- `CapFaultObserve(process_group_id)`
- `CapFaultAcknowledge(process_group_id)`
- `CapMemoryMap(address_space_id, object_id, rights)`
- `CapServiceRegister(supervisor_id)`
- `CapDeviceClaim(device_name)`
- `CapDeviceObserve(device_name)`

The currently implemented prototype exposes a narrower typed capability set:

- `Yield`
- `IpcSend`
- `IpcReceive`
- `FaultObserve`
- `FaultAcknowledge`

Those capabilities are represented as kernel-issued `KernelCapabilityRecord`
values with stable `record_id` identities bound to process groups.

The current implementation also distinguishes:

- kernel-seeded capabilities granted during process-group creation
- delegated capabilities granted later by a same-supervisor caller

Delegated capabilities retain delegator provenance on the record itself:

- `delegated_by_process_group_id`
- `delegated_by_supervisor_id`

### 5.5 Capability Rules

The first ABI revision should enforce these rules:

- capabilities are granted to process groups, not individual threads
- a thread may exercise only capabilities owned by its current process group
- a faulted process group may have ordinary requests rejected deterministically
- recovery-related requests are exempt where needed so the fault pipeline
  remains usable
- capabilities are monotonic during a kernel step: a request cannot observe
  half-revoked state
- capability record identity is kernel-owned: callers may query and revoke by
  `record_id`, but may not forge grant-time record identities
- capability provenance is explicit: the ABI distinguishes kernel-seeded
  capabilities from delegated capabilities and preserves delegator identity
  for delegated grants
- supervisors may inspect bounded recent capability-transition history for the
  process groups they own and may target revocation using an observed
  transition sequence

### 5.6 Request Rejection Model

Every request must return one of:

- `Ok`
- `InvalidRequest`
- `CapabilityDenied`
- `FaultedCaller`
- `NotFound`
- `Conflict`
- `RetryLater`
- `PolicyDenied`

The boundary must reject deterministically and without partial side effects.

The ABI should also expose explicit rejection reasons for the most common
policy and validation failures. The currently implemented slice already uses
this shape and should be treated as the baseline for follow-on requests:

- `MissingCallerThread`
- `MissingCallerProcessGroup`
- `FaultedCaller`
- `MissingDestinationThread`
- `MissingMessage`
- `IpcSendFailed`
- `IpcReceiveEmpty`
- `MissingTargetThread`
- `CrossProcessGroupTarget`
- `FaultInboxEmpty`
- `MissingCapability`
- `MissingTargetProcessGroup`
- `MissingAddressSpace`
- `MissingBootCriticalValue`
- `InvalidCapabilityRecordId`
- `MissingCapabilityTransition`
- `MissingDelegationScope`
- `MissingSupervisor`
- `SupervisorMismatch`
- `ForeignSupervisorScope`
- `ForeignAddressSpace`
- `MissingServiceName`
- `MissingService`
- `ServiceRequestRejected`
- `ServiceActionRejected`

These rejections are not all equivalent and should stay distinct:

- `SupervisorMismatch` is used for supervisor-targeted control operations where
  the caller is attempting to act on the wrong supervisor domain
- `ForeignSupervisorScope` is used for read-oriented ABI queries where the
  caller is attempting to inspect runtime, fault, or process-group state
  outside its supervisory scope
- `ForeignAddressSpace` is used for owner-bound address-space control requests
  such as boot-critical toggling
- `InvalidCapabilityRecordId` is used for malformed capability record queries
  and forged grant-time capability record identities
- `MissingCapabilityTransition` is used for sequence-based capability control
  requests that refer to a transition not present in retained supervisor
  capability history
- `MissingDelegationScope` is used for bulk delegated-capability revocation
  requests that omit the delegator provenance needed to define the revocation
  set

This distinction matters because it keeps diagnostics precise and prevents the
ABI from collapsing unrelated policy failures into one generic denial bucket.

### 5.7 Memory Ownership Boundary

This RFC makes one architectural restriction explicit:

- userland must not request arbitrary page-table edits

Instead, userland interacts with kernel-owned memory objects and declared
mapping rights. The kernel then decides whether:

- the mapping is valid
- the capability covers it
- the mapping can be established immediately
- the request must enter pager-managed resolution flow

This preserves the current direction where the pager remains a kernel policy
engine rather than becoming a raw user-controlled mapping API.

### 5.8 Supervisor Boundary

Supervisors remain special in policy, but not magical in transport.

A supervisor should use the same kernel-call path as other userland code. The
difference is capability scope, not a separate secret ABI.

That means supervisor-only actions must be implemented as ordinary request
families plus stronger capability checks.

### 5.9 Mapping from Current Kernel APIs

The current kernel ABI implementation already covers an initial vertical slice.
It is still in-kernel and typed, not yet a raw user-pointer boundary, but it
is the concrete migration path this RFC is describing.

Implemented request kinds:

- `Yield`
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

Current policy rules already enforced in code:

- process-group memory queries are limited to the caller's own group or a
  same-supervisor target
- runtime and fault summary queries require a matching `supervisor_id` for
  non-kernel callers
- address-space boot-critical control is owner-bound and rejects foreign
  address spaces explicitly
- fault observation and acknowledgement remain available while the caller group
  is faulted so recovery paths do not self-deadlock
- capability records receive stable kernel-issued identifiers and are queryable
  by `record_id`
- capability records expose whether they are kernel-seeded or delegated, and
  delegated capabilities retain delegator process-group and supervisor
  provenance
- supervisor capability inventory exposes bounded recent capability-transition
  history, and the ABI exposes that same history directly through
  `QueryCapabilityTransitionHistory`
- delegated capabilities are directly queryable by delegator
  process-group/supervisor provenance through `QueryDelegatedCapabilities`
- supervisors can also query compact delegation summary counts through
  `QuerySupervisorDelegationSummary`, grouped by target process group and
  delegator provenance
- supervisor capability-transition history retains the same provenance so
  grants can be inspected without diffing inventory snapshots
- `RevokeCapability` may resolve its target through a previously observed
  capability-transition sequence instead of requiring an immediate inventory
  re-scan
- `RevokeDelegatedCapabilities` may remove all delegated capabilities from a
  managed process group matching a specific delegator process-group/supervisor
  provenance, while preserving kernel-seeded capabilities

This means the RFC is no longer purely aspirational. It now defines the next
expansion stages on top of an implemented ABI core.

The current internal surfaces map onto the future ABI as follows:

- `axion_kernel_ipc_send/recv` -> `SendMessage` / `ReceiveMessage`
- fault inbox and ack helpers -> `ReadFaultInbox` / `AcknowledgeThreadFault`
- service query views -> `QueryServiceEndpoint` and later supervisor queries
- device claim/release -> `ClaimDevice` / `ReleaseDevice`
- internal service register/unregister actions -> future endpoint/service
  registration requests

This RFC does not require deleting the internal kernel APIs immediately. It
requires future user-facing work to converge toward this public boundary.

## 6. Determinism and Safety

This boundary is preferred because it preserves Axion's existing strengths.

Determinism:

- one logical kernel-call path
- typed request families
- explicit rejection codes
- capability-checked authority
- compatibility with audited fault and interrupt sequencing

Safety:

- no ambient privilege
- no raw device or page-table mutation from userland
- no bypass around the process-group fault gate
- supervisor powers remain explicit and enumerable

## 7. Implementation Plan

1. Define a canonical request/response block layout in code.
2. Introduce a logical `kernel_call(...)` entry path in the runtime.
3. Implement the first request families for:
   - `Yield`
   - `SendMessage`
   - `ReceiveMessage`
   - `ReadFaultInbox`
   - `AcknowledgeThreadFault`
   - `ClaimDevice`
   - `ReleaseDevice`
4. Represent capabilities as kernel-issued records bound to process groups.
5. Migrate at least one existing shell/demo control path onto the new boundary.
6. Only after that, widen into service registration and memory object mapping.

## 8. Acceptance Criteria

This RFC should be considered implemented enough to unblock real userland work
when:

- a user thread can enter the kernel through one narrow request path
- the kernel validates request and response buffers
- IPC works through that path
- fault acknowledgement works through that path
- device claim/release works through that path
- capability denial is explicit and deterministic
- the shell/demo stack can use the boundary instead of only internal helpers

## 9. References

- [RFC-00B1: Ternary MMU](RFC-00B1-ternary-mmu.md)
- [RFC-00B3: Axion Kernel Architecture](RFC-00B3-axion-kernel-architecture.md)
- [RFC-00B4: Axion Userland Service Contract](RFC-00B4-userland-service-contract.md)
- [RFC-00B5: Governed Event Interrupt Model](RFC-00B5-governed-event-interrupt-model.md)
- [kernel_architecture_audit.md](../../experimental/ternaryos/docs/kernel_architecture_audit.md)
- [kernel_engineering_follow_on_plan.md](../../experimental/ternaryos/docs/kernel_engineering_follow_on_plan.md)
