# Axion Kernel Execution Plan

Current working release label: `Axion v0.1.0-alpha`

This note now captures the next narrow kernel slice after the currently
implemented stable supervisor/process-group service boundary from
[RFC-00B3](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).

It remains intentionally small. It exists so the next kernel work follows an
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

## Completed Groundwork

The previous service-runtime convergence slice is complete:

- the service contract stayed narrower than a syscall or process ABI
- lifecycle control remained limited to deterministic register, unregister,
  suspend, resume, and health transitions
- lifecycle diagnostics now align across service detail, supervisor inventory,
  supervisor status, supervisor recovery, runtime, fault, audit, and device
  views
- HAL/kernel coverage proves that aligned lifecycle surface end-to-end

The first process-memory ownership slice is also complete:

- each process group now has an explicit kernel-owned address-space object
- runtime, process-group, supervisor, and service diagnostics expose
  address-space ownership and mapped-page counts
- page-table ownership now attaches to a stable runtime object before pager
  work begins

The first pager-groundwork slice is now also complete:

- delivered `Unmapped` faults now mark the owning address space as
  pager-needed
- delivered `PermissionDenied` and `InvalidTva` faults remain explicit policy
  failures instead of being conflated with pager work
- runtime, process-group, service, supervisor, and fault diagnostics now expose
  pager-needed address-space state and fault counts without widening the
  service contract

The second pager-groundwork slice is now also complete:

- pager-needed address spaces now enter a deterministic internal handoff queue
- the kernel loop now dispatches one internal pager handoff at a time without
  widening into a pager ABI
- stable diagnostics now distinguish pager-needed state from handoff-pending
  and handoff-dispatched state

The third pager-groundwork slice is now also complete:

- once the missing mapping appears, the kernel loop now resolves one
  handed-off pager-needed address space at a time
- pager-needed state now clears deterministically without widening into a pager
  ABI
- stable diagnostics now distinguish pager-needed, handoff-dispatched, and
  resolved state

The fourth pager-groundwork slice is now also complete:

- the kernel now has a real internal pager worker with a FIFO inbox and one
  active work item
- dispatched pager handoffs now flow into that worker rather than existing
  only as summary counters
- repeated pager-needed cycles on one address space now remain deterministic
  through handoff and resolution

The fifth pager-groundwork slice is now also complete:

- repeated unresolved faults on a worker-owned address space now coalesce
  instead of creating duplicate pager work items
- stable diagnostics now expose worker-owned state and coalesced pager-fault
  counts across runtime, process-group, service, supervisor, and fault views

## Next Sequence

### 1. Keep the service contract stable

Do not widen the existing service surface further unless a concrete runtime
need appears.

### 2. Keep pager state internal while preparing richer pager work

The next real kernel work is now:

- pager integration
- richer kernel-owned pager work after the first worker model
- explicit transition handling for backlog, prioritization, or multiple queued
  address spaces
- stable diagnostics proving pager worker behavior remains deterministic under
  deeper load

### 3. Keep the pager surface internal first

Pager work should first land as kernel-owned runtime state and fault policy.
Do not introduce a public pager ABI or syscall surface in the first slice.

## Non-Goals For This Slice

Do not add:

- syscalls
- userspace/kernel privilege modes
- capabilities
- public pager RPC or syscall interfaces
- general lazy-allocation policy
- full virtual memory object semantics
- shell logic inside the kernel
- general service graph orchestration
- userland process ABI

## Acceptance Criteria

The current pager-groundwork slice is complete when:

1. explicit kernel-owned runtime state exists for address-space pager handling
2. HAL/kernel tests prove deterministic pager-needed diagnostics without
   widening the public contract
3. MMU faults remain clearly separated between unrecoverable policy failures
   and pager-eligible misses
4. the public service contract remains narrower than a syscall, capability, or
   pager ABI surface

That acceptance bar is now met. The next slice should preserve that state while
expanding pager-worker behavior carefully before any external pager interface
exists.

## Recommended Order

1. preserve the current service-runtime contract without widening it casually
2. preserve the new pager-needed runtime state on address spaces
3. add richer internal pager-worker behavior without widening the contract
4. expose only stable diagnostics for that worker state first
5. only then evaluate pager-facing ABI shape or syscall/capability design
