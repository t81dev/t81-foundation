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

Not yet implemented:

- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. Request ownership and fault interaction rules

Specify how requests behave when groups are healthy versus faulted.

Required outcomes:

- requests remain deterministic
- faulted groups are classified explicitly
- supervisor acknowledgement remains the recovery gate
- no hidden retries or automatic restart behavior

### 2. Service-facing diagnostics

Expose enough stable diagnostics for a service layer to consume kernel state
without reading kernel internals directly.

Examples:

- runtime counters
- fault backlog summary
- process-group state summary
- supervisor pending-group summary
- device arbitration summary

### 3. Service-facing runtime actions

After the read-mostly contract is stable, add one narrow action layer above it.

Initial candidates:

- explicit fault acknowledgement through the service boundary
- deterministic device claim/release requests
- supervisor-visible rejection for requests from faulted groups

## Non-Goals For This Slice

Do not add:

- syscalls
- userspace/kernel privilege modes
- capabilities
- process address spaces
- pager or demand-fault machinery
- shell logic inside the kernel

## Acceptance Criteria

The next kernel slice is complete when:

1. a service-facing request/result contract exists in `kernel/`
2. the contract reads kernel-owned runtime state deterministically
3. faulted and healthy groups are distinguished explicitly
4. HAL/kernel tests prove the request path and its interaction with fault state
5. RFC-00B3 can mark the first service-facing contract as implemented
6. the next slice remains a narrow service boundary, not a syscall/process
   redesign

## Recommended Order

1. tighten request behavior for healthy vs faulted groups
2. expand diagnostics only where needed by the service boundary
3. add HAL/kernel acceptance coverage for request/fault interaction
4. update RFC-00B3 and status/docs
5. only then add narrow service-facing actions
