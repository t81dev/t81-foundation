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
- kernel-owned service registration/liveness state for the first service layer
- supervisor-owned service inventory
- service-facing request routing above raw process-group ids
- service blocked/faulted state at the service layer
- first narrow service-facing service action: deterministic service registration
- richer stable service diagnostics for service detail and supervisor inventory
- second narrow service-facing service action: deterministic service unregister

Not yet implemented:

- capability or syscall semantics
- process address-space ownership
- pager integration

## Next Sequence

### 1. Keep the service contract stable

Do not widen the surface into syscalls, capabilities, or a process ABI yet.

### 2. Preserve deterministic service lifecycle behavior

The current service layer now covers:

- registered service summaries
- supervisor-owned service inventory
- blocked/faulted service visibility
- service request/rejection counters
- deterministic service registration
- deterministic service unregister
- deterministic service suspend / resume

### 3. Only add new actions if a stable service runtime truly needs them

Any further action should remain narrow and lifecycle-oriented:

- explicit service health transition

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

The current service-runtime slice is complete when:

1. stable service-facing diagnostics exist for service, supervisor, fault,
   audit, and device ownership state
2. HAL/kernel tests prove service registration, blocked behavior, and
   unregister lifecycle behavior
3. the service contract remains narrower than a syscall or process ABI
4. RFC-00B3 can shift to supervisor/service runtime convergence as the next step

## Recommended Order

1. keep HAL/kernel acceptance coverage ahead of any new surface growth
2. keep service registration + unregister semantics stable
3. add a further lifecycle action only if the service runtime truly needs it
4. update RFC-00B3 and status/docs
