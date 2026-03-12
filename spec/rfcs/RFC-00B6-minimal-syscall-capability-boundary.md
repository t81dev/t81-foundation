# RFC-00B6: Minimal Syscall and Capability Boundary

**Status:** draft
**Type:** standards-track
**Applies-To:** Axion kernel ABI, kernel/user boundary, capability checks, service-facing execution contract
**Created:** 2026-03-12
**Updated:** 2026-03-12
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

### 5.3 Minimum Request Families

The first ABI revision should support only these request families.

#### 5.3.1 Thread / Execution

- `Yield`
- `ExitThread`
- `GetThreadIdentity`

Reason:

- these are the minimum operations needed to stop relying on internal-only
  scheduler/demo control paths

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

### 5.5 Capability Rules

The first ABI revision should enforce these rules:

- capabilities are granted to process groups, not individual threads
- a thread may exercise only capabilities owned by its current process group
- a faulted process group may have ordinary requests rejected deterministically
- recovery-related requests are exempt where needed so the fault pipeline
  remains usable
- capabilities are monotonic during a kernel step: a request cannot observe
  half-revoked state

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
