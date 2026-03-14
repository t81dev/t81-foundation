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

The newest execution-control slice now goes one step further than named entry
descriptors: process groups can register CanonRef-backed executable objects and
spawn from them through the typed, wire, and hosted C ABI layers. Services can
also bind to those registered executable objects by CanonRef during
`RegisterService`, and service register/query/spawn paths now preserve that
backing executable identity. Registration now validates a canonical
`CanonExec` block against the supplied `CanonRef`, so this is no longer only a
free-form descriptor registry. Executable objects can now also be registered
from mapped caller memory through `RegisterExecutableObjectFromTva`. The
published-object repository is now also bindable to an external block-device
image, so a fresh kernel can reload published executables by `CanonRef`. The
kernel can now also bind a persistent CanonFS root and resolve executable
objects by `CanonRef` through that driver. The
next execution milestone is fetching and loading those executable objects from
real CanonFS-backed object storage instead of only from registration-time
input, caller memory, or the current bindable published-object repository.

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

- real hardware trap/syscall entry
- executable loading
- CanonFS-backed executable object acquisition beyond the current
  registration-time, caller-memory, and `CanonStore`-backed published-repository
  `CanonExec` validation paths

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

The interrupt summary-convergence slice under RFC-00B5 is now complete without
widening the service contract:

- kernel-owned interrupt event intake exists
- deterministic loop delivery exists
- stable runtime, fault, and audit summaries now expose queue state,
  per-source accounting, and latest interrupt-audit metadata
- `KernelInterruptRecord` now retains both intake and delivery audit
  provenance directly
- HAL/kernel coverage proves that interrupt summary surfaces stay
  self-describing without log reconstruction

The first real interrupt-policy slice under RFC-00B5 is now complete:

- Timer interrupt delivery now calls `axion_kernel_tick()` unconditionally,
  turning the cooperative tick loop into a timer-driven preemptive kernel
- Storage, Network, and Keyboard interrupts are accounted via
  `device_interrupts_handled` (policy stub; no thread-wake behavior yet)
- Unknown interrupts are recorded and discarded
- New counters: `timer_interrupts_handled`, `timer_preempts`,
  `device_interrupts_handled`; new retained state: `last_timer_preempt_cycle`,
  `last_timer_preempt_sequence`
- `KernelRuntimeStatusView` exposes all new fields
- `[AC-22g]` (35 assertions) proves timer preemption, storage non-preemption,
  and runtime status view coverage

**Slice 1A — Real Executable Load is now complete:**

- `load_canon_exec_sections()` (`kernel_loader.cpp` / `kernel_loader.hpp`)
  allocates a ternary page at the VPN containing the entry PC, maps it with
  read+execute permissions via `mmu_map()`, and copies CanonExec image block
  bytes into `physical_page_storage`.
- `SpawnThreadFromExecutableObject` now calls the section loader before
  spawning; the spawned thread's PC is the real mapped entry TVA.
- Re-spawn of the same executable reuses the already-mapped page (no
  double-allocation).
- `[AC-22h]` (24 assertions): page unmapped before spawn → mapped after →
  physical storage holds block bytes → spawned PC matches → re-spawn reuses
  existing page.

**Slice 4 — Syscall trap wiring is now complete:**

- New `kernel_trap_shim.hpp` / `kernel_trap_shim.cpp` model the hosted
  AArch64 `svc` exception entry path.
- `SvcTrapFrame {request_tva, response_tva, svc_imm}` carries the context
  a real exception handler would save from `x0`, `x1`, and the SVC immediate.
- `axion_kernel_handle_svc_trap()` enforces svc_imm == 0 (single Axion
  entry convention per RFC-00B6 §5.2) and delegates to
  `axion_kernel_call_wire_tva()` — no new dispatch logic is invented.
- `syscall_trap_dispatches` counter added to `Counters` and exposed through
  `KernelRuntimeStatusView`.
- `[AC-22k]` (28 assertions): svc_imm=0 dispatch round-trip, response wire
  block decoded, svc_imm=7 rejection, counter monotonicity, status view.

The next milestone on the critical path toward a bootable kernel:

**Slice 5 — User-mode address space isolation is now complete:**

- `kKernelSpaceVpnBase = 3^19` added to `mmu/tva.hpp` with `tva_in_user_space()`
  and `tva_in_kernel_space()` predicates.
- `AddressSpaceState` gains `kernel_owned` flag; bootstrap sets it on
  `kKernelAddressSpace`.
- `axion_kernel_validate_address_space_span()` rejects user-owned AS for any
  TVA with VPN >= `kKernelSpaceVpnBase`.
- `kernel_space_rejections` counter incremented in write and wire-TVA paths;
  exposed through `KernelRuntimeStatusView`.
- `[AC-22l]` (28 assertions): flags, TVA predicates, validate/write rejection,
  user-space TVA accepted, wire-TVA error response, runtime view.

The next milestone on the critical path toward a bootable kernel:

**Slice 6 — QEMU AArch64 EDK2 guest image bootstrap is now complete:**

- `qemu_slice6_startup_snapshot.cpp` bootstraps the kernel with the QEMU
  virt memory layout, exercises Slices 4+5, queries the runtime status view,
  and generates `qemu_slice6_startup.h` at build time.
- `qemu_armv8_efi_stub.c` is a freestanding AArch64 PE/COFF EFI application
  compiled with `-ffreestanding -nostdlib` and linked with LLD.  At UEFI
  runtime it emits a serial banner, writes `TERNOS/efi-slice6-ran.txt` and
  `TERNOS/slice6-boot-report.txt` to the FAT32 boot volume, then calls the
  kernel shim.
- `build_qemu_slice6_artifact.sh` produces a 64 MB GUID-partitioned FAT32
  raw image (`qemu_slice6_guest.img`) suitable for QEMU virtio block without
  a VDI conversion step.
- `run_qemu_armv8_slice6_probe.sh` boots the image under QEMU `virt` +
  EDK2 AArch64 firmware, mounts the FAT32 partition, and validates the
  boot report via `validate_qemu_armv8_slice6_reports.sh`.
- CMake target `t81_ternaryos_qemu_armv8_slice6_probe` covers the full
  snapshot → compile → link → image → boot → validate pipeline.
- Confirmed in QEMU: `slice4_svc_trap_wiring=complete`,
  `slice5_user_isolation=complete`, `kernel_space_rejections=1`,
  `efi_marker_seen=1`, `boot_banner_seen=1`, all validation checks pass.

The next milestone toward the bootable kernel:

**Slice 9 — first public pager service ABI is now complete:**

- `KernelCapabilityKind::PagerService` is a new capability kind that authorises a service
  thread to supply page mappings on behalf of faulted address spaces (RFC-00B7 §3.2).
  Not seeded by default — must be explicitly granted via `GrantCapability`.
- `KernelCallKind::RequestPageMapping` is the new ABI call.  The caller supplies an
  `address_space_id`; the kernel validates PagerService capability, verifies `pager_needed`,
  looks up `last_pager_fault`, calls `mmu_map()` at `last_pager_fault->tva` with permissions
  derived from `access_mode`, and returns `pager_mapping_supplied = true`.
- On the next `axion_kernel_step()` tick the pager worker detects the mapping via
  `is_pager_work_item_ready()` and clears `pager_needed` through the existing
  `resolve_completed_pager_work()` path — no new pager-worker code required.
- New `KernelCallRejection` variants: `AddressSpaceNotPagerNeeded`, `MissingPagerFault`.
- New counter: `Counters::pager_service_mappings` (exposed via `KernelRuntimeStatusView`).
- `[AC-22o]` (34 assertions): victim fault → pager stall → grant PagerService → rejection
  paths (no AS, bad AS, not pager_needed) → success → TVA mapped → worker resolves →
  pager_needed cleared → second call rejected → runtime view counter.

**Slice 8 — AArch64 exception vector table is now complete:**

- `aarch64_exception_vectors.S` provides the full 2 KiB-aligned VBAR_EL1 table
  (16 × 128-byte slots per ARM DDI 0487).  The only functional slot is offset
  0x400 (Lower EL, AArch64, Synchronous — the SVC path); all others branch to
  `axion_vec_unhandled` (deliberate infinite loop).
- `axion_svc_entry` saves all 31 GPRs plus `sp_el0`/`elr_el1`/`spsr_el1`/
  `esr_el1` into an `AArch64TrapFrame` (280 bytes) on the EL1 stack, calls
  `axion_kernel_handle_svc_trap_aarch64()`, restores the frame, and executes
  `eret`.  Layout verified by compile-time `static_assert` in
  `aarch64_trap_entry.hpp`.
- `axion_kernel_svc_frame_from_aarch64()` bridges the raw AArch64 frame to the
  kernel's `SvcTrapFrame` (x0→request_tva, x1→response_tva, ESR[15:0]→svc_imm).
- `axion_kernel_install_exception_vectors()` writes VBAR_EL1 on bare-metal
  AArch64; documented no-op on macOS/Apple-Silicon host builds.
- Assembly verified to compile for `aarch64-pc-windows-msvc` via CMake target
  `t81_ternaryos_aarch64_exception_vectors_obj`.
- `[AC-22n]` (24 assertions): struct layout, ESR helpers, bridge, null-safety,
  install callable, SVC #7 rejection via bridge.

**Slice 7 — CanonFS-backed executable object acquisition is now complete:**

- `SpawnThreadFromExecutableObject` now resolves a `CanonRef` directly from the
  bound `published_executable_canonfs` driver when it is absent from the
  in-memory registry — the first CanonFS-first spawn path (RFC-00B2 §3.1).
- On a registry miss the kernel calls `load_published_executable_block()`,
  decodes the returned `CanonExec` block, builds a temporary `ExecutableRecord`,
  and proceeds through the existing section-loader and thread-spawn path.
- `counters.canonfs_fetch_spawns` and `KernelRuntimeStatusView.canonfs_fetch_spawns`
  track spawns that took the CanonFS fetch path.
- A `CanonRef` absent from both registry and CanonFS still returns
  `MissingExecutableRegistration` — no silent fallback.
- `[AC-22m]` (27 assertions) proves: in-memory driver write → spawn without
  registration → Ok + page mapped + correct PC/label → re-spawn reuses page →
  unknown ref fails → runtime view exposes counter.

## Recommended Order

1. preserve the current service-runtime contract without widening it casually
2. preserve the new pager-needed runtime state on address spaces
3. preserve the new terminal-failure rule for unresolved parked heads
4. preserve the new internal boot-critical pager-resolution policy without
   widening the contract
5. preserve the new explicit boot-progress/fail reporting for that internal
   policy
6. preserve the now-closed boot-ready slice and its status/RFC framing
7. preserve the now-complete interrupt summary-convergence surface under
   RFC-00B5
8. preserve the now-complete first interrupt-policy slice: timer-driven
   preemption is wired; device-wake behavior follows after blocking primitives
   exist
9. **[DONE]** Slice 1A — real executable section load into mapped address space
10. **[DONE]** Slice 2 — blocking IPC: thread sleep/wake on inbox
11. **[DONE]** Slice 3 — device-wake: Storage/Network interrupt wakes device-waiting threads
12. **[DONE]** Slice 4 — syscall trap wiring: ARM `svc` exception vector → typed ABI boundary
13. **[DONE]** Slice 5 — user-mode address space isolation (kernel vs. user TVA split)
14. **[DONE]** Slice 6 — QEMU AArch64 EDK2 guest image bootstrap
15. **[DONE]** Slice 7 — CanonFS-backed executable object acquisition
16. **[DONE]** Slice 8 — AArch64 exception vector table (VBAR_EL1 entry, frame layout, bridge)
17. **[DONE]** Slice 9 — first public pager service ABI (RequestPageMapping + PagerService capability)
