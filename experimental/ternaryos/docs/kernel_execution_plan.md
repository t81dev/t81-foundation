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

Not yet implemented:

- a service-facing kernel request contract
- stable request/result types above the supervisor boundary
- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. First service-facing runtime contract

Define the smallest kernel request surface above the current supervisor
boundary.

Initial scope should be read-mostly and deterministic:

- runtime status query
- fault state query
- process-group status query
- device-arbitration summary

### 2. Request ownership and fault interaction rules

Specify how requests behave when groups are healthy versus faulted.

Required outcomes:

- requests remain deterministic
- faulted groups are classified explicitly
- supervisor acknowledgement remains the recovery gate
- no hidden retries or automatic restart behavior

### 3. Service-facing diagnostics

Expose enough stable diagnostics for a service layer to consume kernel state
without reading kernel internals directly.

Examples:

- runtime counters
- fault backlog summary
- process-group state summary
- supervisor pending-group summary
- device arbitration summary

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

## Recommended Order

1. define request/result structs in `kernel_main.hpp`
2. implement deterministic dispatch in `kernel_main.cpp`
3. add HAL/kernel acceptance coverage
4. update RFC-00B3 and status/docs
5. only then expand the contract surface
