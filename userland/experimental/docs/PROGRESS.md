# Axion Implementation Progress

Current working release label: `Axion v0.1.0-alpha`

Working name note: the project-facing OS name is now `Axion`, while internal
paths, namespaces, and build/test identifiers still use `ternaryos`.

Current naming split:

- `T81 Foundation` = umbrella project/ecosystem
- `T81VM` = ternary runtime/execution substrate
- `Axion` = operating system
- `CanonFS` / `TISC` remain subsystem names

RFC-00B9 boundary note:

- the supported user-environment and shell public surface now lives under
  `include/t81/axion/userenv/`, `include/t81/axion/shell/`,
  `src/axion/userenv/`, and `src/axion/shell/`
- the matching headers under `experimental/ternaryos/userenv/` and
  `experimental/ternaryos/shell/` remain compatibility shims while the
  broader kernel lane stays experimental

**Last updated:** 2026-03-16
**Commit:** Slice 28 — RFC-00B5 integrated (UnhandledInterruptDropped)
**Branch:** `main`

Recent architecture milestone:

- **RFC-00B5 — Governed Event Interrupt Model** is now **integrated** (2026-03-16).
  All three interrupt policy slices complete:
  - Slice 26: `WaitForDevice` — Storage/Network/Keyboard parks thread; interrupt wakes it
  - Slice 27: Interrupt policy gate — per-source rate-limit, quarantine, deny verdicts
  - Slice 28: Unhandled interrupt governance — `UnhandledInterruptDropped` audit event
  Total: 3214/3214 assertions passing; interrupt model now official architecture.
  thread → fault triggers handoff dispatch → svc thread woken → IPC message carries AS id
  → RequestPageMapping → pager_needed cleared → runtime view reflects final state.
  3048 `hal_boot_test` assertions pass (was 3016 after Slice 9).

- **Slice 9 — first public pager service ABI** is now complete (RFC-00B7 §3.2).
  Introduces `KernelCapabilityKind::PagerService` (new capability kind) and
  `KernelCallKind::RequestPageMapping` (new ABI call).  A thread holding `PagerService`
  capability calls `RequestPageMapping{address_space_id=X}` to supply a page mapping for a
  pager-needed address space.  The kernel validates: (1) `PagerService` capability present,
  (2) `address_space_id` provided and exists, (3) `pager_needed == true`, (4)
  `last_pager_fault` recorded.  On success `mmu_map()` is called at `last_pager_fault->tva`
  with permissions derived from `last_pager_fault->access_mode`; `pager_service_mappings`
  counter increments; result carries `pager_mapping_supplied = true`.  On the next
  `axion_kernel_step()` tick the pager worker detects the mapping via `is_pager_work_item_ready()`
  and calls `resolve_completed_pager_work()` to clear `pager_needed` — no new pager-worker code
  required.  New rejection variants: `AddressSpaceNotPagerNeeded`, `MissingPagerFault`.  Counter
  `pager_service_mappings` exposed via `KernelRuntimeStatusView`.  `[AC-22o]` (34 assertions):
  victim fault → pager stall → grant PagerService → rejection paths (no AS, bad AS, not
  pager_needed) → success → TVA mapped → worker resolves → pager_needed cleared → second call
  rejected → runtime view.  3016 `hal_boot_test` assertions pass (was 2982 after Slice 8).

- **Slice 8 — AArch64 exception vector table** is now complete (RFC-00B6 §5.2 / ARM DDI 0487).
  `aarch64_exception_vectors.S` provides a correct 2 KiB-aligned VBAR_EL1 exception vector table
  with 16 slots (128 bytes each).  The only functional slot is offset 0x400 (Lower EL, AArch64,
  Synchronous) which branches to `axion_svc_entry` — an out-of-table handler that saves all 31
  general-purpose registers plus `sp_el0`, `elr_el1`, `spsr_el1`, and `esr_el1` into an
  `AArch64TrapFrame` (280 bytes) on the EL1 stack, calls
  `axion_kernel_handle_svc_trap_aarch64()`, restores the frame, and executes `eret`.  All other
  exception slots branch to `axion_vec_unhandled` (deliberate infinite loop).
  `aarch64_trap_entry.hpp` defines `AArch64TrapFrame` with compile-time `static_assert` offset
  checks, ESR_EL1 decode helpers (`aarch64_svc_imm_from_esr`, `aarch64_ec_from_esr`), and the
  `axion_kernel_svc_frame_from_aarch64()` bridge (x0→request_tva, x1→response_tva, ESR→svc_imm).
  `aarch64_trap_entry.cpp` provides `axion_kernel_set_kernel_state_for_trap_dispatch()`,
  `axion_kernel_handle_svc_trap_aarch64()` (delegates to `axion_kernel_handle_svc_trap()`), and
  `axion_kernel_install_exception_vectors()` (writes VBAR_EL1 on bare-metal AArch64; documented
  no-op on macOS/host builds).  Assembly verified to compile for `aarch64-pc-windows-msvc` via
  CMake target `t81_ternaryos_aarch64_exception_vectors_obj`.  `[AC-22n]` (24 assertions): struct
  layout, ESR decode, bridge conversion, null-safety, SVC #7 rejection via bridge, install
  callable.  2982 `hal_boot_test` assertions pass (was 2958 after Slice 7).

- **Slice 7 — CanonFS-backed executable object acquisition** is now complete (RFC-00B2 §3.1).
  `SpawnThreadFromExecutableObject` now fetches a `CanonExec` block directly from the bound
  `published_executable_canonfs` driver when the requested `CanonRef` is not present in the
  in-memory registry — no prior `RegisterExecutableObject` call required.  On a registry miss the
  kernel calls `load_published_executable_block()` (CanonFS primary → CanonStore fallback),
  decodes the block, builds a temporary `ExecutableRecord`, increments
  `counters.canonfs_fetch_spawns`, then proceeds through the existing section-loader and thread-spawn
  path unchanged.  A missing `CanonRef` (not in registry and not in CanonFS) still returns
  `MissingExecutableRegistration`.  New counter `canonfs_fetch_spawns` is exposed through
  `KernelRuntimeStatusView`.  `[AC-22m]` (27 assertions): in-memory driver populated, spawn without
  registration returns Ok, counter reaches 1, page mapped, spawned PC/label correct, re-spawn
  reuses existing page and counter reaches 2, unknown CanonRef returns
  `MissingExecutableRegistration`, runtime status view exposes `canonfs_fetch_spawns == 2`.
  2958 `hal_boot_test` assertions pass (was 2931 after Slice 6).

- **Slice 6 — QEMU AArch64 EDK2 guest image bootstrap** is now complete (RFC-00B3 §3.9).
  A freestanding `BOOTAA64.EFI` (compiled with `--target=aarch64-pc-windows-msvc -ffreestanding
  -nostdlib`, linked with LLD) boots the QEMU `virt` machine under EDK2 AArch64 firmware without
  a host OS process boundary.  At UEFI runtime the stub: (1) emits a serial banner detectable in
  the QEMU serial log; (2) writes `TERNOS/efi-slice6-ran.txt` (execution marker) and
  `TERNOS/slice6-boot-report.txt` (Slice 4+5 readiness data) to the FAT32 boot volume; (3)
  calls `ternaryos_hal_main_c()` via the thin hosted shim.  The report content is generated at
  build time by `qemu_slice6_startup_snapshot` which: bootstraps the Axion kernel with the
  QEMU virt memory layout, exercises `axion_kernel_handle_svc_trap()` with `svc_imm=7` (Slice 4
  rejection confirmed), spawns a user thread and attempts a kernel-space TVA write from user AS
  (Slice 5 isolation confirmed; `kernel_space_rejections=1`), queries
  `axion_kernel_service_request(RuntimeStatus)` (runtime status view confirmed), and writes
  `qemu_slice6_startup.h`.  The probe script (`run_qemu_armv8_slice6_probe.sh`) boots QEMU,
  mounts the FAT32 image, extracts artefacts, and calls `validate_qemu_armv8_slice6_reports.sh`
  which verifies: `slice4_svc_trap_wiring=complete`, `slice5_user_isolation=complete`,
  `runtime_status_ok=true`, `kernel_boot_ready_slice=slice6-efi-boot`,
  `boot_validation_lane=qemu-armv8-slice6-probe`.  CMake target
  `t81_ternaryos_qemu_armv8_slice6_probe` covers the full build-to-boot-to-validate pipeline.
  All existing 2931 `hal_boot_test` assertions remain passing.

- **Slice 5 — User-mode address space isolation** is now implemented (RFC-00B1 §3.1 / RFC-00B6 §5.7).
  `kKernelSpaceVpnBase = 3^19 = 1,162,261,467` splits the 20-trit VPN space: VPN 0..3^19−1
  is user space; VPN 3^19..3^20−1 is kernel-only.  New inline helpers `tva_in_user_space()`
  and `tva_in_kernel_space()` added to `mmu/tva.hpp`.  `AddressSpaceState` gains
  `kernel_owned` flag, set to `true` only for `kKernelAddressSpace` at bootstrap.
  `axion_kernel_validate_address_space_span()` rejects any user-owned AS that tries to
  span a kernel-space TVA.  `axion_kernel_write_address_space_bytes()` and the
  wire-TVA path increment `counters.kernel_space_rejections` on such rejections.
  New field exposed through `KernelRuntimeStatusView`.
  `[AC-22l]` (28 assertions): `kernel_owned` flags, TVA helper predicates, validate/write
  rejection for kernel-space TVA, user-space TVA accepted, wire-TVA structured error
  response, runtime view exposes `kernel_space_rejections`.
- **Slice 4 — Syscall trap wiring** is now implemented (RFC-00B6 §5.2 / RFC-00B0 §3.4.2).
  New files `kernel_trap_shim.hpp` / `kernel_trap_shim.cpp` model the ARM `svc`
  exception entry path in the hosted simulation.  `SvcTrapFrame {request_tva,
  response_tva, svc_imm}` carries the context a real AArch64 exception handler
  would save from `x0`, `x1`, and the SVC immediate field.
  `axion_kernel_handle_svc_trap()` enforces the Axion convention (svc_imm == 0
  is the single unified kernel-call entry) and delegates to the existing
  `axion_kernel_call_wire_tva()` path — no new ABI logic is invented.  A
  new `syscall_trap_dispatches` counter in `Counters` (and exposed through
  `KernelRuntimeStatusView`) tracks dispatch volume.  `[AC-22k]` (28 assertions):
  svc_imm=0 dispatch round-trip via Yield, response wire block valid + decoded,
  svc_imm=7 rejection, counter monotonicity across two dispatches, runtime
  status view exposes `syscall_trap_dispatches`.
- **Slice 3 — Device-wake interrupts** is now implemented (RFC-00B5 §3.3).
  A new `KernelCallKind::WaitForDevice` handler parks the calling thread in
  `device_waiting_tids[source]` via `scheduler.sleep()`.  When a Storage or
  Network interrupt is delivered by `axion_kernel_deliver_pending_interrupt`,
  the kernel sends a synthetic `CanonMessage` (sender=kKernelTid,
  tag=`"device-wake"`, payload=interrupt sequence) to each waiting thread and
  calls `scheduler.wake()`.  Keyboard remains accounting-only.
  `device_wakes` counter and `device_waiting_thread_count` view field are
  exposed through `KernelRuntimeStatusView`.
  `[AC-22j]` (43 assertions): Storage park/wake/message round-trip, Network
  park/wake, Keyboard no-wake, runtime status view coverage.
- **Slice 2 — Blocking IPC** is now also complete (RFC-00B6 §5.3.2 / RFC-00B5 §3.6).
  A new `KernelCallKind::BlockOnIpcReceive` handler in `kernel_abi.cpp` parks
  the calling thread on an empty inbox via `scheduler.sleep()`, records its TID
  in the new `ipc_blocked_tids` unordered_set in `KernelRuntimeState`, and
  increments `counters.ipc_blocks`.  `SendMessage` now checks that set after
  delivery: if the destination thread was IPC-blocked, it calls
  `scheduler.wake()`, removes the TID, and increments `counters.ipc_wakes`.
  The fast path (message already present when `BlockOnIpcReceive` is called)
  returns the message immediately without sleeping.  New fields exposed through
  `KernelRuntimeStatusView`: `ipc_blocks`, `ipc_wakes`,
  `ipc_blocked_thread_count`.
- `KernelCallResult` gains `bool thread_sleeping{false}` so callers can
  distinguish a parked return from a successful synchronous one.
- New test `[AC-22i]` (45 assertions) proves: receiver parks on empty inbox,
  `ipc_blocked_tids` is updated, scheduler shows one sleeping thread, sender's
  `SendMessage` wakes the receiver, `ReceiveMessage` delivers the message with
  correct sender/payload/tag/CanonRef, fast path returns without sleeping, and
  runtime status view exposes all three new fields.
- **Slice 1A — Real Executable Load** (previous milestone) is also complete.
  `load_canon_exec_sections()` maps the CanonExec image block into the kernel
  page table with read+execute permissions; `SpawnThreadFromExecutableObject`
  calls it before spawning.  `[AC-22h]` (24 assertions) proves the section
  load path.  Total ternaryos assertions: **3751**.
- Previous milestone: RFC-00B5 first interrupt policy slice — Timer interrupt
  delivery forces `axion_kernel_tick()` for preemptive scheduling.  New
  counters `timer_interrupts_handled`, `timer_preempts`,
  `device_interrupts_handled` and retained state `last_timer_preempt_cycle`,
  `last_timer_preempt_sequence` exposed through `KernelRuntimeStatusView`.
  Test `[AC-22g]` (35 assertions).

Previous architecture milestone:

- The original kernel monolith has now been decomposed into subsystem-oriented
  units: `kernel_runtime.cpp`, `kernel_views.cpp`, `kernel_queries.cpp`,
  `kernel_actions.cpp`, `kernel_faults.cpp`, `kernel_interrupts.cpp`,
  `kernel_pager.cpp`, `kernel_lifecycle.cpp`, and
  `kernel_runtime_utils.cpp`.
- `kernel_main.hpp` remains intentionally shared for now; header splitting is
  deferred until the runtime-state/API boundary is narrower.
- RFC-00B6 is now partially implemented in code: the typed `axion_kernel_call`
  boundary covers thread identity/spawn/termination, same-supervisor
  execution control, IPC, fault inbox/recovery, supervisor recovery and
  inspection, capability management, service
  lifecycle control, process-group memory inspection, boot-critical
  address-space control, and guarded runtime/fault summary queries.
- Execution control now carries an explicit spawn descriptor too, so
  `SpawnThreadInCallerGroup` and `SpawnThreadUnderSupervisor` can seed
  `pc`, `sp`, `register0`, `label`, and halted/active state through the
  typed ABI, wire blocks, and exported C bridge.
- Process groups now also have reusable named thread entries through
  `RegisterThreadEntryDescriptor` and `SpawnThreadFromEntryDescriptor`.
- Process groups now also have CanonRef-backed executable records through
  `RegisterExecutableObject`, `QueryExecutableObject`, and
  `SpawnThreadFromExecutableObject`.
- Executable registration now validates a canonical `CanonExec` block against
  the supplied `CanonRef`, so those records are no longer only placeholder
  hash/descriptor pairs.
- Executable objects can now also be registered from mapped caller memory
  through `RegisterExecutableObjectFromTva`, so the kernel can ingest a real
  `CanonExec` block over the address-space boundary.
- Those validated executable images can now also be published into a
  kernel-owned repository and later registered by `CanonRef` alone.
- That published-executable repository is now bindable to an external
  `IBlockDevice`, so a fresh kernel can reload and register published
  executables from the same stored backing image.
- `RegisterExecutableObject` can now also resolve published executable objects
  directly from a persistent CanonFS root when one is bound into the kernel.
- In the hosted lane, `axion_kernel_bootstrap(...)` now auto-attaches that
  CanonFS executable source when `T81_CANONFS_ROOT` is set.
- Axion can now also adopt the VirtualBox guest storage binding directly as
  the published executable store, so the executable object lane can persist
  across a fresh guest bootstrap over the same backing image.
- Wire and hosted C executable-object results now also carry the stored
  executable entry descriptor, not just the CanonRef key.
- Services can now retain an optional thread entry descriptor during
  `RegisterService`, and `SpawnThreadForService` can execute that stored
  descriptor in the owning process group.
- Services can now also bind to a previously registered executable object by
  CanonRef during `RegisterService`, and service register/query/spawn results
  now preserve that backing executable object identity.
- Service status and supervisor service-inventory views now also expose that
  stored service entry descriptor, and the typed ABI mirrors that state on
  `RegisterService` / `QueryServiceStatus`.
- Fixed-size wire responses and the hosted C bridge now also carry stored
  service entry descriptors on service register/query results.
- `QuerySupervisorServiceInventory` is now live on the typed ABI and carried
  through fixed-size wire and hosted C responses, including bounded service
  entries with stored service entry descriptors.
- Wire and hosted C coverage now explicitly verifies supervisor-scoped service
  inventory preserves service names and stored entry descriptors.
- `QuerySupervisorServiceStatus` is now live too. It provides a supervisor-
  scoped single-service inspection path that can still read managed service
  state when the ordinary service-status query is deferred by unhealthy state.
- Execution inspection now includes `QueryThreadExecutionState`, with
  caller-local and same-supervisor thread-context reads exposed through the
  typed ABI, wire transport, and hosted C bridge.
- Capability management on that ABI is now stronger than the initial slice:
  capability records have kernel-issued stable IDs, supervisors can inspect
  bounded recent capability-transition history, and revocation can target an
  observed transition sequence instead of only a freshly scanned capability
  list.
- Capability provenance is now explicit as well: default process-group
  capabilities are marked as kernel-seeded, later same-supervisor grants are
  marked as delegated, and delegator process-group/supervisor identity now
  flows through capability queries, inventory views, and transition history.
- Delegated capability control is now broader too: supervisors can revoke
  delegated capabilities in bulk by delegator provenance without disturbing
  kernel-seeded capabilities in the target process group.
- Delegated capability reads now match that control surface: supervisors can
  query only the capabilities delegated by a specific delegator
  process-group/supervisor pair instead of diffing full capability inventories.
- Supervisors now also have a compact delegation-summary view with counts by
  target process group and delegator provenance, so broad delegation state is
  inspectable without scanning individual capability records.
- ABI rejection taxonomy is now more explicit as well. Address-space ownership,
  missing boot-critical control values, and foreign supervisory read scope now
  produce dedicated rejections instead of collapsing into generic policy
  denials.
- The ABI now also has a canonical transport layer: fixed-size
  `KernelCallWireRequestBlock` / `KernelCallWireResponseBlock` transport,
  pointer-style wire dispatch, a raw byte-span bridge, a mapped-TVA bridge,
  and exported hosted C entrypoints `ternaryos_kernel_call_c(...)` and
  `ternaryos_kernel_call_tva_c(...)`.
- The mapped-TVA bridge is now slightly less trusted-host oriented: it no
  longer takes an explicit address-space id from the caller and instead
  derives the current caller address space from kernel runtime state.
- That mapped-TVA bridge now also performs explicit span validation before
  dispatch. Invalid request spans return structured ABI faults when the
  response span is still writable, while invalid response spans remain hard
  bridge failures.
- That exported hosted ABI now also has opaque runtime-handle lifecycle:
  `ternaryos_kernel_bootstrap_c(...)` and `ternaryos_kernel_destroy_c(...)`.

Reference docs:
- Roadmap: [docs/research/ternary_os_roadmap.md](../../../docs/research/ternary_os_roadmap.md)
- RFC-00B0 (HAL): [spec/rfcs/RFC-00B0-hal-spec.md](../../../spec/rfcs/RFC-00B0-hal-spec.md)
- RFC-00B1 (MMU): [spec/rfcs/RFC-00B1-ternary-mmu.md](../../../spec/rfcs/RFC-00B1-ternary-mmu.md)
- RFC-00B2 (Drivers): [spec/rfcs/RFC-00B2-device-drivers.md](../../../spec/rfcs/RFC-00B2-device-drivers.md)
- RFC-00B3 (Kernel): [spec/rfcs/RFC-00B3-axion-kernel-architecture.md](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md)
- RFC-00B5 (Interrupts): [spec/rfcs/RFC-00B5-governed-event-interrupt-model.md](../../../spec/rfcs/RFC-00B5-governed-event-interrupt-model.md)
- RFC-00B6 (Kernel ABI): [spec/rfcs/RFC-00B6-minimal-syscall-capability-boundary.md](../../../spec/rfcs/RFC-00B6-minimal-syscall-capability-boundary.md)
- x86_64 handoff: [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
- Axion shell design: [axion_shell_design.md](axion_shell_design.md)
- Architecture audit: [kernel_architecture_audit.md](kernel_architecture_audit.md)
- Kernel execution plan: [kernel_execution_plan.md](kernel_execution_plan.md)
- Engineering follow-on plan: [kernel_engineering_follow_on_plan.md](kernel_engineering_follow_on_plan.md)

---

## Status by Phase

### Phase 1 — Bootloader & HAL ✅ COMPLETE

**Gate condition (v1.5):** TISC `NOP`/`HALT` executes with no host OS.
Status: hosted simulation passing; VirtualBox guest promotion is now the first concrete non-hosted target.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `hal/hal.hpp` | Public interface: `MemoryRegion`, `HardwareInterrupt`, `BootContext`, `hal_main`, I/O stubs | — |
| `hal/hal_c_abi.h/.cpp` | C-compatible HAL handoff bridge so freestanding guest stubs can construct a boot context without C++ STL dependencies, plus exported hosted kernel lifecycle (`ternaryos_kernel_bootstrap_c(...)` / `ternaryos_kernel_destroy_c(...)`) and `ternaryos_kernel_call_c(...)` ABI bridges | 19 |
| `hal/hal_main.cpp` | Ethics-first boot — validates `BootContext`, evaluates Θ₁–Θ₉, and hands off to the first kernel-owned runtime entry | 9 |
| `hal/interrupt_table.cpp` | Shadow binary dispatch table; `register_interrupt_handler`, `dispatch_interrupt`, `fire_simulated_interrupt`; governed interrupt model now tracked by RFC-00B5 | (above) |
| `hal/hosted_stub.cpp` | macOS/Linux UEFI stub simulation; synthetic memory map; calls `hal_main` | (above) |
| `kernel/kernel_main.hpp` + `kernel/kernel_runtime.cpp` | First Axion kernel-owned runtime entry/bootstrap over validated `BootContext`; computes runtime state, records MMU faults, queues and delivers faults through the loop, routes delivered faults into per-thread runtime state, quarantines faulting threads deterministically, gates recovery through kernel-owned process-group policy, emits audit-only governance events, drives deterministic scheduler dispatch, routes CanonRef-safe IPC, provides a deterministic kernel-step loop with counters, performs active device claims/releases, rejects invalid handoff contexts, exposes a service-facing request/result contract with deterministic healthy/faulted-group behavior plus stable diagnostics for runtime, process-group, supervisor, fault, device, audit, and per-device ownership summaries, and now carries a kernel-owned service runtime layer with service ids, supervisor ownership, backing process-group linkage, blocked/suspended/unhealthy state, stable service detail, lifecycle counters, richer supervisor-owned service inventory, deterministic register/unregister/suspend/resume/health actions, audit-visible lifecycle transitions, retained supervisor inventory metadata for the latest managed-service lifecycle transition, per-entry supervisor inventory transition metadata, compact supervisor-status lifecycle metadata, aligned supervisor-recovery lifecycle metadata, aligned fault-summary lifecycle metadata, aligned runtime-status lifecycle metadata, aligned audit-summary lifecycle metadata, aligned device-summary lifecycle metadata, aligned service-status transition metadata, explicit kernel-owned address-space ownership diagnostics bound to process groups, internal pager-needed fault-state diagnostics that distinguish `Unmapped` faults from policy faults, deterministic internal pager-handoff state that distinguishes pending from dispatched handoffs, deterministic internal pager-resolution state once the missing mapping appears, and a first real kernel-owned pager worker with FIFO consumption, deterministic repeated cycles, coalesced duplicate unresolved faults on worker-owned address spaces, retained backlog/load high-water diagnostics, explicit FIFO stall/backlog-blocked counters, explicit ready-behind-active backlog diagnostics, ready-backlog depth/high-water tracking, retained receipt identities/ordinals, active-work handoff ordinals, queued-head identities/ordinals, deterministic bounded ready-bypass activation on idle worker selection, retained ready-bypass deferral diagnostics, parked capped deferral once a blocked head has already used its one bypass, retained parked-cycle and parked-episode diagnostics, live parked-ready backlog diagnostics, retained parked-resumption diagnostics, retained parked-resume backlog diagnostics, retained parked-resume handoff diagnostics, retained parked-resumed-head handoff diagnostics, retained parked-resolved-head diagnostics, retained parked-resolved remaining-work diagnostics, retained parked-resolution follow-on diagnostics, retained parked-resolution successor completion diagnostics, retained blocker/blocked address-space identities, retained stall ordinals, retained blocked-side stall ordinals, retained blocked-side backlog depth, retained activation identities/ordinals, retained completion identities/ordinals, a deterministic terminal parked-head failure policy with retained terminal diagnostics, a kernel-owned boot-critical pager auto-resolution policy with retained resolution diagnostics, and explicit boot-progress/fail runtime reporting | 127 |
| `hal/virtualbox_platform.hpp/.cpp` | First-target VirtualBox promotion scaffold: VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC profile validation, device-map descriptors, timer-tick simulation, and `BootContext` construction | 43 |
| `hal/virtualbox_guest_devices.hpp/.cpp` | VirtualBox guest-device binding seam: maps the first supported HAL storage, network, and display profile onto AHCI/E1000/VMSVGA-shaped Phase 4 adapters, rejects unsupported NVMe/PCNet/VGA promotion paths, and bootstraps the first guest profile as a reusable runtime bundle | 39 |
| `hal/virtualbox_efi_stub.c` | Freestanding VBox EFI guest stub source: constructs the first guest `BootContext` via the C ABI bridge, emits the narrow boot-report/startup-status contract, and serves as the source for the staged `BOOTX64.obj` plus local diagnostic `BOOTX64.EFI` artifact | — |
| `hal/virtualbox_armv8_efi_stub.c` | Freestanding ARMv8 VBox EFI guest stub source for the secondary developer lane; constructs an ARMv8 `BootContext` via the same C ABI bridge, emits the live serial banner and boot report, and now embeds a backend-generated Axion shell startup snapshot into the staged guest artifact | — |
| `hal/virtualbox_armv8_efi_control.c` | Ultra-minimal ARMv8 VBox EFI control application for the secondary developer lane; writes a standalone execution marker without touching the HAL bridge so Apple Silicon VirtualBox can be tested for "any EFI app runs at all" | — |
| `hal/virtualbox_armv8_efi_shim.c` | Temporary ARMv8 developer-lane EFI link shim: provides a freestanding `ternaryos_hal_main_c` stub so Apple Silicon hosts can emit a real `BOOTAA64.EFI` without claiming the true C++ HAL bridge is linked into PE/COFF yet | — |
| `scripts/build_virtualbox_guest_artifact.sh` | Reproducible VirtualBox guest-artifact pipeline: emits both the primary `x86_64` artifact lane and a temporary ARMv8 developer-lane artifact, stages `BOOTX64.obj` / `BOOTAA64.obj`, links a local diagnostic `BOOTX64.EFI` plus the developer-lane `BOOTAA64.EFI`, and places `STARTUP.NSH` at both the filesystem root and `EFI/BOOT` for probe fallback | — |
| `scripts/package_virtualbox_x86_64_handoff.sh` | Packages the official `x86_64` VirtualBox handoff lane into a portable bundle with the guest artifact, profile summary, expected boot-progress contract files, a comparison helper, demo output, and runbook for an external validator | — |
| `scripts/validate_virtualbox_x86_64_handoff.sh` | Compares guest-produced `boot-report.txt` / `startup-status.txt` against the packaged `x86_64` acceptance-lane contract files inside the handoff bundle | — |
| `scripts/check_virtualbox_host.sh` | Host-capability check for local VirtualBox validation: reports whether the current machine can run the roadmap’s `x86_64` guest target or the temporary ARMv8 developer lane | — |
| `scripts/check_efi_link_toolchain.sh` | EFI-link capability check: records whether the current host has a PE/COFF linker (`lld-link` / `ld.lld`) available to turn staged EFI stub objects into final `.efi` applications | — |
| `scripts/probe_virtualbox_armv8_vm.sh` | Headless ARMv8 VirtualBox boot probe: registers a temporary VM, attaches the staged developer-lane VDI, boots VBox EFI, captures `VBox.log`, and confirms firmware-visible AHCI disk attachment before cleanup | — |
| `scripts/run_qemu_armv8_efi_control.sh` | QEMU AArch64 EFI control probe: boots the ARM developer-lane raw image under EDK2, captures serial output, and verifies whether the staged control EFI leaves its execution marker | — |
| `scripts/run_qemu_armv8_guest_probe.sh` | QEMU AArch64 guest bring-up probe: boots the staged ARM guest image under EDK2, requires the serial banner, and inspects the mutated probe image for `efi-ran.txt`, `boot-report.txt`, and the staged startup artifacts, including the consolidated Phase 4 device-layer report | — |
| `scripts/run_qemu_x86_64_guest_probe.sh` | QEMU x86_64 guest bring-up probe: boots the staged `x86_64` guest image under EDK2, requires `efi-ran.txt`, `boot-report.txt`, and `startup-status.txt`, and reuses the shipped `x86_64` handoff validator against the recovered contract files | — |
| `scripts/validate_qemu_armv8_guest_reports.sh` | Shared guest-report validator used by the live QEMU ARM guest probe and a synthetic blocked-state fixture so ready and blocked boot-progress expectations stay on one external contract | — |
| `dev/phase4_startup_snapshot.cpp` | Build-time Phase 4 startup snapshot generator: runs the real VirtualBox guest bootstrap, proves CanonStore recovery plus VMSVGA present and E1000 round-trip through the hosted device seam, and emits the generated `startup-phase4` artifact/header embedded by the ARM EFI developer-lane stub | — |
| `mmu/ternary_page_alloc.hpp/.cpp` | Physical page allocator; balanced-ternary `PageState` {Free=-1, Reserved=0, Allocated=+1}; `alloc_page`, `alloc_contiguous`, `free_page` | 28 |
| `sched/tisc_context.hpp` | `TiscContext` — full TISC thread snapshot; `ThreadState` {Sleeping=-1, Ready=0, Running=+1} | — |
| `sched/context_switch.hpp/.cpp` | `context_save` / `context_restore` / `context_yield` over `t81::vm::ThreadContext` | 43 |

#### Design notes

- Hosted HAL remains the executable path today, but Phase 1 promotion is now encoded as a concrete VirtualBox x86_64 hardware profile rather than a generic future boot target.
- The first RFC-00B3 kernel-integration step is now implemented: `hal_main` no longer terminates at a stub and instead hands off to `axion_kernel_main(...)`, which builds the first kernel runtime state from the validated `BootContext`.
- The first supported VM profile is intentionally narrow: VBox EFI firmware, AHCI storage, E1000 networking, VMSVGA display, and HPET/IOAPIC timing.
- NVMe remains visible in the upstream VirtualBox source tree but is explicitly deferred behind AHCI for the first persistence gate.
- A reproducible VirtualBox disk-artifact target now exists (`t81_ternaryos_virtualbox_guest_artifact`) and stages metadata, demo evidence, a compiled `BOOTX64.obj` stub object, and a local diagnostic `BOOTX64.EFI` candidate for the `x86_64` lane.
- Local boot validation is blocked on this Apple Silicon development host because `VBoxManage list systemproperties` currently reports `Supported platform architectures: ARMv8`, while the roadmap target remains `x86_64`.
- Program split is now explicit: `x86_64` VirtualBox remains the acceptance lane, QEMU AArch64 is the primary local developer lane for observable EFI execution, and ARMv8 VirtualBox is a secondary diagnostic lane for artifact generation and narrow host-specific investigation.
- A temporary ARMv8 developer-lane artifact now exists to keep local VirtualBox iteration moving on Apple Silicon without redefining the official promotion target, and it now includes a compiled `BOOTAA64.obj` EFI-stub object.
- That ARMv8 developer lane now goes one step further locally: a headless VirtualBox boot probe can start VBox EFI on this host, attach the staged VDI through AHCI, and capture a log proving the disk is firmware-visible.
- With `lld` installed locally, the ARMv8 developer lane can now emit a real `BOOTAA64.EFI`; however, that image is still linked through a developer-lane shim rather than the true C++ HAL bridge.
- The ARMv8 artifact layout has been tightened for local debugging: the staged disk now places `STARTUP.NSH` at both `/STARTUP.NSH` and `/EFI/BOOT/STARTUP.NSH`, and the shell script attempts to leave a `TERNOS/startup-ran.txt` marker before chaining to `BOOTAA64.EFI`.
- Even with that stronger shell fallback, current Apple Silicon VirtualBox runs still show no execution evidence: the VM attaches the VDI correctly, but local probes still produce an empty UART log and no `startup-ran.txt` or `efi-ran.txt` markers on the post-boot disk.
- The ARM control experiment is now in place as a stricter discriminator: `BOOTAA64_CTRL.EFI` is staged ahead of the shim-backed app and only attempts to leave `efi-ctrl-ran.txt`, but current probes still show no `startup-ran.txt`, `efi-ctrl-ran.txt`, or `efi-ran.txt` markers and no UART output.
- A separate QEMU AArch64 EFI control probe is now available locally. Under QEMU + EDK2 on this Apple Silicon host, the staged ARM image does execute the EFI control application and leaves `TERNOS/efi-ran.txt`, proving the ARM PE/COFF artifact is viable in an observable AArch64 EFI environment.
- QEMU AArch64 is now also the bring-up lane for the staged ARM guest image itself: the probe path can boot a copied raw image, capture serial output, and inspect the resulting filesystem markers without depending on VirtualBox ARM visibility.
- Current QEMU guest-probe result is stronger than the earlier control-only lane: the staged ARM image reaches `BOOTAA64.EFI` directly (`boot_path_inference=default-bootaa64-efi`), leaves `TERNOS/efi-ran.txt`, writes `TERNOS/boot-report.txt` confirming `platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane`, `hal_main_result=0`, `kernel_boot_ready_slice=complete`, `boot_progress_state=ready`, `boot_progress_pending=false`, `boot_progress_blocked=false`, `boot_progress_source=kernel-boot-critical-policy`, and `boot_validation_lane=qemu-armv8-guest-probe`, emits the visible serial banner `Axion ARMv8 EFI stub`, writes `TERNOS/startup-status.txt` exposing guest-visible Axion state (`phase=5`, `shell_mode=typed-builtins`, `kernel_boot_ready_slice=complete`, `boot_progress_pending=false`, `boot_progress_blocked=false`, `boot_validation_lane=qemu-armv8-guest-probe`, `virtualbox-ahci`, `virtualbox-vmsvga`, `virtualbox-e1000`), writes `TERNOS/startup-shell.txt` from a build-time snapshot generated by the real shell backend, writes `TERNOS/startup-session.txt` from the same backend as a compact `show session` snapshot with live profile and durable-state counts, writes `TERNOS/startup-history.txt` from that backend as a persisted durable-history view, writes `TERNOS/startup-store.txt` as a backend-derived `store ls` inventory snapshot, writes `TERNOS/startup-ref.txt` as a backend-derived `show ref <canonref>` object retrieval snapshot, writes `TERNOS/startup-report.txt` as a consolidated shell/session/object proof surface, and now writes `TERNOS/startup-phase4.txt` as a consolidated Phase 4 device-layer proof showing 20 CanonStore objects recovered across two guest cycles, index overflow past the 17-entry root-header threshold, successful torn-header fallback recovery for the same 20 objects, one VMSVGA present, and a three-frame E1000 workload with successful round-trip recovery through the guest bootstrap seam.
- A synthetic blocked-state guest fixture now exercises that same validator with `boot_progress_state=blocked` and `boot_progress_blocked=true`, so the external lane can prove it distinguishes ready and blocked boot-progress outcomes without requiring a deliberately failing live QEMU boot.
- Program decision: stop escalating local ARMv8 VirtualBox boot-layout experiments, use QEMU AArch64 as the primary local EFI/debug lane, and prepare the official `x86_64` acceptance lane as a handoff package instead; the runbook for that external validation path is now checked in.
- That handoff path is now reproducible as a single build target: the `x86_64` VirtualBox artifact, profile summary, demo transcript, and runbook can be packaged into a tarball for external validation on a real `x86_64` host.
- The official `x86_64` handoff package now also carries `expected-boot-report.txt` and `expected-startup-status.txt`, aligning the acceptance lane with the same boot-progress contract already enforced in the ARM/QEMU developer lane.
- The handoff bundle now also carries `validate_virtualbox_x86_64_handoff.sh`, so an external `x86_64` reviewer can validate recovered guest boot artifacts against that contract directly.
- The handoff bundle now also carries a `recovered-artifacts/` template so the external `x86_64` reviewer returns boot-report/startup-status evidence in one standard layout.
- A synthetic `x86_64` handoff fixture now exercises that validator locally, so the acceptance-lane helper is no longer only documented and shipped; it is also verified against a valid recovered-artifact layout in-tree.
- A second synthetic `x86_64` handoff negative fixture now proves the same helper rejects mismatched boot-progress state with an explicit validation-mismatch failure.
- A packaged-bundle smoke helper plus `t81_ternaryos_virtualbox_x86_64_handoff_bundle_smoke` target now prove the shipped handoff bundle is internally self-consistent using a bundled smoke fixture.
- A packaged-bundle negative smoke helper plus `t81_ternaryos_virtualbox_x86_64_handoff_bundle_negative_smoke` now also prove the shipped handoff bundle rejects a bundled mismatch fixture with an explicit validation failure.
- That closes the current local external-validation packaging lane. The next milestone is external `x86_64` VirtualBox execution and return of real recovered boot artifacts against the shipped contract.
- A new local QEMU x86_64 EFI probe now boots the staged `x86_64` guest image, recovers `efi-ran.txt`, `boot-report.txt`, and `startup-status.txt`, and passes the existing `x86_64` handoff validator unchanged. That gives the official `x86_64` artifact a real local execution lane without claiming VirtualBox acceptance proof.

**Phase 1 test total: 161 / 161**

---

### Phase 2 — Ternary MMU ✅ COMPLETE

**Gate condition (v1.6):** T81VM allocates from ternary page boundaries.
Status: ternary radix page table implemented and tested behind the stable MMU API.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `mmu/tva.hpp` | `TernaryVirtualAddress` as `uint64_t` base-3 number; `kPageSize=59049` (3¹⁰), `kMaxVpn=3²⁰−1`, `kMaxTva=3³⁰−1`; `tva_vpn`, `tva_offset`, `tva_from_vpn_offset`, `tva_valid`, `trit_at`, `trit_weight`, `tva_to_string` | 22 |
| `mmu/page_table.hpp/.cpp` | 20-trit ternary radix page table with a 3-way branch per trit; permission-aware `mmu_map`, checked read/write/exec translation, `mmu_translate`, `mmu_unmap`, `page_table_dump`, `page_table_stats`, `page_table_trace` | 50 |

**Address space design (RFC-00B1 §2):**
- Page offset: lower 10 trits → 3¹⁰ = 59,049 positions within a page
- VPN: next 20 trits → 3²⁰ = 3,486,784,401 virtual pages
- Total virtual space: 3³⁰ ≈ 205 TB (fits in `uint64_t`)
- Binary↔ternary gap: "narrow virtual" strategy — physical addresses stay as plain `uint64_t`; ternary structure lives entirely in the virtual address

Additional Phase 2 notes:
- Page-table leaves now carry read/write/execute permission bits.
- `mmu_translate_checked()` classifies invalid TVA, unmapped access, and permission-denied faults over the radix walk.
- `page_table_dump()` and `page_table_trace()` now expose per-leaf permission state.

**Phase 2 test total: 87 / 87**

---

### Phase 3 — Kernel Scheduling & IPC ✅ COMPLETE

**Gate condition (v1.7):** Two concurrent TISC threads run deterministically.
Status: all deliverables implemented and passing; 193 assertions green.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `sched/run_queue.hpp/.cpp` | 81-slot round-robin run queue; `add_thread`, `remove_thread`, `next_ready`, `set_running`, `preempt_running`, `sleep_thread`, `wake_thread` | 102 |
| `sched/scheduler.hpp/.cpp` | `Scheduler::tick()` — save current → preempt → next_ready → set_running → restore; `spawn`, `terminate`, `sleep`, `wake` | 18 |
| `ipc/canon_message.hpp/.cpp` | `CanonMessage` (sender + `CanonRef` + payload + tag); `MessageBus` (per-Tid inbox deques, depth ≤ 81, FIFO, peek) | 73 |

#### Design notes

- `Scheduler::tick()` returns `false` when only one thread exists (same-thread re-schedule detected); returns `true` only on a genuine context switch.
- Round-robin is deterministic: `rr_index_` advances through `slots_` in insertion order; 81-slot cap mirrors Hanoi scheduler.
- `CanonRef` handles (not raw pointers) cross IPC boundaries, preserving CanonFS audit trail invariants.
- OQ-5 (Axion determinism under pre-emption) — **closed by RFC-00C3**: governance audit ring (`FsGovRecord`, 16 slots) records every IRQ-driven wake (`kGovTimerDeviceWake`) and every async ERET (`kGovAsyncContextSwitch`); `obs_seq_at` cross-references gov events against the KernelCall obs ring timeline; Phase 14 CI gate validates completeness.
- RFC-00C4 (per-device wake filtering) — **implemented (Phase 15)**: `WaitForDevice` wire request extended to 16 bytes with `device_id` discriminator; `fs_sched_timer_device_wake(intid)` filters `BlockedDeviceWait` threads by `device_id == 0 || device_id == intid`; `FsGovRecord` extended to 24 bytes recording `device_id`; `fs_gov_find_device(tid, event, device_id)` enables precise gov ring lookup; Process E (tid=6, LBA 11) validates with `device_id=30` (GICv3 PPI30); Phase 15 CI gate: `[axion] el0: device filter OK (device_id=30, tid=6)`.
- RFC-00C5 (concurrent device wait) — **implemented (Phase 16)**: two threads (tid=6/did=30 exact, tid=7/did=0 wildcard) park on `WaitForDevice` simultaneously using two independent EL0 code+stack page pairs; single timer IRQ wakes both via `fs_sched_timer_device_wake(30)`; `fs_sched_exit_thread` chains the two ExitThread returns without EL1 re-entry; MMU extended with `proc_code_page2` + `proc_stack_page2`; `el0_tva_valid` covers all six pages; Phase 16 CI gate: `[axion] el0: concurrent wake OK (device_id=30, tid=6+7)`.
- RFC-00B3 now defines the active integration path: the first kernel-owned runtime entry exists, and the kernel now consumes checked MMU translation through a kernel-facing fault/reporting path instead of leaving the runtime bootstrap as a thin handoff.
- The first persistent kernel runtime state object now exists: allocator, page table, scheduler, IPC bus, and fault log are now owned by kernel runtime state seeded from `BootContext`.
- Active device arbitration for the first supported VirtualBox storage/display/network profile is now attached to that same owned boundary.
- The kernel runtime now also exposes a deterministic loop step plus runtime counters for scheduler and IPC activity.
- The loop-owned fault delivery path now feeds a real kernel policy boundary: delivered faults are routed into per-thread runtime state, the faulting thread is quarantined deterministically, the owning process group enters a blocked fault state, explicit group acknowledgement gates recovery, and audit-only governance events are recorded in deterministic order.
- The RFC-00B5 interrupt summary-convergence slice is now implemented too: kernel-owned interrupt events can be recorded explicitly, delivered deterministically through the kernel loop after faults and before pager work, and surfaced through stable runtime/fault/audit diagnostics with queue-state visibility, per-source accounting, latest interrupt-audit metadata, and record-level intake/delivery audit provenance.
- Interrupt queue observability now exists on top of that first slice too: the runtime retains latest interrupt intake provenance, pending-interrupt high-water mark, and FIFO head visibility through stable runtime/fault/audit summaries.
- Deterministic source accounting now exists too for the current interrupt classes: timer, storage, network, keyboard, and unknown record/deliver counts flow through the same stable runtime/fault/audit summaries.
- Pending interrupt queue composition is now exposed too: runtime and fault summaries retain queued counts for the current interrupt source classes while preserving FIFO semantics.
- Pending interrupt queue bounds are now exposed too: runtime and fault summaries retain both head and tail pending interrupt visibility while preserving the same FIFO event model.
- Stable interrupt-audit correlation now exists too: runtime, fault, and audit summaries retain the latest interrupt-delivery audit sequence directly.
- Audit-summary interrupt queue alignment now exists too: audit summaries retain pending interrupt counts, pending source composition, and FIFO head/tail visibility alongside interrupt history.
- Audit-summary interrupt accounting is now aligned too: audit summaries expose the aggregate recorded interrupt count alongside delivered counts and per-source counters.
- Interrupt intake is now audit-visible too: the kernel records `InterruptRecorded` audit events when interrupt inputs enter the owned FIFO, so intake and delivery are both traceable.
- Stable interrupt-intake audit correlation now exists too: runtime, fault, and audit summaries retain the latest `InterruptRecorded` audit sequence directly.
- Latest interrupt audit kind retention now exists too: runtime, fault, and audit summaries expose whether the newest interrupt-related audit event was intake or delivery.
- Interrupt records now retain intake provenance too: pending and delivered `KernelInterruptRecord` values carry their own `InterruptRecorded` audit sequence directly.
- Delivered interrupt records now retain delivery provenance too: `KernelInterruptRecord` values carry their own `InterruptDelivered` audit sequence once the loop dispatches them.
- Stable latest delivery correlation now exists too: runtime, fault, and audit summaries retain the latest `InterruptDelivered` audit sequence directly even after newer intake events occur.
- Latest interrupt audit target retention now exists too: runtime, fault, and audit summaries expose which interrupt sequence the newest interrupt-related audit event refers to.
- Latest interrupt audit source retention now exists too: runtime, fault, and audit summaries expose the interrupt source class targeted by the newest interrupt-related audit event.
- Latest interrupt audit payload retention now exists too: runtime, fault, and audit summaries expose the payload carried by the newest interrupt-related audit event.
- Latest interrupt audit timestamp retention now exists too: runtime, fault, and audit summaries expose the timestamp carried by the newest interrupt-related audit event.
- The first service-facing runtime contract above the supervisor/process-group boundary is now implemented, including deterministic healthy/faulted-group behavior and stable diagnostics.
- The first narrow service-facing runtime action is now implemented too: supervisor fault-group acknowledgement through the same contract.
- Supervisor-facing recovery/report flows are now implemented on top of that action: the service boundary exposes deterministic pending-group, acknowledgement, and recovered-group state for each supervisor.
- A second narrow service-facing action is now implemented too: deterministic device claim/release requests through the same contract, with healthy-vs-faulted group enforcement.
- The current internal boot-ready kernel slice is now closed above the new kernel-owned process-memory ownership, pager-needed fault state, internal handoff queue, internal resolution state, first worker model, duplicate-fault coalescing, retained backlog/load diagnostics, retained receipt identities/ordinals, active-work handoff ordinals, queued-head identities/ordinals, the bounded ready-bypass parking rule, retained ready-bypass deferral diagnostics, retained parked-cycle and parked-episode diagnostics, live parked-ready backlog diagnostics, retained parked-resumption diagnostics, retained parked-resume backlog diagnostics, retained parked-resume handoff diagnostics, retained parked-resumed-head handoff diagnostics, retained parked-resolved-head diagnostics, retained parked-resolved remaining-work diagnostics, retained parked-resolution follow-on diagnostics, retained parked-resolution successor completion diagnostics, retained blocker/blocked identities, retained stall ordinals, retained blocked-side stall ordinals, retained blocked-side backlog depth, retained activation identities/ordinals, retained completion identities/ordinals, the terminal parked-head failure policy, the boot-critical pager auto-resolution policy, and the new explicit boot-progress/fail reporting. The local external boot-lane packaging/validation slice is now closed as well; the next step is real external `x86_64` host execution, as tracked in `kernel_execution_plan.md`.

**Phase 3 test total: 193 / 193**

---

### Phase 4 — Device Drivers & I/O 🟡 IN PROGRESS

**Gate condition (v2.0):** CanonFS read/write survives a reboot cycle.
Status: hosted simulation primitives implemented and passing; bare-metal/NVMe promotion still open.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `dev/block_device.hpp` | Abstract 729-byte CanonBlock-aligned block device interface (`IBlockDevice`, `BlockDeviceInfo`) | — |
| `dev/hosted_block_dev.hpp/.cpp` | File-backed hosted block device; read/write/flush/save/load for reboot simulation | 15 |
| `dev/virtualbox_ahci_dev.hpp/.cpp` | VirtualBox-first AHCI block-device adapter scaffold over an existing `IBlockDevice`; ABAR/IRQ metadata + op accounting | 16 |
| `dev/virtualbox_e1000_dev.hpp/.cpp` | VirtualBox-first E1000 NIC scaffold with MMIO/IRQ/MAC metadata, frame TX/RX queues, and ternary-packet send/receive helpers | 18 |
| `dev/virtualbox_vmsvga_dev.hpp/.cpp` | VirtualBox-first VMSVGA display scaffold with MMIO/IRQ metadata, ternary framebuffer ownership, and present-to-ASCII capture for hosted simulation | 13 |
| `dev/canon_store.hpp/.cpp` | Content-addressed CanonBlock store over `IBlockDevice`; dedup, flush, rebuild, corruption detection | 29 |
| `dev/framebuffer.hpp/.cpp` | 81×27 ternary framebuffer with ASCII dump | 18 |
| `dev/ttf.hpp/.cpp` | Minimal Ternary Text Format codec + framebuffer text renderer for ASCII terminal output | 12 |
| `dev/net_packet.hpp` | Ternary Ethernet packet wrapper with payload validation, canonical content hash, and binary frame encode/decode | 18 |
| `apps/demo.cpp` | Presentation demo: VirtualBox guest bootstrap over hosted storage binding, reboot-persistent CanonStore, TTF framebuffer output, and Ethernet frame round-trip | — |
| `tests/device_driver_test.cpp` | Phase 4 acceptance tests AC-D1 through AC-D8 plus VirtualBox AHCI/E1000/VMSVGA adapter scaffolds, hosted TTF rendering, Ethernet frame translation checks, repeated guest-bootstrap reboot persistence, recovery after index-header and payload corruption, multi-block CanonStore persistence beyond the 17-entry root-header threshold, torn-header fallback recovery, and interrupted-flush durability through the guest path | 342 |

#### Design notes

- Logical block size is fixed at 729 bytes (one CanonBlock / 3^6 trytes), keeping device I/O aligned with CanonFS primitives.
- `CanonStore` still roots its metadata at LBA 0 (`CST1`), but overflow index blocks now spill into the tail of the device so the data region can continue growing contiguously from LBA 1 upward.
- `HostedBlockDev::save()` / `load()` provides the current reboot-cycle simulation path for the v2.0 gate.
- `ttf_encode_ascii()` / `ttf_render_text()` now provide a minimal hosted TTF terminal path over the ternary framebuffer.
- The network layer now translates between ternary packet structs and a binary Ethernet-like frame format, and the first hosted RX/TX path is modeled through the VirtualBox E1000 scaffold.
- AHCI is now represented as a first-target VirtualBox adapter scaffold, and the HAL-side VirtualBox profile can bind its storage path through that adapter while still delegating to hosted storage underneath.
- E1000 is now represented as a first-target VirtualBox NIC scaffold, and the HAL-side VirtualBox profile can bind its guest network path through that adapter while still using hosted loopback queues underneath.
- VMSVGA is now represented as a first-target VirtualBox display scaffold, and the HAL-side VirtualBox profile can bind its framebuffer output through that adapter while still presenting to an ASCII capture in hosted simulation.
- The demo path now boots through the VirtualBox guest bootstrap for storage, display, and network, showing the same hosted story through the VM-targeted HAL/device seam rather than through manually assembled hosted device objects.
- The Phase 4 persistence proof is now stronger than a raw `CanonStore` reboot cycle alone: the device-driver suite also verifies that the same CanonRef survives across two hosted reboot cycles when storage is reached through the VirtualBox guest bootstrap and AHCI-shaped binding.
- The local QEMU developer lane now carries a Phase 4 guest artifact too: `startup-phase4.txt` is derived from the real guest bootstrap path and proves AHCI-backed CanonStore recovery for 20 persisted objects across two guest cycles, active overflow metadata beyond the 17-entry root header, successful torn-header recovery for those same objects, a mutable three-present VMSVGA framebuffer cycle, and a two-batch five-frame E1000 send/inject/receive workload under the staged ARM guest.
- The hosted recovery story is stronger too: when the persisted CanonStore header is damaged and one payload block is corrupted, the guest-bootstrap path now proves `rebuild_index()` falls back to scanning AHCI-backed payload blocks, preserves the intact CanonRef, and rejects the stale pre-corruption CanonRef for the damaged payload.
- The old 17-entry single-block threshold is no longer the effective store limit: the hosted suite now pushes CanonStore through multiple metadata blocks behind the VirtualBox bootstrap, flushes, reboots, rebuilds, and verifies every stored CanonRef survives intact.
- Interrupted persistence is covered more directly now: when the header keeps its magic but advertises an impossible entry count, the guest-bootstrap path rejects the torn index and still recovers every intact payload block by rescanning the AHCI-backed data region.
- Threshold crossing is covered directly too: the suite now proves CanonStore can persist and rebuild entries that spill past the root header into tail-resident overflow metadata blocks without changing the guest-bootstrap seam.
- Guest-path durability semantics are now explicit under transient failure: if `flush()` fails after new blocks are written in memory, reboot still exposes only the last durable state, and a later successful retry makes the pending CanonRef durable.
- Real NVMe/ethernet hardware adapters remain open.

**Phase 4 test total: 342 / 342**

---

### Phase 5 — Userland Ecosystem 🟡 STARTED (early v2.x scaffold)

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `shell/shell_session.hpp/.cpp` | Shared Phase 5 shell-session backend: executes a minimal typed shell command model (`help`, `profile`, `name set <label> <ref>`, `name ls`, `object pin <kind> <name> <ref>`, `object ls`, `object show <name>`, `show profile`, `session status`, `session checkpoint`, `session export`, `session import <ref>`, `session diff <ref>`, `session run <ref>`, `session show durable`, `show session`, `session refs`, `store put <text>`, `store put script <line>|<line>|...>`, `store put ref <ref>`, `store cp <ref>`, `store ls`, `store get <ref>`, `show ref <canonref>`, `store rm <ref>`, `history`, `history show session`, `history show object <ref>`, `history use <ref>`, `history show durable`, `clear`) through the VirtualBox guest-bootstrap path, persists shell history through CanonStore, reboots, recovers it, and exposes deterministic transcript/framebuffer state to frontends | — |
| `apps/shell_demo.cpp` | Verbose Phase 5 backend proof: prints the built-in shell session and framebuffer evidence directly to stdout | — |
| `apps/shell_tui.cpp` | First FTXUI-based TernOS shell frontend with transcript pane, command buffer, session pane, builtins pane, framebuffer preview, interactive typed command execution, and `--snapshot` render mode | — |
| `shell/shell_startup_snapshot.cpp` | Build-time Axion shell snapshot generator: runs the real Phase 5 shell backend, emits compact startup-shell, startup-session, startup-history, startup-store, startup-ref, and startup-report artifacts, and produces the generated headers embedded by the ARM EFI developer-lane stub | — |
| `tests/shell_session_test.cpp` | Phase 5 shell acceptance test: scripted durable-history path plus interactive typed-command execution, session-checkpoint/export/import/diff/run persistence, durable-anchor inspection, quoted payload parsing, durable script objects via `store put script <line>|<line>|...>`, canonical object composition via `store put ref <ref>` and `store cp <ref>`, session-history inspection, object-history lookup via `history show object <ref>`, durable-anchor rebinding via `history use <ref>`, session-local named refs via `name set <label> <ref>` / `name ls`, typed pinned objects via `object pin <kind> <name> <ref>` / `object ls` / `object show <name>`, `@label` resolution across ref-taking commands, store ref listing, store ref retrieval, durable ref removal, explicit durable-history inspection, transcript clearing, and parse-error handling through the guest-bootstrap seam | 183 |
| `docs/axion_shell_design.md` | Phase 5 shell design note: positioning, grammar, screen model, and milestone sequence for evolving the current built-in shell into a real Axion shell | — |

#### Design notes

- Phase 5 is now started, but still only as a narrow hosted scaffold.
- The current shell path is intentionally narrow: it now supports interactive typed command execution in the TUI, but it still does not execute TISC userland and does not introduce a syscall surface.
- Its value is architectural: user-facing output now rides the same guest-bootstrap storage and display seams already proven in Phase 4.
- The shell UI now uses the repo's established FTXUI stack instead of a one-off terminal surface, and `--snapshot` still gives it a deterministic review/debug mode.
- The old fixed transcript is gone: the current shell transcript now comes from command handlers for `help`, `profile`, `name set <label> <ref>`, `name ls`, `object pin <kind> <name> <ref>`, `object ls`, `object show <name>`, `show profile`, `session status`, `session checkpoint`, `session export`, `session import <ref>`, `session diff <ref>`, `session run <ref>`, `session show durable`, `show session`, `session refs`, `store put <text>`, `store put script <line>|<line>|...>`, `store put ref <ref>`, `store cp <ref>`, `store ls`, `store get <ref>`, `show ref <canonref>`, `store rm <ref>`, `history`, `history show session`, `history show object <ref>`, `history use <ref>`, `history show durable`, and `clear`.
- The live TUI now accepts typed commands directly, including quoted `store put` payloads, and the shell test target now covers scripted durability, interactive typed command execution, session-state reporting, object-native session/ref inspection, shell-local durable ref inspection, named CanonRef aliases, explicit durable-history inspection, store ref inspection/removal, transcript clearing, and parser errors.
- The shell state now distinguishes the local session window from durable CanonStore state instead of treating them as one undifferentiated history stream, and the command surface now reflects that split explicitly.
- The shell now has its first object-native read/write grammar: `show profile`, `show session`, `show ref <canonref>`, `session show durable`, `session export`, `session import <ref>`, `session diff <ref>`, `session run <ref>`, `history show session`, `history show object <ref>`, `history use <ref>`, `store put script <line>|<line>|...>`, `store put ref <ref>`, and `store cp <ref>`, plus session-local naming with `name set <label> <ref>` / `name ls` and typed pinned objects with `object pin <kind> <name> <ref>` / `object ls` / `object show <name>`, so ref-taking commands can use `@label`.
- The next real Phase 5 milestone is moving from narrow built-ins to a broader user model while preserving the same durable-history and object-ref semantics.

---

## Test Summary

| Test binary | Assertions | Phase |
| :--- | :---: | :---: |
| `t81_ternaryos_hal_boot_test` | 2765 | 1 |
| `t81_ternaryos_page_alloc_test` | 28 | 1 |
| `t81_ternaryos_context_switch_test` | 43 | 1 |
| `t81_ternaryos_mmu_test` | 87 | 2 |
| `t81_ternaryos_scheduler_test` | 120 | 3 |
| `t81_ternaryos_ipc_test` | 73 | 3 |
| `t81_ternaryos_device_driver_test` | 342 | 4 |
| `t81_ternaryos_shell_session_test` | 183 | 5 |
| **Total** | **3641** | |

Run all TernOS tests:

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build -R ternaryos -V
```

---

## Open Questions / Risks

| # | Question | Blocking |
| :-- | :--- | :--- |
| OQ-1 | Guest artifact format is now narrowed to a GPT EFI-partitioned raw disk + VBox VDI wrapper; EFI linking now works for the ARMv8 developer lane, and QEMU AArch64 proves the ARM EFI control app executes, but actual VirtualBox ARM boot selection / handoff remains unproven | Phase 1 promotion |
| OQ-2 | First supported VirtualBox device profile is intentionally narrow (VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC); implementation is now scaffolded, but host/target architecture mismatch still blocks acceptance-lane local boot proof | Phase 1 promotion |
| OQ-3 | CI target now leans toward QEMU for local ARM automation, with VirtualBox reserved for acceptance/demo validation and host-specific diagnostics | Phase 1 promotion |
| OQ-4 | TISC interrupt semantics are now architecturally narrowed by RFC-00B5: no trap-return opcode is required in the current Axion path, but later priority, nesting, and audit-surface choices remain open | Phase 4 |
| ~~OQ-5~~ | ~~Axion determinism under pre-emption — governance model must be extended for async context switches~~ | **Closed — RFC-00C3** |
| OQ-7 | Real AHCI / E1000 / VMSVGA-facing adapters are not implemented; Phase 4 currently satisfies the gate only in hosted simulation despite the new guest-artifact packaging target | Phase 4 promotion |
| OQ-8 | CanonStore now scales past the root header via tail-resident overflow metadata blocks, but compaction/rewrite strategy for long-lived stores is still deferred | Phase 4 scaling |
| OQ-9 | VirtualBox should remain a tactical promotion target, not a permanent HAL dependency; portability boundaries must stay explicit | Cross-phase portability |

---

## Promotion Checklist (per layer)

When any layer is ready to graduate from `experimental/` to the mainline:

- [ ] All RFC acceptance criteria met
- [ ] CI gate added (ctest + determinism check)
- [ ] Sources moved to `include/t81/` + `src/` (headers) and `runtime/` (impl)
- [ ] CMake option removed; target added to default build
- [ ] Axion policy extended if new syscall surface introduced
- [ ] Spec impact assessed (ISA freeze must not be broken)
| `kernel/kernel_abi.hpp/.cpp` | Typed `axion_kernel_call(...)` boundary, capability enforcement, service/supervisor ABI dispatch, and rejection taxonomy | 206 |
| `kernel/kernel_abi_wire.hpp/.cpp` | Fixed-size canonical request/response wire blocks, wire encode/decode, pointer-style wire dispatch, and raw byte-span ABI bridge | 45 |
