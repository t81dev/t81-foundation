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
- stable service-facing audit summaries and per-device ownership detail views
- first service-facing runtime action: supervisor fault-group acknowledgement
- supervisor-facing recovery/report flows through the current contract
- second narrow service-facing action: deterministic device claim/release
  requests through the same contract
- explicit request/action rejection semantics and stable diagnostic views across
  the current contract

Not yet implemented:

- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. Contract stabilization

Now that the second narrow action and stable diagnostics exist, keep the
contract stable before widening it further:

- keep request/result types stable
- keep request outcomes deterministic
- avoid leaking kernel internals directly into services
- preserve audit-summary and per-device ownership detail semantics

### 2. Additional narrow actions only if needed

Only after the current contract is stable should another narrow action be
considered.

Likely candidates:

- supervisor-visible device arbitration summaries tied to action results
- explicit action rejection reasons if the current `InvalidRequest` bucket
  becomes too coarse

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
4. stable service-facing diagnostics exist for group, supervisor, fault, device,
   audit, and per-device ownership state
5. HAL/kernel tests prove the request path and its interaction with fault state
6. one narrow service-facing action exists above the contract
7. supervisor-facing recovery/report flows are exposed through the same contract
8. a second narrow action is implemented without widening the contract shape
9. RFC-00B3 can shift to contract stabilization as the next step

## Recommended Order

1. stabilize the existing request/action/result shapes
2. keep HAL/kernel acceptance coverage ahead of any new action growth
3. update RFC-00B3 and status/docs
4. only then widen the service-facing surface again
