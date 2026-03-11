# Axion Kernel Execution Plan

Current working release label: `Axion v0.1.0-alpha`

This note captured the next narrow kernel slice after the currently
implemented stable supervisor/process-group service boundary from
[RFC-00B3](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).

It was intentionally small. It exists so the next kernel work follows an
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
- kernel-owned address-space objects bound to process groups
- supervisor-owned service inventory
- service-facing request routing above raw process-group ids
- service blocked/faulted state at the service layer
- first narrow service-facing service action: deterministic service registration
- richer stable service diagnostics for service detail and supervisor inventory
- second narrow service-facing service action: deterministic service unregister

Not yet implemented:

- capability or syscall semantics
- pager integration

## Slice Result

The service-runtime convergence slice is now complete for the current
contract surface:

- the service contract stayed narrower than a syscall or process ABI
- lifecycle control remained limited to deterministic register, unregister,
  suspend, resume, and health transitions
- lifecycle diagnostics now align across service detail, supervisor inventory,
  supervisor status, supervisor recovery, runtime, fault, audit, and device
  views
- HAL/kernel coverage now proves that aligned lifecycle surface end-to-end

The first process-memory ownership slice is now also complete:

- each process group now has an explicit kernel-owned address-space object
- runtime, process-group, supervisor, and service diagnostics now expose
  address-space ownership and mapped-page counts
- page-table ownership can now attach to a stable runtime object before pager
  work begins

## Next Sequence

### 1. Keep the service contract stable

Do not widen the existing service surface further unless a concrete runtime
need appears.

### 2. Shift the next kernel slice below the service layer

The next real kernel work is now:

- pager integration
- the fault-to-pager handoff needed before any syscall or capability design

### 3. Do not add new lifecycle verbs opportunistically

Any further service action should require a concrete runtime need, not just
surface symmetry.

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
4. RFC-00B3 can shift from supervisor/service runtime convergence to
   process-memory ownership and pager work as the next step

## Recommended Order

1. preserve the current service-runtime contract without widening it casually
2. define kernel-owned process memory semantics
3. integrate pager behavior at the runtime boundary
4. only then evaluate syscall or capability design
