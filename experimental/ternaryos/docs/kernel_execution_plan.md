# Axion Kernel Execution Plan

Current working release label: `Axion v0.1.0-alpha`

This note now captures the next narrow kernel slice after the currently
implemented stable supervisor/process-group service boundary from
[RFC-00B3](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).

It remains intentionally small. It exists so the next kernel work follows an
explicit sequence instead of growing organically.

The current post-kernel packaging phase is now complete: the staged ARM/QEMU
lane validates explicit boot-progress state, and the `x86_64` handoff bundle
now carries aligned contract files, helper scripts, recovered-artifact
templates, positive/negative local fixtures, and packaged smoke-checks. The
next milestone is no longer more local packaging work; it is actual external
`x86_64` VirtualBox host execution and evidence return against that contract.

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

The sixth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain pending-handoff and worker-inbox
  high-water marks for deeper pager backlog/load visibility
- the internal pager worker now records deterministic activation counts
- HAL/kernel coverage now proves FIFO backlog handling across two queued
  address spaces without widening the pager surface

The seventh pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain pager-worker stall cycles when an
  active unresolved item prevents immediate progress
- backlog-blocked cycles now distinguish the narrower case where FIFO ordering
  is explicitly holding queued work behind that stalled active item
- HAL/kernel coverage now proves those stall/backlog-blocked counters advance
  deterministically under FIFO backlog pressure

The eighth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now also retain ready-backlog cycles for the
  narrower case where queued work is already mappable behind a stalled active
  item
- diagnostics now preserve the last ready queued address space observed behind
  that active stall
- HAL/kernel coverage now proves ready-behind-active FIFO pressure is tracked
  deterministically without changing scheduling policy yet

The ninth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now expose current ready-backlog depth behind a
  stalled active item
- diagnostics also retain the high-water mark for that ready-backlog depth
- HAL/kernel coverage now proves ready-backlog depth rises and drains
  deterministically under the existing FIFO worker model

The tenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the last stalled active address
  space alongside the last ready queued address space observed behind it
- postmortem pager-worker summaries can now explain both sides of the blocked
  FIFO relationship after the queue has already drained
- HAL/kernel coverage now proves those retained blocker/blocked identities stay
  deterministic after backlog drain

The eleventh pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the ordinal of the latest pager
  worker stall alongside the retained blocker/blocked identities
- postmortem pager-worker summaries can now correlate the blocker/blocked pair
  to a specific deterministic stall event after backlog drain
- HAL/kernel coverage now proves that retained stall ordinal stays stable after
  the worker goes idle

The twelfth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the stall ordinal associated with
  the last ready queued address space observed behind a stalled active item
- postmortem pager-worker summaries can now correlate the retained blocked
  address directly to the specific deterministic stall event that exposed it
- HAL/kernel coverage now proves that retained blocked-address stall ordinal
  stays stable after backlog drain

The thirteenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the ready-backlog depth observed at
  the same deterministic stall event as the retained blocked queued address
- postmortem pager-worker summaries can now explain not only which queued
  address was blocked, but how much ready backlog existed with it
- HAL/kernel coverage now proves that retained blocked-side backlog depth stays
  stable after backlog drain

The fourteenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the last activated address space and
  its activation ordinal after the worker goes idle
- postmortem pager-worker summaries can now correlate the final worker
  activation with the later blocker/blocked relationship summaries
- HAL/kernel coverage now proves that retained activation identity and ordinal
  stay stable after backlog drain

The fifteenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the last completed pager-worker
  address space and its resolution ordinal after the worker goes idle
- postmortem pager-worker summaries can now correlate completion history
  directly with the retained activation and stall/backlog relationship fields
- HAL/kernel coverage now proves that retained completion identity and ordinal
  advance deterministically through FIFO backlog drain

The sixteenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now retain the last received pager-worker
  address space and its handoff ordinal after the inbox drains
- postmortem pager-worker summaries can now correlate intake history directly
  with the retained activation, stall/backlog, and completion fields
- HAL/kernel coverage now proves that retained receipt identity and ordinal
  advance deterministically through FIFO backlog dispatch and drain

The seventeenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now expose the active pager-worker handoff
  ordinal while work is in flight and clear it again when the worker goes idle
- live worker summaries can now correlate the currently active item directly
  with the retained intake, activation, and completion provenance
- HAL/kernel coverage now proves that active handoff ordinal tracks FIFO work
  in flight without changing scheduling policy

The eighteenth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now expose the next queued pager-worker
  address space and handoff ordinal at the head of the FIFO inbox
- live worker summaries can now describe both the active item and the next
  queued item without changing worker scheduling policy
- HAL/kernel coverage now proves that queued-head provenance advances and
  drains deterministically through FIFO backlog execution

The nineteenth pager-groundwork slice is now also complete:

- when the worker is idle and the FIFO head is still unresolved, the kernel
  now selects the earliest already-ready queued item instead of activating the
  blocked head first
- runtime and fault diagnostics now expose ready-bypass activation counts plus
  the blocked head and promoted ready address space for the latest bypass
- HAL/kernel coverage now proves that this first deterministic ready-bypass
  rule advances progress without widening the pager surface

The twentieth pager-groundwork slice is now also complete:

- the same blocked FIFO head can now be bypassed at most once while it remains
  unresolved, after which later ready items are deferred behind it
- runtime and fault diagnostics now expose ready-bypass deferral counts plus
  the latest blocked head and deferred ready address space for that bounded
  bypass rule
- HAL/kernel coverage now proves that repeated ready items do not starve one
  unresolved blocked head under the internal pager-worker policy

The twenty-first pager-groundwork slice is now also complete:

- once that bounded ready-bypass cap has fired, the worker now remains parked
  instead of activating the blocked unresolved head just to record another
  deterministic stall
- the bounded deferral path now preserves the blocked head at the queue front
  until it becomes ready, while later ready items remain queued behind it
- HAL/kernel coverage now proves that capped deferral parks the worker,
  avoids redundant stall cycles, and resumes progress once the blocked head is
  finally mappable

The twenty-second pager-groundwork slice is now also complete:

- parked capped-deferral state now accumulates deterministic parked-worker
  cycles while the blocked head remains unresolved
- runtime and fault diagnostics now retain the latest parked blocked/ready
  pair plus the ready-item count observed during that parked cycle
- HAL/kernel coverage now proves repeated parked cycles accumulate cleanly
  before the blocked head becomes mappable and backlog drain resumes

The twenty-third pager-groundwork slice is now also complete:

- ready-bypass deferrals now count parked episodes for one blocked head rather
  than incrementing every parked worker cycle
- parked-worker cycles continue to accumulate independently so the kernel can
  separate "how many times parking started" from "how long the worker waited"
- HAL/kernel coverage now proves repeated parked idle cycles preserve one
  deferral record while parked-cycle duration continues to advance

The twenty-fourth pager-groundwork slice is now also complete:

- runtime and fault diagnostics now expose live parked-ready backlog count and
  a retained high-water mark distinct from ready-behind-active backlog state
- parked-worker summaries can now distinguish "worker idle with blocked head"
  from "worker idle with ready work trapped behind a parked head"
- HAL/kernel coverage now proves parked-ready backlog accounting advances while
  the worker stays parked and clears once the blocked head drains

The twenty-fifth pager-groundwork slice is now also complete:

- parked capped-deferral state now records explicit parked-resumption
  transitions once the blocked head finally becomes ready again
- runtime and fault diagnostics now retain parked-resumption counts plus the
  latest resumed blocked-head identity and resumption ordinal
- HAL/kernel coverage now proves the worker stays at zero resumptions while the
  head remains parked, then records one deterministic resumption when that head
  drains

The twenty-sixth pager-groundwork slice is now also complete:

- parked-resumption diagnostics now also retain how much ready backlog was
  still queued behind the resumed blocked head at the moment the worker resumed
- runtime and fault diagnostics can now distinguish "resumed from parked state"
  from "resumed with trailing ready work still waiting behind that head"
- HAL/kernel coverage now proves the parked head resumes first while one ready
  item remains queued behind it

The twenty-seventh pager-groundwork slice is now also complete:

- parked-resumption diagnostics now also retain the latest still-ready queued
  address space and handoff ordinal observed behind the resumed blocked head
- runtime and fault diagnostics can now identify not only how much trailing
  ready work remained at resumption time, but exactly which queued handoff it was
- HAL/kernel coverage now proves the resumed head snapshots the third queued
  handoff as trailing ready work at parked resumption time

The twenty-eighth pager-groundwork slice is now also complete:

- parked-resumption diagnostics now also retain the resumed blocked head's own
  handoff ordinal alongside its resumption ordinal
- runtime and fault diagnostics can now identify both sides of the parked
  resumption snapshot: the resumed head and the trailing ready handoff behind it
- HAL/kernel coverage now proves the resumed head retains its original first
  handoff ordinal at parked resumption time

The twenty-ninth pager-groundwork slice is now also complete:

- parked-resumption flow now also retains when that resumed blocked head
  actually resolves, including its address space, handoff ordinal, and
  resolution ordinal
- runtime and fault diagnostics can now distinguish "resumed from parked state"
  from "completed after parked resumption"
- HAL/kernel coverage now proves the blocked head records one deterministic
  parked-head resolution when it drains after resumption

The thirtieth pager-groundwork slice is now also complete:

- parked-head resolution diagnostics now also retain the queued work that
  remained behind the resolved head at the instant it drained
- runtime and fault diagnostics can now distinguish "parked head resolved"
  from "parked head resolved while queued work still remained behind it"
- HAL/kernel coverage now proves the parked head resolves with one queued item
  still remaining behind it

The thirty-first pager-groundwork slice is now also complete:

- the first activation that follows a parked-head resolution is now retained
  explicitly as a deterministic parked-resolution follow-on transition
- runtime and fault diagnostics can now link "parked head drained with queued
  work behind it" to "that queued work activated next"
- HAL/kernel coverage now proves the trailing queued item activates as the
  first follow-on step after the parked head resolves

The thirty-second pager-groundwork slice is now also complete:

- the queued successor activated after a parked-head resolution is now also
  retained through its own deterministic completion
- runtime and fault diagnostics can now link "parked head resolved" to
  "queued successor activated" to "queued successor resolved"
- HAL/kernel coverage now proves the queued successor resolves as the final
  deterministic step after the parked path drains

## Next Sequence

### 1. Keep the service contract stable

Do not widen the existing service surface further unless a concrete runtime
need appears.

### 2. Close the current boot-ready slice

This kernel slice is now complete. The next real kernel work is now:

- external boot-lane validation on top of the completed internal pager policy
- later pager integration beyond the current kernel-owned boot-critical path
- eventual pager-facing ABI shape only after external boot evidence exists

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

That acceptance bar is now met, and the current boot-ready slice is now
closed. The next slice should preserve that state while moving outward to
external boot evidence before any external pager interface exists.

The first interrupt-convergence slice under RFC-00B5 can proceed in parallel
without widening the service contract: kernel-owned interrupt event intake,
deterministic loop delivery, and stable runtime/fault/audit diagnostics.
The next narrow interrupt slice is queue observability: retained intake
provenance, pending-queue high-water marks, and stable visibility into the
next queued interrupt without adding controller policy.
After that, the next narrow slice is deterministic source accounting for the
existing interrupt classes, still without priority, masking, or controller
ownership policy.
The next useful queue-facing slice after that is pending source composition:
how many queued interrupts exist per current source class, still with FIFO
ordering and no priority policy.
The next queue-facing slice after that is explicit queue-bound visibility:
stable head/tail pending interrupt reporting, still without changing delivery
order or adding controller policy.
The next provenance slice after that is stable interrupt-audit correlation:
retain the latest interrupt-delivery audit sequence directly in the interrupt
summary surfaces instead of forcing callers to infer it from the recent log.
The next alignment slice after that is live interrupt queue state in
`AuditSummary`: retain pending interrupt counts, pending source composition,
and FIFO head/tail visibility there too so the stable summaries stay aligned.
The next accounting slice after that is total recorded-interrupt alignment in
`AuditSummary`: expose the same aggregate recorded count already carried by the
runtime and fault summaries.

## Recommended Order

1. preserve the current service-runtime contract without widening it casually
2. preserve the new pager-needed runtime state on address spaces
3. preserve the new terminal-failure rule for unresolved parked heads
4. preserve the new internal boot-critical pager-resolution policy without
   widening the contract
5. preserve the new explicit boot-progress/fail reporting for that internal
   policy
6. preserve the now-closed boot-ready slice and its status/RFC framing
7. move next to external boot-lane validation
8. only then evaluate pager-facing ABI shape or syscall/capability design
