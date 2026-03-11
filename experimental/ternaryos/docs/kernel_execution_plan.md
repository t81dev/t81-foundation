# Axion Kernel Execution Plan

Current working release label: `Axion v0.1.0-alpha`

This note captures the next narrow kernel slice after the currently
implemented supervisor/process-group fault boundary from
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
- first service-facing runtime action: supervisor fault-group acknowledgement

Not yet implemented:

- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. Supervisor-facing recovery/report flows

After one narrow action exists, expose the smallest supervisor-facing
recovery/report sequence above it.

Initial candidates:

- request-visible fault acknowledgement results
- deterministic recovery status after acknowledgement
- supervisor-visible pending-group drains

### 2. Additional narrow actions

Only after supervisor-facing recovery/report flows are stable, add one more
narrow action.

Likely candidates:

- deterministic device claim requests
- deterministic device release requests
- supervisor-visible action rejection for faulted groups

### 3. Contract hardening

Once one action path exists, harden the contract instead of widening it:

- keep request/result types stable
- keep request outcomes deterministic
- avoid leaking kernel internals directly into services

## Non-Goals For This Slice

Do not add:

- syscalls
- userspace/kernel privilege modes
- capabilities
- process address spaces
- pager or demand-fault machinery
- shell logic inside the kernel

## Acceptance Criteria

The current kernel slice is complete when:

1. a service-facing request/result contract exists in `kernel/`
2. the contract reads kernel-owned runtime state deterministically
3. faulted and healthy groups are distinguished explicitly
4. stable service-facing diagnostics exist for group, supervisor, fault, and
   device state
5. HAL/kernel tests prove the request path and its interaction with fault state
6. one narrow service-facing action exists above the contract
7. RFC-00B3 can mark the action layer as started

## Recommended Order

1. expose supervisor-facing recovery/report flows through the current contract
2. add HAL/kernel acceptance coverage for those flows
3. update RFC-00B3 and status/docs
4. only then add another narrow action
