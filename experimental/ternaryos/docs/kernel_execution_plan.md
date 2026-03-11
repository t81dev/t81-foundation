# Axion Kernel Execution Plan

Current working release label: `Axion v0.1.0-alpha`

This note captures the next narrow kernel slice after the currently
implemented stable supervisor/process-group service boundary from
[RFC-00B3](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).

It is intentionally small. It exists so the next kernel work follows an
explicit sequence instead of growing organically.

## Current Kernel Position

Implemented:

- HAL-to-kernel handoff
- radix MMU with checked permission faults
- runtime-owned allocator/MMU/scheduler/IPC state
- active device arbitration for the first supported profile
- deterministic kernel-step loop
- per-thread fault inboxes
- process-group fault policy with manual acknowledgement
- audit-only supervisor layer above the process-group boundary
- first service-facing kernel request/result contract for runtime, process
  groups, supervisors, faults, and device arbitration state
- deterministic request behavior for healthy vs faulted groups
- stable service-facing diagnostics for group, supervisor, fault, and device
  state
- stable service-facing audit summaries and per-device ownership detail views
- first service-facing runtime action: supervisor fault-group acknowledgement
- supervisor-facing recovery/report flows through the current contract
- second narrow service-facing action: deterministic device claim/release
  requests through the same contract
- explicit request/action rejection semantics and stable diagnostic views across
  the current contract
- the stable service-facing contract is now ready to back a small service
  runtime layer

Not yet implemented:

- kernel-owned service registration/liveness state
- supervisor-owned service inventory
- service-facing request routing above raw process-group ids
- service blocked/faulted state at the service layer
- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. Service runtime object model

Add the smallest kernel-owned service model above the current boundary:

- service ids
- supervisor ownership
- backing process-group membership
- service liveness / blocked state
- deterministic service counters

### 2. Supervisor-mediated service requests

Move from raw process-group-oriented service usage toward a service-facing
contract:

- healthy services can query runtime state through their supervisor
- faulted/blocked services are rejected deterministically
- supervisors can inspect service state without bypassing kernel policy

### 3. Stable service diagnostics

Expose narrow structured views for:

- registered services
- owning supervisors
- service blocked/faulted state
- recent audit-visible service transitions

### 4. One narrow service action

After the service model exists, add only one action through it.

Preferred candidates:

- service register / unregister
- service suspend / resume

## Non-Goals For This Slice

Do not add:

- syscalls
- userspace/kernel privilege modes
- capabilities
- process address spaces
- pager or demand-fault machinery
- shell logic inside the kernel
- general service graph orchestration
- userland process ABI

## Acceptance Criteria

The next kernel slice is complete when:

1. a kernel-owned service runtime object model exists in `kernel/`
2. each service has deterministic supervisor ownership and backing process-group
   linkage
3. service-facing requests distinguish healthy vs blocked/faulted services
4. stable service-facing diagnostics exist for service, supervisor, fault,
   audit, and device ownership state
5. HAL/kernel tests prove service registration/state visibility and blocked
   behavior
6. one narrow service-facing action exists above the service runtime contract
7. RFC-00B3 can shift to service-runtime convergence as the next step

## Recommended Order

1. define service runtime state without changing unrelated subsystems
2. route service requests through supervisors and process-group ownership
3. expose stable service diagnostics
4. add one narrow service action
5. update RFC-00B3 and status/docs
