# Axion OS Execution Plan

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

All defined kernel slices (1A through 28) and DPE slices (13–25) are now
complete — 3214 ternaryos assertions passing.  Slice 28 adds RFC-00B5 §3.5
unhandled interrupt governance: `register_unhandled_interrupt_callback()` HAL
API, `axion_kernel_record_unhandled_interrupt()` kernel function,
`interrupts_unhandled` counter, and `UnhandledInterruptDropped` audit event —
RFC-00B5 status advanced to `integrated`.  Slice 27 adds the RFC-00B5 §3.7
Interrupt Policy Gate: per-source rate limiting and quarantine evaluated before
every interrupt dispatch, with full audit trail, runtime status view counters,
and three new kernel ABI calls (SetInterruptPolicy, ClearInterruptQuarantine,
QueryInterruptPolicy).  The device-wake surface is now fully closed: Storage,
Network, and Keyboard interrupts all wake parked threads via `WaitForDevice`.
CanonFS-backed executable object acquisition (Slice 7), EL0→EL1 SVC roundtrip
(Slice 12), pager service ABI (Slices 9–11), and the full DPE epoch execution
pipeline are all in place.

The QEMU x86_64 EFI boot lane has been validated: BOOTX64.EFI executed under
QEMU TCG, all 5 contract files verified, `hal_main_result=0`,
`kernel_boot_ready_slice=complete`, `phase=5`.  Evidence record:
`docs/records/audits/TERNARYOS_X86_64_BOOT_EVIDENCE_2026-03-16.md`.

The next open external milestone is actual `x86_64` VirtualBox host execution
and evidence return against the packaged boot contract (tracked in the Boot
Milestones section below).

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

**Slice 10 — WaitForPagerHandoff blocking call is now complete:**

- `KernelCallKind::WaitForPagerHandoff` parks a `PagerService`-capable thread via
  `scheduler.sleep()` and registers its TID in
  `KernelRuntimeState::pager_handoff_waiting_tids` (RFC-00B7 §3.3).
- `dispatch_pending_pager_handoff()` in `kernel_pager.cpp` wakes all waiting threads when
  a handoff is dispatched: delivers a synthetic IPC message with
  `tag = "pager-handoff-wake"` and `payload = address_space_id`, calls `scheduler.wake()`,
  clears the set, and increments `pager_handoff_wakes`.
- The woken service thread reads the IPC message to learn which AS needs a mapping, then
  calls `RequestPageMapping` directly — no extra status query required.
- New fields: `Counters::pager_handoff_wakes`, `pager_handoff_waiting_thread_count` in
  `KernelRuntimeStatusView`.
- `[AC-22p]` (32 assertions): capability rejection → park → runtime view 1 waiting thread
  → fault triggers handoff dispatch → svc woken → IPC carries AS id → RequestPageMapping
  → pager_needed cleared → runtime view final state.

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
18. **[DONE]** Slice 10 — WaitForPagerHandoff blocking call (park/wake + IPC notification)
19. **[DONE]** Slice 11 — ResumePageFaultedThread (complete fault→handoff→service→resume lifecycle)
20. **[DONE]** Slice 26 — Keyboard WaitForDevice: complete device-wake trinity (Storage ✅ Network ✅ Keyboard ✅)
21. **[DONE]** Slice 27 — Interrupt Policy Gate: per-source rate limiting + quarantine + 3 ABI calls (RFC-00B5 §3.7)
22. **[DONE]** Slice 28 — Unhandled IRQ Governance: `register_unhandled_interrupt_callback()` + `axion_kernel_record_unhandled_interrupt()` + `UnhandledInterruptDropped` audit event + `interrupts_unhandled` counter; RFC-00B5 → `integrated` (RFC-00B5 §3.5 / [AC-22x])

**RFC-00B7 (Pager Service ABI) is now written and accepted.**  All three pager
service ABI calls are normatively specified in
`spec/rfcs/RFC-00B7-pager-service-abi.md` with full acceptance criteria
aligned to `[AC-22o]`, `[AC-22p]`, `[AC-22q]`.

**RFC-00B3 (Axion OS Architecture) is now accepted.**  All §8 acceptance
criteria were already met; the two major open questions (capability model,
device arbitration registry) are resolved.

The pager service ABI is complete.  The next critical-path milestone is the
first real EL0→EL1 SVC roundtrip in QEMU — a user-mode thread executing
`svc #0`, the exception vector firing, and `axion_kernel_handle_svc_trap_aarch64()`
routing the call back through the typed ABI.  That validates the
kernel/user boundary end-to-end and unblocks both DPE implementation
(RFC-DPE-0001 trigger) and external boot-lane acceptance validation.

**Slice 13 — DPE Task Graph Primitives [DONE] (commit 129b5e74):**

- RFC-DPE-0002 `experimental/dpe/` module: `t81_dpe` library + 36-assertion test suite.
- `TaskDescriptor`, `EpochGraph`, `TaskId` (CanonHash81 content-addressed), `DeltaBuffer`,
  `EpochAcceptor` — full data model and canonical serialisation.
- Epoch acceptance: Kahn's cycle detection (`[DPE-02-02]`), exclusive-region conflict
  check (`[DPE-02-03]`), duplicate task_seq guard.
- DeltaBuffer: correct page-granular delta accumulation (`[DPE-02-01]`),
  `OutOfRegionWrite` fault with post-fault rejection (`[DPE-02-04]`).
- Key design: "program identity" (dep_task_ids-stripped TaskId) for epoch index —
  makes content-addressed dep references resolvable within an epoch.
- `[DPE-02-05]` (single-task epoch ≡ direct TISC execution) — see Slice 14.
- Suite: 36 passed, 0 failed.

**Slice 15 — DPE Epoch Commit Engine / RFC-DPE-0003 [DONE]:**

- `experimental/dpe/epoch_commit.hpp` + `epoch_commit.cpp`: `commit_epoch()`.
- Canonical commit: TaskId-ascending sort → TVA-ascending delta application →
  last-writer-in-canonical-order for non-exclusive overlapping pages.
- `EpochHash = CanonHash81(epoch_id ∥ input_snapshot ∥ committed_deltas_hash)`.
- Abort path: any faulted `DeltaBuffer` → `Aborted_TaskFault`, `committed_pages`
  empty, `epoch_hash` zero-valued.
- `t81_dpe_epoch_commit_test`: 29 assertions across 6 test functions.
- `[DPE-03-01]`: identical committed state regardless of submission order.
- `[DPE-03-02]`: higher-TaskId task wins for overlapping non-exclusive pages.
- `[DPE-03-03]`: abort on `TaskFault` — canonical state unchanged.
- `[DPE-03-04]`: `EpochHash` identical across two independent DeltaBuffer sets.
- RFC-DPE-0003 advanced to `accepted`; [DPE-03-05..06] deferred (RFC-0030 / kernel wiring).
- Suite: 29 passed, 0 failed.

**Slice 14 — DPE Task Runner / [DPE-02-05] [DONE]:**

- `experimental/dpe/task_runner.hpp` + `task_runner.cpp`: `DpeTaskRunner::run_direct()`.
- Executes a TISC program through a fresh `IVirtualMachine` instance; collects
  `DeltaRecord`s for declared `output_regions` via post-execution memory diff.
- `t81_dpe_task_runner_test`: 17 assertions across 4 test functions.
- `[DPE-02-05]`: single-task epoch (no output regions) final registers match direct
  VM execution exactly — both `LoadImm R5=42` and `LoadImm R1=10, R2=20, Add R3` cases.
- `[DPE-runner-01]`: `DeltaRecord` emitted after `Store` to declared heap-region page.
- `[DPE-runner-02]`: no `DeltaRecord` for declared output-region pages that are not written.
- CMake: `task_runner.cpp` added to `t81_dpe` sources; `t81_vm` added to link deps.
- Suite: 17 passed, 0 failed.

**AI Track — RFC-0032 Phase 5 [DONE]:**

- C-06 (`evidence_collector.cpp`) promoted to `tests/determinism/evidence_collector.cpp`.
  - Removed: `openssl/sha.h` → replaced with FNV-1a 64-bit (no external deps, integer-only).
  - Removed: `std::chrono` timing fields from `ExecutionEvidence` (non-deterministic wall-clock).
  - Removed: `nlohmann/json` → replaced with plain key=value text (`evidence-schema-v1`).
  - `EvidenceCollector` class: `start_collection()`, `record_output()`, `record_trace()` (accepts `AIHookEngine::ai_trace()` directly), `record_metrics()`, `validate_determinism()`, `write_schema()`.
  - `tests/determinism/README.md`: evidence-schema-v1 specification, FNV-1a algorithm, hash field table, example output.
  - Gate tests: 15/15 passing [C06-01..06].
- C-07 (`t81_ai_cli.cpp`) promoted to `tooling/cli/ai/t81_ai_cli.cpp`.
  - Removed: all `std::this_thread::sleep_for()` calls, all `std::chrono` timing, `nlohmann/json`, all hardcoded mock metadata.
  - Wired subsystems: `verify` → `load_model_via_tloadhash()` (Phase 3); `run` → `T81VmBackend::dispatch_embed()` (Phase 4); `quantize` → `quantize_threshold()` + `pack_ternary_to_base81()` (Phase 1); `policy test` → `PolicyEngine::evaluate()`; `benchmark` → dispatches all four AI opcodes via `T81VmBackend`, reports Axion verdict (no timing).
  - Smoke verified: `benchmark` shows ATTN=axion-deny (tier<2), QMATMUL/EMBED/WLOAD=reached-vm; `verify` shows correct allow/deny trace events.
- CMake targets: `t81_determinism_evidence_test` (CTest), `t81_ai_cli` (executable).

**AI Track — RFC-0032 Phase 4 [DONE]:**

- C-02 (`backend_adapter.cpp`) promoted to `core/vm/ai_backend/backend_adapter.cpp` (T81VmBackend).
  - Removed: `LlamaCppBackend`, `OnnxRuntimeBackend`, all `std::this_thread::sleep_for()` calls, all `std::chrono` timing fields, `nlohmann/json` dependency.
  - `include/t81/vm/ai_backend/backend_adapter.hpp`: `T81VmBackend` class + `AiDispatchResult` struct.
  - `core/vm/ai_backend/backend_adapter.cpp`: each dispatch builds a synthetic `[<opcode>, Halt]` TISC program, creates a fresh VM with `AIHookEngine(PolicyEngine(policy_))` attached, loads program, sets register operands, runs to halt, returns result. No external inference runtime.
  - `run_ai_opcode()`: calls `make_interpreter_vm(std::move(hook))`, then `vm->run_to_halt()`; on SecurityFault extracts deny reason from `state().axion_log`.
- `core/vm/ai_backend/backend_adapter.cpp` added to `t81_vm` CMake sources.
- `tests/cpp/backend_adapter_test.cpp`: 9/9 passing [C02-01..05].
  - ATTN with Tier0 → SecurityFault (Axion tier gate fires, not llama.cpp) ✓
  - deny_reason mentions "tier" or "attn_guard" ✓
  - QMATMUL allowed by Axion → DecodeFault (VM reached, not SecurityFault) ✓
  - EMBED allowed by Axion → DecodeFault (VM reached, not SecurityFault) ✓

**AI Track — RFC-0032 Phase 3 [DONE]:**

- C-04 (`axion_hooks.cpp`) promoted to `kernel/axion/ai_hooks.cpp` (AIHookEngine + factory).
  - `include/t81/axion/ai_hooks.hpp`: AIHookEngine, ai_reasons builders (model_load, attn_guard, qmatmul_guard, ai_exec_gate).
  - `kernel/axion/ai_hooks.cpp`: evaluates AI opcodes, emits RFC-0032 §8.2 canonical event strings, enforces tier ≥ 2 guard for ATTN, delegates to inner PolicyEngine.
- C-03 (`model_manager.cpp`) ad hoc hash verification replaced by `TLOADHASH` Axion gate.
  - `include/t81/axion/ai_model_loader.hpp` + `kernel/axion/ai_model_loader.cpp`: `load_model_via_tloadhash()` constructs `SyscallContext{next_opcode=TLoadHash, payload=hash}`, calls engine.evaluate(), emits `model_load success|failure` trace event.
- Both sources added to `t81_axion` CMake target.
- `tests/cpp/axion_ai_hooks_test.cpp`: 17/17 passing [C03-01..04, C04-01..08].
  - Policy-denial test (non-whitelisted hash → Deny) passes — Phase 3 gate criterion met.
  - All §8.2 event strings (model_load, attn_guard, qmatmul_guard, ai_exec_gate) appear in trace.

**AI Track — RFC-0031/RFC-0032 Phase 1 + Phase 2 [DONE]:**

- RFC-0031 status: `draft` → `proposed` (Deterministic AI Execution Contract).
- RFC-0032 Phase 1 (C-01 `ternary_codec`): float-domain logic excised from core.
  - `include/t81/math/quantization/ternary_codec.hpp`: `TritValue`, `pack_ternary_to_base81`, `unpack_base81_to_ternary`, `quantize_threshold`, `dequantize`.
  - `core/math/quantization/ternary_codec.cpp`: integer-only implementation; bit-exact across x86-64 + ARM64.
  - `tools/diagnostics/ternary_codec_metrics.cpp`: MSE/PSNR relocated here; gated behind `T81_BUILD_DIAGNOSTICS`.
  - `tests/determinism/codec/ternary_codec_test.cpp`: 8 acceptance criteria [C01-01..08]; covers pack bit-exactness, round-trip, partial groups, threshold boundaries, dequantize, round-trip invariant, no hidden state.
  - `t81_math_quantization` CMake library; `T81_BUILD_DIAGNOSTICS` option; `t81_determinism_codec_test` CTest target.
- RFC-0032 Phase 2 (C-10 `IMPLEMENTATION_REPORT.md`): promoted to `docs/architecture/ai-opcode-phase1-conformance.md`.
  - ATTN/QMATMUL/EMBED Phase 1 baseline hashes preserved; RFC-0027/RFC-0031 cross-reference notes added.
  - `phase_status` remains `runtime_bound` pending Phase 2 conformance programs (`attn-determinism.t81` etc.).
- `spec/rfcs/index.md`: RFC-0031 status updated to `proposed`.

**Slice 25 — RFC-DPE-0009: Epoch History Ring [DONE]:**

- `EpochHistoryRecord` added to `KernelRuntimeState` (epoch_id, epoch_hash, task_count, level_count, total_delta_records, commit_sequence).
- `EpochRuntimeState`: `kEpochHistoryCapacity = 8`, `std::deque<EpochHistoryRecord> epoch_history{}`.
- `kernel_epoch.cpp`: on commit, push new record; pop_front when over capacity. `total_delta_records` computed by summing `delta_sets[i].records.size()` across all tasks.
- `KernelRuntimeStatusView`: `std::vector<EpochHistoryRecord> epoch_history{}` populated as deque snapshot.
- `t81_ternaryos_epoch_history_test`: 18 assertions across 6 test functions.
- `[DPE-10-01]`: first entry has correct epoch_id, task/level counts, hash, commit_sequence.
- `[DPE-10-02]`: ring grows to N for N ≤ 8; oldest-first ordering.
- `[DPE-10-03]`: 9th commit evicts oldest entry; size stays at 8.
- `[DPE-10-04]`: aborted (task-fault) epoch does not add to ring.
- `[DPE-10-05]`: commit_sequence monotonically increasing across all ring entries.
- `[DPE-10-06]`: KernelRuntimeStatusView snapshot matches state ring exactly.

**Slice 24 — RFC-DPE-0008: Epoch Audit Events [DONE]:**

- `EpochSubmitted`, `EpochCommitted`, `EpochAborted` added to `KernelAuditEventKind`.
- `emit_epoch_audit()` module-private helper: calls `record_audit_event()`, increments dedicated counter, updates `last_epoch_audit_kind`/`last_epoch_audit_sequence`.
- `EpochSubmitted` emitted after `accept_epoch()` passes (on successful submission path only — not on `Rejected_AcceptFailed`).
- `EpochCommitted` emitted after `commit_epoch()` succeeds and state is recorded.
- `EpochAborted` emitted for `Aborted_TaskFault`, `Aborted_ExclusiveConflict`, `Aborted_Timeout`; policy fault retains dedicated `EpochAbortedPolicyFault`.
- `Counters`: `epoch_audit_submissions`, `epoch_audit_commits`, `epoch_audit_aborts`.
- `KernelRuntimeState`: `last_epoch_audit_kind`, `last_epoch_audit_sequence`.
- `KernelRuntimeStatusView`: all five new fields exposed.
- `t81_ternaryos_epoch_audit_test`: 15 assertions across 4 test functions.
- `[DPE-09-01]`: successful epoch → epoch_audit_submissions=1, epoch_audit_commits=1.
- `[DPE-09-02]`: last_epoch_audit_kind == EpochCommitted + sequence set.
- `[DPE-09-03]`: task-fault epoch → epoch_audit_aborts=1, last_kind == EpochAborted.
- `[DPE-09-04]`: timeout epoch → epoch_audit_aborts += 1, last_kind == EpochAborted.
- `[DPE-09-05]`: policy-denied epoch → epoch_audit_aborts NOT incremented; last_kind != EpochAborted.
- `[DPE-09-06]`: audit_events_recorded += 2 per lifecycle.

**Slice 23 — RFC-DPE-0007: Epoch Execution Timeout [DONE]:**

- `Aborted_Timeout` added to `KernelEpochStatus` enum.
- `EpochTimedOut` added to `KernelCallRejection`.
- `epoch_timeout_ms` (`std::optional<std::chrono::milliseconds>`) added to `KernelCallRequest` (guarded by `#ifdef T81_ENABLE_DPE`).
- `axion_kernel_submit_epoch()` gains optional `timeout_ms` parameter (default `std::nullopt`); all existing callers unaffected.
- `kernel_epoch.cpp`: `epoch_start = steady_clock::now()` captured before level loop; after each level's result collection, elapsed is checked and `Aborted_Timeout` returned if `elapsed >= *timeout_ms`.
- `kernel_abi.cpp` `SubmitEpoch` dispatch: passes `request.epoch_timeout_ms` through to `axion_kernel_submit_epoch()`; maps `Aborted_Timeout → RetryLater/EpochTimedOut`.
- `t81_ternaryos_epoch_timeout_test`: 9 assertions across 4 test functions.
- `[DPE-08-01]`: nullopt timeout → Ok (baseline regression check).
- `[DPE-08-02]`: 5 000 ms generous timeout → Ok.
- `[DPE-08-03/04]`: 0 ms timeout → Aborted_Timeout + epochs_aborted/epoch_aborts incremented.
- `[DPE-08-05/06]`: SubmitEpoch syscall with 0 ms → RetryLater/EpochTimedOut; epoch_committed=false.
- RFC-DPE-0007 §7: timeout is a resource-bounding mechanism; EpochHash is never computed on timed-out epochs — replay determinism unaffected.

**Slice 22 — RFC-00B6 §5.3.6: ClaimDevice / ReleaseDevice / QueryDevice as KernelCallKind [DONE]:**

- `ClaimDevice`, `ReleaseDevice`, `QueryDevice` added to `KernelCallKind` enum.
- `device_name` field added to `KernelCallRequest`.
- `device_claimed`, `device_released`, `device_is_claimed`, `device_owner_tid` added to `KernelCallResult`.
- `MissingDeviceName`, `DeviceNotFound`, `DeviceAlreadyClaimed`, `DeviceNotOwned` added to `KernelCallRejection`.
- Dispatch cases in `axion_kernel_call()`: ClaimDevice enforces ownership via `owner_tid` (no separate capability token — device registry is the authority); ReleaseDevice checks caller is owner; QueryDevice is read-only.
- `t81_ternaryos_device_arbitration_syscall_test`: 36 assertions across 9 test functions.
- `[AC-22d-01]`: ClaimDevice on unclaimed device → Ok, device_claimed=true.
- `[AC-22d-02]`: ClaimDevice by different thread → Conflict/DeviceAlreadyClaimed.
- `[AC-22d-03]`: Idempotent re-claim by same owner → Ok.
- `[AC-22d-04]`: ReleaseDevice by owner → Ok, device_released=true.
- `[AC-22d-05]`: ReleaseDevice by non-owner → CapabilityDenied/DeviceNotOwned.
- `[AC-22d-06]`: QueryDevice reports is_claimed=false/true and correct owner_tid.
- `[AC-22d-07]`: Missing device_name → InvalidRequest/MissingDeviceName for all three calls.
- `[AC-22d-08]`: Unknown device name → NotFound/DeviceNotFound for all three calls.
- RFC-00B6 §8 open question (device claim/release via syscall path) is now **fully resolved**.

**Slice 21 — RFC-DPE-0003 §10 / RFC-DPE-0006 §4: SubmitEpoch Kernel Syscall Promotion [DONE]:**

- `SubmitEpoch` added to `KernelCallKind` enum (guarded by `#ifdef T81_ENABLE_DPE`).
- `epoch_graph` / `epoch_programs` fields added to `KernelCallRequest` (guarded).
- `epoch_committed` / `epoch_hash` fields added to `KernelCallResult` (guarded).
- `MissingEpochGraph`, `MissingEpochPrograms`, `EpochAcceptFailed`, `EpochTaskFault`,
  `EpochExclusiveConflict`, `EpochPolicyFault` added to `KernelCallRejection`.
- `SubmitEpoch` dispatch case in `axion_kernel_call()`: validates presence of
  `epoch_graph` and `epoch_programs`; delegates to `axion_kernel_submit_epoch()`;
  maps `KernelEpochStatus` to `KernelCallStatus` / `KernelCallRejection`.
- `t81_ternaryos_epoch_syscall_test`: 20 assertions across 6 test functions.
- `[DPE-07-01]`: valid single-task epoch → Ok, epoch_committed=true, non-zero hash.
- `[DPE-07-02]`: missing epoch_graph → MissingEpochGraph.
- `[DPE-07-03]`: missing epoch_programs → MissingEpochPrograms.
- `[DPE-07-04]`: non-halting task → EpochTaskFault; epoch_committed=false.
- `[DPE-07-05]`: consecutive epochs produce distinct hashes.
- `[DPE-07-06]`: Yield succeeds before and after SubmitEpoch (caller context stable).

**Slice 20 — RFC-DPE-0006: Bounded Thread Pool [DONE]:**

- `DpeThreadPool` class added to `experimental/dpe/thread_pool.hpp/.cpp`:
  N worker threads, `std::queue` + `std::mutex` + two `condition_variable`s
  (`cv_` for workers, `idle_cv_` for `wait_idle()`), `pending_` counter.
- `submit()` enqueues a `std::function<void()>`, increments `pending_`, notifies
  one worker; returns false if pool is stopped. `noexcept`.
- `wait_idle()` blocks until `pending_ == 0`. `noexcept`.
- `shutdown()` sets `stopped_`, notifies all workers, joins all threads.
  Idempotent. Called by destructor.
- `axion_kernel_submit_epoch()` gains optional `DpeThreadPool* pool` parameter
  (default nullptr = RFC-DPE-0005 unbounded behavior, fully backwards-compatible).
  When pool != nullptr, tasks are submitted to the pool; pool submission failure
  falls back to inline execution.
- `t81_dpe_thread_pool_test`: 22 assertions across 4 test functions.
- `[DPE-06-01]`: 4 tasks, 2-worker pool — all 4 complete with correct values.
- `[DPE-06-02]`: EpochHash with 2-worker pool == EpochHash with unbounded dispatch.
- `[DPE-06-03]`: 1-worker pool serialises 3 tasks correctly.
- `[DPE-06-04]`: Destructor (with and without pending tasks) no crash/hang.
- RFC-DPE-0006 is now **accepted** and fully implemented.

**Slice 19 — RFC-DPE-0005: Level-Parallel Epoch Execution [DONE]:**

- `topological_levels_epoch()` added to `task_graph.hpp/.cpp`: level-aware
  Kahn's BFS producing `vector<vector<size_t>>`; each inner vector is one
  topological level sorted by canonical TaskId (RFC-DPE-0005 §3).
- `axion_kernel_submit_epoch()` reworked from topo-sort loop to level loop:
  for each level, pre-flight (policy gate + snapshot construction) is
  sequential; then one `std::thread` is launched per task; threads joined
  before the next level begins (RFC-DPE-0005 §4).
- Thread-creation failure caught with `...`; affected task runs inline as
  fallback (RFC-DPE-0005 §7); `axion_kernel_submit_epoch()` remains
  `noexcept`.
- `t81_dpe_epoch_parallel_test`: 21 assertions across 4 test functions.
- `[DPE-05-01]`: level assignment correct for single task, chain, independent
  pair, and diamond.
- `[DPE-05-02]`: two independent tasks (level 0) each produce correct
  committed values.
- `[DPE-05-03]`: fan-out T0→{T1,T2} — T1 and T2 each receive T0's delta
  via input snapshot and produce correct transformed values (101, 102).
- `[DPE-05-04]`: EpochHash identical regardless of delta_set submission
  order — canonical commit determinism preserved under parallelism.
- RFC-DPE-0005 is now **accepted** and fully implemented.

**Slice 18 — RFC-DPE-0004: DAG-Ordered Multi-Task Epoch Execution [DONE]:**

- `program_identity()` promoted from anonymous namespace to public function in
  `task_graph.hpp/.cpp` (RFC-DPE-0004 §2.1 stable program-identity reference).
- `topological_sort_epoch()` added to `task_graph.hpp/.cpp`: Kahn's algorithm
  over the program-identity dependency graph; tie-breaking by ascending canonical
  TaskId (RFC-DPE-0004 §2.2); returns empty vector on cycle.
- `DpeTaskInputSnapshot` struct added to `task_runner.hpp`: maps `uint64_t
  word_start → page_bytes`; populated from predecessor delta records before task
  execution (RFC-DPE-0004 §4).
- `DpeTaskRunner::run_direct()` updated to accept optional `DpeTaskInputSnapshot`;
  pages are written into VM flat memory via `set_memory_word()` after
  `load_program()` and before output-region pre-snapshot (RFC-DPE-0004 §3.2).
- `axion_kernel_submit_epoch()` reworked: builds program-identity index, calls
  `topological_sort_epoch()`, accumulates predecessor deltas into
  `DpeTaskInputSnapshot` per task (higher canonical TaskId wins on TVA conflict),
  passes snapshot into `run_direct()`.
- `t81_dpe_epoch_dag_test`: 13 assertions across 3 test functions.
- `[DPE-04-01]`: T0 writes 42 to page P; T1's input snapshot contains 42 before T1 runs.
- `[DPE-04-02]`: T1 reads 42, adds 1, commits 43 — page P contains V', not V.
- `[DPE-04-03]`: Reversed array order (T1-first, T0-second) → identical committed
  state — execution follows DAG, not array order.
- `[DPE-04-04]`: Independent task (no deps) unaffected; topo sort returns [0].
- RFC-DPE-0004 is now **accepted** and fully implemented.

**Slice 17 — DPE Policy Audit Wiring / [DPE-03-06] [DONE]:**

- `KernelAuditEventKind::EpochAbortedPolicyFault` added to `kernel_runtime_support.hpp`.
- `KernelEpochStatus::Aborted_PolicyFault` added to `kernel_epoch.hpp`.
- `KernelEpochPolicyGate` struct added to `kernel_epoch.hpp`: raw function pointer
  and `user_data`; default-constructed gate (fn == nullptr) allows all tasks.
- `axion_kernel_submit_epoch()` gains optional `gate` parameter (default `{}`);
  gate is evaluated before each task — denial records `EpochAbortedPolicyFault` in
  the audit log, increments `state.counters.policy_faults`, aborts immediately.
- Existing callers (`epoch_submission_test`) remain source-compatible (default gate).
- `t81_ternaryos_epoch_policy_test`: 15 assertions across 4 test functions.
- `[DPE-03-06-01]`: policy denial → `Aborted_PolicyFault`.
- `[DPE-03-06-02]`: audit log contains `EpochAbortedPolicyFault`; `last_audit_event` set.
- `[DPE-03-06-03]`: `counters.policy_faults` incremented.
- `[DPE-03-06-04]`: `epochs_committed` unchanged on policy abort.
- `[DPE-03-06-05]`: allow-all gate does not affect normal epoch execution.
- `[DPE-03-06-06]`: first-task denial short-circuits — gate called once, not twice.
- RFC-DPE-0003 `[DPE-03-06]` is now **met**; only `[DPE-03-05]` (T81Float strict path,
  deferred to RFC-0030) remains open.

**Slice 16 — Kernel EpochRuntimeState Wiring [DONE]:**

- `EpochRuntimeState` struct added to `KernelRuntimeState` (after `pager_worker{}`):
  `epochs_submitted`, `epochs_committed`, `epochs_aborted`, `epoch_task_executions`,
  `last_committed_epoch_id`, `last_committed_epoch_hash`.
- Epoch counters mirrored in `Counters` struct (for `KernelRuntimeStatusView` exposure).
- `KernelRuntimeStatusView` extended with `epoch_submissions`, `epoch_commits`,
  `epoch_aborts`, `epoch_task_executions`, `last_committed_epoch_id`,
  `last_committed_epoch_hash` (RFC-DPE-0003 §7).
- `make_runtime_view()` updated to populate epoch fields from state.
- `TaskDeltaSet` struct + `commit_epoch(EpochGraph, vector<TaskDeltaSet>)` overload
  added to `epoch_commit.hpp/cpp` — kernel-facing bridge from `DpeTaskRunner` to
  commit engine without live `DeltaBuffer` objects.
- `kernel_epoch.hpp` / `kernel_epoch.cpp`:
  `axion_kernel_submit_epoch(KernelRuntimeState&, EpochGraph, vector<Program>)` —
  validates via `accept_epoch()` → runs tasks via `DpeTaskRunner::run_direct()` →
  commits via `commit_epoch(TaskDeltaSet)` → updates counters and epoch state.
- `t81_ternaryos_epoch_submission_test`: 22 assertions across 4 test functions.
- `[AC-22s-01/02]`: counters and last_committed_epoch_id/hash set after successful epoch.
- `[AC-22s-03]`: `make_runtime_view()` reflects epoch counters.
- `[AC-22s-04]`: faulted (non-halting) task aborts epoch; committed counter unchanged.
- `[AC-22s-05]`: consecutive epochs accumulate counters correctly.
- CMake: `kernel_epoch.cpp` added to `t81_ternaryos_hal` via `target_sources` when
  both `T81_ENABLE_TERNARYOS=ON` and `T81_ENABLE_DPE=ON`; `t81_dpe` linked in.

**Slice 12 — First EL0→EL1 SVC Roundtrip [DONE] (commit 69fcc987):**

- `[AC-22r]` `test_kernel_el0_svc_roundtrip()` — 28 assertions.
- Spawns a non-kernel-owned user thread; maps request + response TVAs in
  its address space; arms `axion_kernel_set_kernel_state_for_trap_dispatch()`.
- Dispatches `Yield` and `GetThreadIdentity` through `axion_kernel_handle_svc_trap_aarch64()`
  via synthetic `AArch64TrapFrame`; verifies response round-trip and `caller_tid`.
- Confirms SVC #7 rejection leaves dispatch counter unchanged.
- Confirms post-disarm call is a no-op; validates runtime status view.
- Suite: 3112 passed, 0 failed.

**Slice 27 — Interrupt Policy Gate (RFC-00B5 §3.7 / Slice 27) [DONE]:**

- `kernel_interrupt_policy.hpp` / `kernel_interrupt_policy.cpp` (new): implements
  `evaluate_interrupt_policy()` (rate-limit window + quarantine check) and
  `record_interrupt_policy_event()` (audit only on Quarantine/Deny; Allow is silent
  so `InterruptDelivered` remains the last audit event on the normal path).
- `kernel_runtime_support.hpp`: `InterruptPolicyVerdict` enum (`Allow`,
  `Quarantine`, `Deny`); `KernelInterruptRateConfig`; `KernelInterruptPolicySourceState`;
  three new `KernelAuditEventKind` entries (`InterruptPolicyAllow`,
  `InterruptPolicyQuarantine`, `InterruptPolicyDeny`).
- `kernel_runtime_state.hpp`: `interrupt_policy` map keyed by `uint8_t` source;
  `last_interrupt_policy_verdict` / `last_interrupt_policy_source` /
  `last_interrupt_policy_sequence` optionals; three counters
  (`interrupts_policy_allowed`, `interrupts_policy_quarantined`,
  `interrupts_policy_denied`).
- `kernel_abi.hpp`: `SetInterruptPolicy`, `ClearInterruptQuarantine`,
  `QueryInterruptPolicy` in `KernelCallKind`; matching request/result fields;
  `MissingInterruptPolicySource` and `InterruptSourceNotQuarantined` rejections.
- `kernel_interrupts.cpp`: policy gate block wired before the source-specific
  switch; Quarantine/Deny return early without dispatching.
- `kernel_abi.cpp`: three new call handlers for `SetInterruptPolicy`,
  `ClearInterruptQuarantine`, `QueryInterruptPolicy`.
- `kernel_service_contract.hpp` + `kernel_views.cpp`: three counters and last
  policy verdict/source/sequence exposed through `KernelRuntimeStatusView`.
- `[AC-22w]` (44 assertions): Default→allow; SetInterruptPolicy(Storage, max=2,
  window=100); two allowed → third triggers Quarantine (interrupt dropped,
  device_wakes unchanged) → fourth Denied; ClearInterruptQuarantine → fifth
  Allowed; QueryInterruptPolicy returns correct config; Timer unaffected;
  missing-source and non-quarantined rejections; runtime view exposes all counters.
- Suite: 3203 passed, 0 failed.

**Slice 26 — Keyboard WaitForDevice (RFC-00B5 §3.3 / Slice 26) [DONE]:**

- `kernel_interrupts.cpp`: Keyboard interrupt delivery now wakes any threads
  parked via `WaitForDevice(Keyboard)`, matching the Storage/Network wake path
  exactly.  Previously Keyboard was accounting-only.
- `Counters::keyboard_wakes` added to `KernelRuntimeState` — tracks
  Keyboard-specific wakes separately from the general `device_wakes` counter.
- `KernelRuntimeStatusView::keyboard_wakes` added to `kernel_service_contract.hpp`
  and populated in `kernel_views.cpp`.
- `kernel_abi.cpp` `WaitForDevice` comment updated: Storage, Network, and
  Keyboard are now all valid park sources.
- `[AC-22v]` (36 assertions): WaitForDevice(Keyboard) → Ok, thread_sleeping →
  Keyboard interrupt delivery → device_wakes/keyboard_wakes incremented →
  device_waiting_tids[Keyboard] cleared → ReceiveMessage delivers synthetic
  "device-wake" IPC with correct sender/tag/payload → second Keyboard interrupt
  with no waiter leaves counters unchanged → runtime status view exposes both
  `device_wakes` and `keyboard_wakes`.
- Suite: 3148 passed, 0 failed.

**Slice 28 — Unhandled IRQ Governance (RFC-00B5 §3.5) [DONE]:**

- `hal/hal.hpp`: `register_unhandled_interrupt_callback(InterruptHandler)` API added.
- `hal/interrupt_table.cpp`: `g_unhandled_callback` global; fallback in `dispatch_interrupt()`
  invokes unhandled callback when source has no registered handler.
- `kernel_runtime_support.hpp`: `UnhandledInterruptDropped` added to `KernelAuditEventKind`.
- `kernel_runtime_state.hpp`: `interrupts_unhandled` counter added to `KernelRuntimeCounters`.
- `kernel_main.hpp` + `kernel_interrupts.cpp`: `axion_kernel_record_unhandled_interrupt()`
  declared and implemented — increments counter, emits `UnhandledInterruptDropped` audit,
  updates `last_interrupt_audit_*` fields.
- `hal/qemu_kernel_entry.cpp`: `register_unhandled_interrupt_callback()` wired in
  `qemu_kernel_run_loop()` to call `axion_kernel_record_unhandled_interrupt()`.
- `[AC-22x]` (11 assertions): registers callback; fires Network interrupt with null handler
  registered; verifies `interrupts_unhandled=1`, audit kind=`UnhandledInterruptDropped`,
  source=Network, `interrupts_delivered` unchanged; second fire increments to 2; callback
  disarmed cleanly.
- RFC-00B5 status advanced: `accepted` → `integrated`.
- Suite: 3214 passed, 0 failed.

**QEMU x86_64 EFI Boot Lane [DONE] (2026-03-16):**

- BOOTX64.EFI executed under QEMU TCG (`qemu-system-x86_64`) with EDK2 OVMF firmware.
- All 5 contract files written to guest disk and verified:
  `efi-ran.txt`, `boot-report.txt`, `startup-status.txt`,
  `expected-boot-report.txt`, `expected-startup-status.txt`.
- Boot report: `hal_main_result=0`, `kernel_boot_ready_slice=complete`,
  `boot_progress_state=ready`, `platform_id=virtualbox-x86_64:VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC/acceptance-lane`.
- Startup status: `os_name=Axion`, `phase=5`, `storage_binding=virtualbox-ahci`,
  `display_binding=virtualbox-vmsvga`, `network_binding=virtualbox-e1000`.
- Serial log: 7251 bytes; all expected markers seen.
- Evidence record: `docs/records/audits/TERNARYOS_X86_64_BOOT_EVIDENCE_2026-03-16.md`.

**RFC-DPE-0002 (TISC Task Graph Primitives) advanced `draft` → `accepted` (2026-03-15):**

- All five acceptance criteria met: `[DPE-02-01..05]` proved by
  `t81_dpe_test`, `t81_dpe_task_runner_test`, `t81_ternaryos_epoch_syscall_test`.
- Acceptance note added to RFC-DPE-0002 with criterion-to-evidence mapping.
