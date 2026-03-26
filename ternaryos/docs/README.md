# experimental/ternaryos

**Status:** Governed experimental kernel track overall; RFC-00B9 user-environment slice is governed non-DCP and now has a stable public boundary under `include/t81/axion/` and `src/axion/` in the opt-in TernaryOS build.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**Review Summary:** [review_summary.md](review_summary.md)
**Architecture Audit:** [kernel_architecture_audit.md](kernel_architecture_audit.md)
**x86_64 Handoff:** [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
**Shell Design:** [axion_shell_design.md](axion_shell_design.md)
**Kernel Plan:** [kernel_execution_plan.md](kernel_execution_plan.md)
**Engineering Plan:** [kernel_engineering_follow_on_plan.md](kernel_engineering_follow_on_plan.md)
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [spec/rfcs/RFC-00B0-hal-spec.md](../../../spec/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [spec/rfcs/RFC-00B1-ternary-mmu.md](../../../spec/rfcs/RFC-00B1-ternary-mmu.md)
**RFC-00B2 (Drivers):** [spec/rfcs/RFC-00B2-device-drivers.md](../../../spec/rfcs/RFC-00B2-device-drivers.md)
**RFC-00B3 (Kernel):** [spec/rfcs/RFC-00B3-axion-kernel-architecture.md](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md)
**RFC-00B6 (Kernel ABI):** [spec/rfcs/RFC-00B6-minimal-syscall-capability-boundary.md](../../../spec/rfcs/RFC-00B6-minimal-syscall-capability-boundary.md)
**RFC-00B5 (Interrupts):** [spec/rfcs/RFC-00B5-governed-event-interrupt-model.md](../../../spec/rfcs/RFC-00B5-governed-event-interrupt-model.md)

Prototype implementation of Axion, the current working name for the ternary-native
OS kernel on the T81VM runtime. Phases 1 through 3 are complete, Phase 4
device-driver work is in progress, and Phase 5 now has a first typed shell/TUI
path on top of the hosted guest-bootstrap path.

Kernel integration direction is now tracked explicitly in RFC-00B3, while
governed interrupt semantics are now tracked in RFC-00B5. Together, those RFCs
define the current path for turning the existing HAL/MMU/scheduler/IPC
subsystems into one kernel-owned runtime unit instead of letting them grow
organically. The first kernel-owned handoff path now exists: `hal_main`
transfers control to `axion_kernel_main(...)`, and the HAL suite verifies that
runtime bootstrap.

The kernel implementation is no longer centered in one translation unit. The
runtime coordinator now lives in `kernel_runtime.cpp`, while faults,
interrupts, pager policy, lifecycle/bootstrap, service queries/actions, views,
and shared runtime utilities have been split into dedicated implementation
files. `kernel_main.hpp` remains the shared runtime contract for those units.

RFC-00B6 is now partially implemented rather than only proposed. The current
typed `axion_kernel_call(...)` ABI covers thread identity/spawn/termination,
same-supervisor execution control, named thread entry registration/spawn,
CanonRef-backed executable object registration/query/spawn, mapped-memory
executable registration from caller TVA, kernel-owned published executable
objects, service-owned
thread entry spawn, IPC, fault inbox/recovery, capability management, service
lifecycle control, process-group memory inspection,
boot-critical address-space control, guarded runtime/fault summary queries,
kernel-issued capability record IDs, supervisor capability transition
history, sequence-based capability revocation, and explicit provenance
distinguishing kernel-seeded capabilities from supervisor-delegated grants.
Spawn requests now also accept a compact initial thread descriptor
(`pc`/`sp`/`register0`/`label` plus halted/active state) instead of always
default-constructing blank scheduler contexts, and services can now retain
that descriptor during `RegisterService` for later execution through
`SpawnThreadForService`. Service status and supervisor service-inventory views
now also expose that stored entry descriptor in the typed control plane, and
service register/query responses now carry it through the fixed-size wire and
hosted C ABI paths too. Supervisor-scoped service inventory is now also
reachable through the typed, wire, and hosted C ABI layers, with bounded
service entries that preserve stored service entry-descriptor state.
Process groups can now also register CanonRef-keyed executable objects with a
stored spawn descriptor, query that executable state later, and spawn new
threads from it through the same typed, wire, and hosted C ABI layers. Wire
and hosted C executable register/query/spawn responses now also carry that
stored executable entry descriptor, not only the CanonRef identity.
Executable registration now validates a canonical `CanonExec` block against
the supplied `CanonRef`, so executable records are no longer only synthetic
hash/descriptor pairs.
Executable objects can now also be registered from mapped caller memory
through `RegisterExecutableObjectFromTva`, so the kernel can ingest a real
`CanonExec` block over the address-space boundary.
The kernel can now also retain published executable objects in a kernel-owned
`CanonStore`-backed repository and register them later by `CanonRef` alone.
That repository is now bindable to an external `IBlockDevice`, so the
published-object lane can persist across a fresh kernel bootstrap instead of
remaining only private in-memory runtime state.
The kernel can now also resolve published executable objects directly from a
persistent CanonFS root when one is bound, so `RegisterExecutableObject` no
longer depends only on the block-device-backed repository path.
In the hosted lane, Axion now follows the same `T81_CANONFS_ROOT` convention
used elsewhere in the repo and auto-attaches that persistent CanonFS source at
kernel bootstrap when the environment variable is set.
Axion can now also adopt the VirtualBox guest storage binding directly as its
published executable store, so executable publication and later registration by
`CanonRef` can round-trip through the guest AHCI-shaped storage path too.
Services can now also bind to one of those registered executable objects by
CanonRef during `RegisterService`, and the service register/query/spawn paths
now report that same backing executable object identity.
Supervisors can now also inspect one managed service directly through a
dedicated service-status query path, even when the ordinary service query is
deferred by unhealthy state.
Execution reads now also include `QueryThreadExecutionState`, which exposes the
seeded scheduler context for the caller or a same-supervisor target thread.
Delegated capability control now also includes bulk revocation and direct
delegated-capability queries by delegator provenance, plus compact supervisor
delegation summaries. That typed ABI now also has a real canonical transport
layer:
- `kernel_abi_wire.hpp/.cpp` define fixed-size request/response wire blocks
- `axion_kernel_call_wire(...)` dispatches those blocks
- `axion_kernel_call_wire_bytes(...)` provides the raw byte-span bridge
- `axion_kernel_call_wire_tva(...)` now dispatches those same fixed blocks
  through mapped TVA request/response pages owned by the currently running
  caller address space
- `ternaryos_kernel_bootstrap_c(...)` / `ternaryos_kernel_destroy_c(...)`
  provide opaque C runtime-handle lifecycle
- `ternaryos_kernel_call_c(...)` exports the first hosted C-style
  `kernel_call(req, len, resp, len)` entrypoint
- `ternaryos_kernel_call_tva_c(...)` exports the first hosted mapped-address
  bridge over that transport, with caller address-space resolution derived
  from runtime execution context
- the mapped-TVA bridge now preflights request/read and response/write spans
  explicitly before dispatch
- invalid request spans now return a structured wire response with
  `InvalidAddressSpaceSpan` plus an MMU fault record when the response span is
  valid
- invalid response spans still fail the bridge outright

Current working release label: `Axion v0.1.0-alpha`

For now, internal paths, namespaces, CMake targets, and test names still use
`ternaryos` while the external/project-facing name shifts to `Axion`.

The RFC-00B9 user-environment and shell public surface is no longer centered
only inside `experimental/ternaryos/`. The supported boundary now lives at:

- `include/t81/axion/userenv/`
- `include/t81/axion/shell/`
- `src/axion/userenv/`
- `src/axion/shell/`

The matching headers under `experimental/ternaryos/userenv/` and
`experimental/ternaryos/shell/` remain as compatibility shims while the
broader kernel, HAL, MMU, scheduler, IPC, device, and guest-artifact lanes
stay in `experimental/ternaryos/`.

Naming rule for now:
- `T81 Foundation` is the umbrella project/ecosystem
- `T81VM` is the ternary runtime/execution substrate
- `Axion` is the operating system
- existing subsystem names like `CanonFS` and `TISC` stay as-is

## Structure

```
apps/
  demo.cpp              Phase 4 hosted presentation demo
  shell_demo.cpp        Phase 5 verbose shell backend proof
  shell_tui.cpp         Phase 5 FTXUI shell frontend with snapshot mode

docs/
  README.md             Entry point, structure, build/run guidance
  PROGRESS.md           Phase-by-phase implementation log
  review_summary.md     Reviewer-facing current-state summary
  kernel_architecture_audit.md Formal architectural audit and maturity assessment
  axion_shell_design.md Phase 5 shell design note
  kernel_execution_plan.md Short next-step kernel execution plan
  kernel_engineering_follow_on_plan.md Next-step refactor and delivery plan from the audit
  virtualbox_x86_64_handoff.md External x86_64 VirtualBox runbook

  hal/
  hal.hpp              HAL public interface (MemoryRegion, HardwareInterrupt,
                       BootContext, hal_main)
  hal_c_abi.h/.cpp     C ABI bridge for freestanding guest stubs and the
                       first exported hosted kernel lifecycle + `kernel_call`
                       bridge
  hal_main.cpp         Ethics-first boot (Θ₁–Θ₉ via Axion) → kernel-owned handoff
  interrupt_table.cpp  Shadow binary interrupt dispatch table; current
                       governed interrupt bridge tracked by RFC-00B5
  hosted_stub.cpp      Hosted (macOS/Linux) simulation — stand-in for UEFI stub
  virtualbox_platform.hpp/.cpp  First-target VirtualBox VM profile scaffold
  virtualbox_guest_devices.hpp/.cpp  VirtualBox profile-to-device binding helpers
  virtualbox_efi_stub.c  Freestanding VBox EFI stub source for BOOTX64 handoff
  virtualbox_armv8_efi_stub.c  Freestanding VBox EFI stub source for BOOTAA64 handoff
  virtualbox_armv8_efi_control.c  Minimal ARMv8 EFI control app for execution probes
  virtualbox_armv8_efi_shim.c  Temporary ARMv8 developer-lane EFI link shim

kernel/
  kernel_abi.hpp/.cpp  Typed kernel-call ABI, capability checks, service/
                       supervisor dispatch, named entry registration/spawn,
                       and service-owned entry execution
  kernel_abi_wire.hpp/.cpp Fixed-size canonical request/response blocks,
                       wire encode/decode, pointer-style wire dispatch,
                       and raw byte-span ABI bridge
  kernel_main.hpp + kernel_runtime.cpp First Axion kernel-owned runtime entry/bootstrap;
                       runtime-owned allocator/MMU/scheduler/IPC/device state,
                       process-group fault policy, audit-only governance hooks,
                       and a narrow service runtime lifecycle with deterministic
                       register/unregister/suspend/resume actions plus
                       audit-visible lifecycle transitions and supervisor
                       inventory/supervisor-status/recovery/fault/runtime/audit/device/service lifecycle metadata,
                       explicit kernel-owned address-space state, and an
                       internal pager worker with retained backlog/load,
                       receipt, active-work, queued-head, bounded ready-bypass,
                       parked capped deferral, parked-cycle, parked-episode,
                       parked-ready backlog, parked-resumption, parked-resume
                       backlog, parked-resume handoff, parked-resumed-head
                       handoff, parked-resolved-head,
                       parked-resolved remaining-work,
                       parked-resolution follow-on,
                       parked-resolution successor completion,
                       blocker/blocked, activation, completion, and terminal
                       parked-head diagnostics, plus boot-critical auto-resolution
                       and explicit boot-progress/fail diagnostics

mmu/
  tva.hpp              Ternary Virtual Address: base-3 uint64_t, VPN + offset,
                       kPageSize=59049, kMaxTva=3^30-1, trit utilities
  ternary_page_alloc.hpp/.cpp  Physical page allocator (balanced-ternary PageState)
  page_table.hpp/.cpp  20-trit ternary radix page table; permission-aware
                       mmu_map/translate/unmap with checked read/write/exec
                       translation plus page_table_stats/page_table_trace
                       diagnostics

sched/
  tisc_context.hpp     TiscContext: full TISC thread snapshot for pre-emption
  context_switch.hpp/.cpp  context_save / context_restore / context_yield
  run_queue.hpp/.cpp   81-slot deterministic run queue
  scheduler.hpp/.cpp   Round-robin scheduler over TISC contexts

ipc/
  canon_message.hpp/.cpp  CanonRef-safe FIFO message bus

dev/
  block_device.hpp     CanonBlock-aligned block device interface
  hosted_block_dev.hpp/.cpp  File-backed hosted block device
  virtualbox_ahci_dev.hpp/.cpp  VirtualBox-first AHCI adapter scaffold
  virtualbox_e1000_dev.hpp/.cpp  VirtualBox-first E1000 adapter scaffold
  virtualbox_vmsvga_dev.hpp/.cpp  VirtualBox-first VMSVGA adapter scaffold
  canon_store.hpp/.cpp Content-addressed CanonBlock store + reboot rebuild
  framebuffer.hpp/.cpp 81x27 ternary framebuffer with ASCII dump
  ttf.hpp/.cpp         Minimal ASCII ↔ balanced-ternary text codec + renderer
  net_packet.hpp       Ternary Ethernet packet wrapper + binary frame codec

shell/
  shell_session.hpp/.cpp Phase 5 shell session backend over guest bootstrap
  shell_startup_snapshot.cpp Build-time shell snapshot generator for ARM EFI

tests/
  shell_session_test.cpp     Phase 5 shell command / durable-history test
  hal_boot_test.cpp          Phase 1 / kernel integration — 693 assertions
  ternary_page_alloc_test.cpp Phase 1 — 28 assertions
  context_switch_test.cpp    Phase 1 — 43 assertions
  mmu_test.cpp               Phase 2 — 87 assertions
  scheduler_test.cpp         Phase 3 — 120 assertions
  ipc_test.cpp               Phase 3 — 73 assertions
  device_driver_test.cpp     Phase 4 — 342 assertions
```

## Build & Test

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R ternaryos -V
# Expected: 3606/3606 assertions, 8/8 tests pass
```

## Demo

For a short presentation-oriented walkthrough:

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build --target t81_ternaryos_demo
./build/t81_ternaryos_demo
```

The demo shows a VirtualBox-first hosted simulation path:

- the HAL boots a first-target VirtualBox guest profile
- the guest profile binds its first storage, network, and display devices through AHCI, E1000, and VMSVGA wrappers
- CanonStore persists a CanonBlock across a simulated reboot through that binding
- CanonStore metadata now scales past the root 17-entry header and still rebuilds correctly after reboot
- interrupted flushes preserve only the last durable state until a retry succeeds
- the first kernel-owned runtime handoff now runs after HAL validation and ethics-first boot
- the radix MMU now classifies invalid-TVA, unmapped, and permission-denied access faults
- the kernel runtime now owns allocator, page table, scheduler, IPC bus, and fault log state from `BootContext`
- the kernel runtime now also owns active device arbitration state for the first supported VirtualBox storage/display/network profile
- scheduler execution and CanonRef-safe IPC now flow through that runtime-owned kernel state via kernel-facing APIs
- the kernel runtime now exposes a deterministic `axion_kernel_step(...)` loop surface with runtime counters
- the kernel loop now delivers recorded MMU faults deterministically through a pending-fault queue
- the kernel loop now routes delivered faults into per-thread runtime state, quarantining the faulting thread and preserving a thread-local fault inbox
- faulting threads can now acknowledge delivered faults, but recovery is now gated by a kernel-owned process-group acknowledgement path
- the kernel now records audit-only governance events for fault delivery, quarantine, process-group fault entry, acknowledgement, and recovery
- TTF renders ASCII text into the VirtualBox VMSVGA-backed ternary framebuffer.
- TernaryEthernetPacket round-trips through the VirtualBox E1000 scaffold.

## Shell Demo

To run the first minimal Phase 5 shell/TUI scaffold:

```sh
cmake --build build --target t81_ternaryos_shell_demo
./build/t81_ternaryos_shell_demo
```

What it proves today:

- the shell path boots through the same VirtualBox guest bootstrap seam as the
  Phase 4 demo
- one scripted typed-command transcript is persisted through CanonStore over the
  AHCI-shaped storage binding
- that transcript survives reboot and is recovered before rendering
- the shell page is rendered through the VMSVGA-backed ternary framebuffer

## Shell TUI

To run the first FTXUI-based Axion shell frontend:

```sh
cmake --build build --target t81_ternaryos_shell_tui
./build/t81_ternaryos_shell_tui
```

For a noninteractive snapshot render:

```sh
./build/t81_ternaryos_shell_tui --snapshot
```

What the TUI adds:

- a proper terminal UI surface using the same FTXUI stack as the rest of the repo
- a transcript pane, session/status pane, and framebuffer preview in one screen
- a deterministic snapshot mode suitable for review and later test automation
- a minimal built-in command model behind the transcript:
  - `help`
  - `profile`
  - `name set <label> <ref>`
  - `name ls`
  - `object pin <kind> <name> <ref>`
  - `object ls`
  - `object show <name>`
  - `show profile`
  - `session status`
  - `session checkpoint`
  - `session export`
  - `session import <ref>`
  - `session diff <ref>`
  - `session run <ref>`
  - `session show durable`
  - `show session`
  - `session refs`
  - `store put <text>`
  - `store put script <line>|<line>|...>`
  - `store put ref <ref>`
  - `store cp <ref>`
  - `store ls`
  - `store get <ref>`
  - `show ref <canonref>`
  - `store rm <ref>`
  - `history`
  - `history show session`
  - `history show object <ref>`
  - `history use <ref>`
  - `history show durable`
  - `clear`
- a live typed-input loop in the interactive TUI:
  - printable characters append to the command buffer
  - `Backspace` edits
  - `Enter` executes into the transcript
  - `Up` / `Down` preload `history` / `profile` as quick shortcuts
- the session pane now separates shell-local state from durable CanonStore state:
  - session command count
  - durable ref count
  - durable anchor presence
- the shell now has explicit durable-state inspection commands:
  - `session refs` for the shell-tracked durable ref set
  - `name set <label> <ref>` for assigning a stable session-local alias to a CanonRef
  - `name ls` for listing those aliases
  - `object pin <kind> <name> <ref>` for promoting a CanonRef into a typed shell object
  - `object ls` for listing pinned shell objects
  - `object show <name>` for inspecting pinned object metadata
  - `session checkpoint` for persisting the current transcript as a canonical object
  - `session export` for promoting the current transcript to the durable history anchor
  - `session import <ref>` for restoring a persisted transcript object into the active shell window
  - `session diff <ref>` for comparing the current session transcript against a persisted object
  - `session run <ref>` for replaying a durable script object as a narrow command batch
  - `store put script <line>|<line>|...>` for writing those durable script objects
  - `history show durable` for the current durable history anchor
  - `session show durable` for the current durable anchor/ref view
- the shell now has an explicit session-history inspection command:
  - `history show session` for the current session command window
- the shell now has its first object-native read surface:
  - `show profile` for a structured profile view
  - `show session` for a structured session object view
  - `show ref <canonref>` for direct canonical-object lookup
- the shell now has its first object-native write/composition surface:
  - `store put ref <ref>` for canonical object composition
  - `store cp <ref>` for direct canonical object copy
- all CanonRef-taking commands now accept `@label` aliases created with `name set`:
  - `show ref @label`
  - `store get @label`
  - `store put ref @label`
  - `session import @label`
  - `session diff @label`
  - `session run @label`
- pinned shell objects automatically install a matching `@name` alias, so:
  - `object pin script bootstrap <ref>`
  - `session run @bootstrap`
  become the first typed object workflow above raw CanonRef strings
- the shell now has an object-history inspection surface:
  - `history show object <ref>` for durable object lookup without changing the active anchor
- the shell now has an explicit durable-anchor rebinding surface:
  - `history use <ref>` for adopting an existing canonical object as the active durable history anchor

What it is not yet:

- TISC userland code
- a general command parser or process manager
- arbitrary command piping or process composition

## Axion Serial Shell

To boot the current AArch64 QEMU/EDK2 kernel lane under QEMU and drop into the
Axion serial shell:

```sh
cmake --build build --target t81_ternaryos_qemu_slice6_shell
```

Or run the launcher directly:

```sh
./ternaryos/scripts/run_qemu_slice6_shell.sh build
```

To attach a second raw CanonFS image as the persistent store:

```sh
./ternaryos/scripts/run_qemu_slice6_shell.sh build /path/to/canon_store.img
```

For a noninteractive boot-to-prompt smoke check:

```sh
cmake --build build --target t81_ternaryos_qemu_slice6_shell_smoke
```

To exercise the CanonFS operator actions against a persistent store image:

```sh
./ternaryos/scripts/verify_qemu_slice6_shell.sh build /tmp/qemu_slice6_shell_smoke /path/to/canon_store.img
```

Notes:

- the launcher defaults to `tcg` because it is more reliable than `hvf` on the
  current EL0 scheduler and shell handoff path
- override the accelerator explicitly with `T81_QEMU_ACCEL=hvf` if you want to
  try the hardware-accelerated lane anyway
- exit QEMU with `Ctrl-a x`
- the early `ArmTrngLib`, `Tpm2*`, and `Error: Image at ... start failed`
  lines come from stock EDK2 probing optional firmware features; they are not
  the Axion serial shell failing to boot

Boot is complete when serial reaches:

```text
[axion] t81sh: ready (principal=axion, tier=1)
[axion@T81 tier=1]$
```

The current freestanding serial shell is intentionally narrow. Its built-in
commands are:

- `help`
- `tui`
- `uname`
- `version`
- `canonfs`
- `canonfs ls`
- `canonfs hash <alias>`
- `canonfs run <alias>`
- `irq`
- `el0`
- `waits`
- `status`
- `threads`
- `sched`
- `faults`
- `gov`
- `policy`

`tui` is a shared shell entry command, but it does not launch a guest-native UI
from the serial shell yet. When you boot through
`run_qemu_slice6_shell.sh`, the launcher now watches for the shell handoff token,
shuts down QEMU, and opens the hosted frontend at:

```text
./build/t81_ternaryos_shell_tui
```

If you boot the EFI image through raw QEMU instead of the launcher, `tui`
prints the same handoff target and token but does not auto-launch anything.

The launcher-assisted handoff currently carries a minimal shell context into the
TUI:

- shell source (`t81sh`)
- prompt and command (`[axion@T81 tier=1]$ tui`)
- the emitted `tui` handoff transcript block
- CanonFS mode (`in-memory` or `persistent, virtio-blk`)
- serial-shell status line / handoff banner

In that handoff startup mode, the TUI now opens on a read-only carried-shell
backend first. Enter `hosted` inside the TUI to switch deliberately into the
hosted Phase 5 shell backend.

That command surface comes from the bare-metal bridge in
`hal/qemu_slice6_cpp_bridge.cpp`. Internally this boot lane is still called
`slice6`; that is an implementation/build name, not the user-facing shell name.
It is separate from the broader hosted Phase 5 shell/TUI command model
documented above.

The `canonfs` subcommands are intentionally narrow:

- `canonfs` reports the current storage mode and retained I/O probe state
- `canonfs ls` shows the known boot-lane artifact aliases and LBAs
- `canonfs hash <alias>` reports the stored `code_hash` for T81X v2 artifacts
  and T81M manifests, and computes the payload hash on demand for T81X v1
  artifacts
- `canonfs run <alias>` launches a small allowlisted artifact and returns to
  the shell after the EL0 helper completes

Local hosted proof as of the current branch:

- all 8 TernOS test binaries pass
- `t81_ternaryos_hal_boot_test` is `2730/2730`
- `t81_ternaryos_device_driver_test` is `342/342`
- `t81_ternaryos_shell_session_test` is `183/183`
- `t81_ternaryos_mmu_test` is `87/87`
- total TernOS assertions are `3606`
- the first service-facing kernel request/result contract is now implemented
- healthy vs faulted groups now get deterministic request outcomes through that boundary
- stable service-facing diagnostics now exist for group, supervisor, fault, and device state
- stable service-facing audit summaries and per-device ownership details now exist through that same boundary
- a kernel-owned service runtime layer now exists above the supervisor/process-group boundary:
  service ids, supervisor ownership, backing process-group linkage, blocked/suspended state,
  stable service detail, lifecycle counters, and richer supervisor-owned service inventory
- same-supervisor process groups can now suspend/resume managed services through that same
  stable service action surface
- explicit service health transitions now exist through that same boundary, with stable
  unhealthy-state diagnostics and deterministic unavailable-service rejection
- successful service lifecycle transitions are now visible through that same
  deterministic audit-summary surface
- supervisor-owned inventory now also retains the latest managed-service
  lifecycle transition metadata
- supervisor-owned inventory entries now also retain each managed service's
  latest lifecycle kind and sequence
- compact supervisor status now also exposes managed-service lifecycle counts
  and latest-transition metadata
- supervisor recovery status now also exposes managed-service lifecycle counts
  and latest-transition metadata
- fault summary now also exposes managed-service lifecycle counts and
  latest-transition metadata
- runtime status now also exposes managed-service lifecycle counts and
  latest-transition metadata
- audit summary now also exposes managed-service lifecycle counts and
  latest-transition metadata
- device summary now also exposes managed-service lifecycle counts and
  latest-transition metadata
- service status now also exposes the latest service lifecycle kind and
  sequence
- process groups now also bind to explicit kernel-owned address spaces
- runtime, process-group, supervisor, and service diagnostics now also expose
  address-space ownership plus mapped-page counts
- delivered `Unmapped` faults now also mark the owning address space as
  pager-needed, while `PermissionDenied` and `InvalidTva` remain explicit
  policy faults
- runtime, process-group, service, supervisor, and fault diagnostics now also
  expose pager-needed address-space state without widening the public contract
- pager-needed address spaces now also enter a deterministic internal handoff
  queue, and kernel diagnostics distinguish handoff-pending from
  handoff-dispatched state without adding a public pager ABI
- once the missing mapping appears, the kernel loop now resolves one
  handed-off pager-needed address space at a time and exposes resolved state
  through the same diagnostics
- a first internal pager worker now exists as a kernel-owned FIFO consumer for
  pager handoffs, with deterministic repeated cycles on one address space
- repeated unresolved faults on a worker-owned address space now coalesce
  instead of creating duplicate pager work items, and diagnostics expose that
  worker-owned/coalesced state without widening the contract
- runtime and fault diagnostics now also retain pager backlog/load high-water
  marks plus worker activation counts, and HAL coverage proves FIFO handling
  across two queued address spaces
- runtime and fault diagnostics now also retain worker stall cycles and the
  narrower backlog-blocked subset when FIFO ordering holds queued work behind
  an unresolved active item
- runtime and fault diagnostics now also retain the ready-backlog subset when
  queued work is already mappable behind that stalled active item
- runtime and fault diagnostics now also expose current ready-backlog depth and
  its retained high-water mark under that same FIFO pressure
- runtime and fault diagnostics now also retain the stalled active address
  space alongside the ready queued address space it was blocking
- runtime and fault diagnostics now also retain the ordinal of the latest stall
  event that produced that blocker/blocked relationship
- runtime and fault diagnostics now also retain the exact stall ordinal that
  exposed the retained ready queued address space
- runtime and fault diagnostics now also retain the ready-backlog depth
  observed at that same stall event
- runtime and fault diagnostics now also retain the last activated address
  space and activation ordinal after the worker goes idle
- the first narrow service-facing action now exists through that same boundary:
  supervisor fault-group acknowledgement
- supervisor-facing recovery/report flows are now exposed through that same boundary:
  pending-group visibility, acknowledgement counts, recovered-group counts, and
  deterministic last-acknowledged/last-recovered group state
- a second narrow service-facing action now exists through that same boundary:
  deterministic device claim/release requests with healthy-vs-faulted group
  enforcement
- a third narrow service-facing action now exists through that same boundary:
  deterministic service registration with supervisor-owned inventory
- a fourth narrow service-facing action now exists through that same boundary:
  deterministic service unregister with stable post-unregister service/detail state
- the current kernel slice is now complete for service-runtime stabilization:
  service request routing, stable service detail, richer supervisor inventory,
  and deterministic lifecycle behavior now exist above the stable
  supervisor/process-group contract. The next step is to keep that contract
  stable and now carries explicit boot-progress/fail reporting on top of the
  new pager-needed, handoff-tracked, resolution-tracked, worker-consumed,
  terminal-failed, and boot-critical auto-resolved address-space state. That
  closes the current internal boot-ready kernel slice. The next step is
  external boot-lane validation, tracked explicitly in:
  - `experimental/ternaryos/docs/kernel_execution_plan.md`
- the RFC-00B5 interrupt summary-convergence slice is now complete too: the
  kernel can intake explicit interrupt events, deliver them deterministically
  through `axion_kernel_step(...)`, and expose queue state, per-source
  accounting, latest interrupt-audit metadata, and record-level intake/delivery
  provenance through the stable runtime/fault/audit summaries without widening
  the service ABI
- guest-bootstrap storage coverage now includes:
  - repeated reboot persistence
  - header corruption fallback
  - torn-header fallback
  - multi-block CanonStore metadata persistence
  - interrupted-flush durability semantics
- kernel-facing fault coverage now includes:
  - checked MMU translation consumed through the first kernel-owned runtime path
  - deterministic `InvalidTva`, `Unmapped`, and `PermissionDenied` fault records
- kernel-runtime ownership now includes:
  - allocator seeded from `BootContext`
  - runtime-owned page table
  - runtime-owned scheduler and IPC bus
  - persistent kernel fault log
  - active device arbitration for the first supported VirtualBox profile
- kernel-runtime behavior now includes:
  - deterministic scheduler dispatch through `axion_kernel_tick(...)`
  - CanonRef-safe IPC send/receive through runtime-owned kernel APIs
  - deterministic loop progression through `axion_kernel_step(...)`
  - runtime counters for loop, scheduler, and IPC activity
  - queued fault delivery with deterministic first-in-first-out loop consumption
  - thread-local fault routing and deterministic fault-thread quarantine
  - process-group fault policy with explicit group acknowledgement gates
  - audit-only governance events recorded in deterministic sequence
  - deterministic thread recovery only after both thread and process-group acknowledgement

## VirtualBox Artifact

To generate the first reproducible VirtualBox guest artifact bundle:

```sh
cmake --build build --target t81_ternaryos_virtualbox_guest_artifact
```

Outputs:

- `build/ternaryos/virtualbox/ternos_virtualbox_guest.img`
- `build/ternaryos/virtualbox/ternos_virtualbox_guest.vdi`
- `build/ternaryos/virtualbox/BOOTX64.obj`
- `build/ternaryos/virtualbox/BOOTX64.EFI`
- `build/ternaryos/virtualbox/staging/TERNOS/profile.txt`
- `build/ternaryos/virtualbox/staging/TERNOS/expected-boot-report.txt`
- `build/ternaryos/virtualbox/staging/TERNOS/expected-startup-status.txt`
- `build/ternaryos/virtualbox/staging/TERNOS/demo-output.txt`

To package the official `x86_64` handoff bundle for an external validator:

```sh
cmake --build build --target t81_ternaryos_virtualbox_x86_64_handoff
```

Outputs:

- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff/`
- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff.tar.gz`
- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff.tar.gz.sha256`
- bundle includes `expected-boot-report.txt` and `expected-startup-status.txt`
  so the external `x86_64` host can validate the same boot-progress contract
  now used by the ARM/QEMU developer lane
- bundle also includes `validate_virtualbox_x86_64_handoff.sh` for direct
  comparison of guest-produced `boot-report.txt` / `startup-status.txt`
- bundle also includes a `recovered-artifacts/` template directory so the
  external host has a standard place to return recovered boot outputs
- a local `t81_ternaryos_virtualbox_x86_64_handoff_fixture` target now proves
  the packaged `x86_64` handoff validator against a valid synthetic artifact set
- a local `t81_ternaryos_virtualbox_x86_64_handoff_negative_fixture` target now
  proves the validator rejects a mismatched boot-progress artifact set
- a local `t81_ternaryos_virtualbox_x86_64_handoff_bundle_smoke` target now
  proves the packaged handoff bundle is internally self-consistent
- a local `t81_ternaryos_virtualbox_x86_64_handoff_bundle_negative_smoke`
  target now proves the packaged bundle also rejects a mismatched smoke fixture
- that closes the local `x86_64` handoff packaging lane; the next step is a real
  external `x86_64` VirtualBox host run returning recovered boot artifacts

To boot-probe the staged `x86_64` guest image under a local QEMU EFI
diagnostic lane:

```sh
cmake --build build --target t81_ternaryos_qemu_x86_64_guest_probe
```

Outputs:

- `build/ternaryos/qemu_x86_64_guest/qemu-x86_64-guest-summary.txt`
- `build/ternaryos/qemu_x86_64_guest/qemu-x86_64-guest-serial.log`
- `build/ternaryos/qemu_x86_64_guest/qemu-x86_64-guest-probe.img`
- `build/ternaryos/qemu_x86_64_guest/efi-ran.txt`
- `build/ternaryos/qemu_x86_64_guest/boot-report.txt`
- `build/ternaryos/qemu_x86_64_guest/startup-status.txt`
- `build/ternaryos/qemu_x86_64_guest/expected-boot-report.txt`
- `build/ternaryos/qemu_x86_64_guest/expected-startup-status.txt`

To generate the temporary ARMv8 developer-lane artifact for Apple Silicon
VirtualBox hosts:

```sh
cmake --build build --target t81_ternaryos_virtualbox_armv8_dev_artifact
```

Outputs:

- `build/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img`
- `build/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.vdi`
- `build/ternaryos/virtualbox_armv8/BOOTAA64.obj`
- `build/ternaryos/virtualbox_armv8/BOOTAA64.EFI`
- `build/ternaryos/virtualbox_armv8/staging/TERNOS/profile.txt`
- `build/ternaryos/virtualbox_armv8/staging/TERNOS/demo-output.txt`

To check whether the local VirtualBox host can validate the current `x86_64`
guest target:

```sh
cmake --build build --target t81_ternaryos_virtualbox_host_check
```

To check whether the local VirtualBox host can validate the temporary `ARMv8`
developer lane:

```sh
cmake --build build --target t81_ternaryos_virtualbox_armv8_host_check
```

To boot-probe the temporary ARMv8 developer lane headlessly in local
VirtualBox:

```sh
cmake --build build --target t81_ternaryos_virtualbox_armv8_boot_probe
```

Outputs:

- `build/ternaryos/virtualbox_armv8/armv8_boot_probe.log`
- `build/ternaryos/virtualbox_armv8/armv8_boot_probe_summary.txt`
- `build/ternaryos/virtualbox_armv8/efi-link-status.txt`

To run the primary local ARM EFI developer probe under QEMU:

```sh
cmake --build build --target t81_ternaryos_qemu_armv8_efi_control_probe
```

Outputs:

- `build/ternaryos/qemu_armv8_control/qemu-armv8-control-serial.log`
- `build/ternaryos/qemu_armv8_control/edk2-aarch64-vars.fd`

To boot-probe the staged ARM guest image under the real Axion QEMU developer lane:

```sh
cmake --build build --target t81_ternaryos_qemu_armv8_guest_probe
```

Outputs:

- `build/ternaryos/qemu_armv8_guest/qemu-armv8-guest-summary.txt`
- `build/ternaryos/qemu_armv8_guest/qemu-armv8-guest-serial.log`
- `build/ternaryos/qemu_armv8_guest/qemu-armv8-guest-probe.img`
- `build/ternaryos/qemu_armv8_guest/boot-report.txt`
- `build/ternaryos/qemu_armv8_guest/startup-status.txt`
- `build/ternaryos/qemu_armv8_guest/startup-shell.txt`
  - generated from the real Axion shell backend at build time, then embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-session.txt`
  - backend-generated `show session` snapshot embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-history.txt`
  - backend-generated durable-history snapshot embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-store.txt`
  - backend-generated `store ls` inventory snapshot embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-ref.txt`
  - backend-generated `show ref <canonref>` snapshot embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-report.txt`
  - consolidated backend-generated shell/session/history/store/ref proof surface embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest/startup-phase4.txt`
  - consolidated backend-generated Phase 4 storage/display/network proof surface embedded into the ARM EFI stub
- `build/ternaryos/qemu_armv8_guest_blocked_fixture/`
  - synthetic blocked boot-progress fixture used to prove the shared guest-report validator distinguishes blocked boot state from the ready live probe

Current status:

- the image is FAT-formatted and VirtualBox-ready as a disk artifact
- it stages the current guest profile, captured demo evidence, a compiled `BOOTX64.obj` stub object, and a local diagnostic `BOOTX64.EFI` candidate
- on this Apple Silicon host, `VBoxManage list systemproperties` currently reports `Supported platform architectures: ARMv8`, so the `x86_64` guest target cannot be boot-validated locally
- local QEMU x86_64 + EDK2 can now execute the staged `BOOTX64.EFI` candidate, recover `efi-ran.txt`, `boot-report.txt`, and `startup-status.txt`, and pass the shipped `validate_virtualbox_x86_64_handoff.sh` contract helper unchanged
- that local QEMU x86_64 lane is diagnostic only: it reduces uncertainty before external handoff, but it does not replace the real `x86_64` VirtualBox acceptance lane
- the ARMv8 developer lane now goes one step further locally: VirtualBox firmware can boot headless, open the staged VDI through AHCI, and emit a captured `VBox.log`
- with `lld` installed, the ARMv8 lane now emits a real `BOOTAA64.EFI`, but it is still a developer-lane shim rather than the true C++ HAL bridge
- a separate control `BOOTAA64_CTRL.EFI` now exists for the ARMv8 lane and is staged ahead of the shim-backed app in `STARTUP.NSH`; current local probes still show no `startup-ran.txt`, `efi-ctrl-ran.txt`, or `efi-ran.txt` markers, which strongly suggests the local blocker is VirtualBox ARM EFI execution/boot selection rather than the TernOS HAL bridge
- that VirtualBox-specific conclusion is now stronger because the same ARM image executes under local QEMU AArch64 + EDK2 and leaves `TERNOS/efi-ran.txt`; the current blind spot is therefore the local VirtualBox ARM path, not the basic ARM EFI control artifact
- the QEMU lane is now usable for actual staged guest bring-up too: it can boot the staged ARM guest image, capture serial output, and inspect the mutated probe image; current local probes show `efi-ran.txt`, `boot-report.txt`, and `boot_path_inference=default-bootaa64-efi`, so QEMU is reaching the staged `BOOTAA64.EFI` directly rather than via shell fallback
- the current `boot-report.txt` confirms the staged ARM guest reaches the Axion handoff stub with `platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane`, `hal_main_result=0`, `kernel_boot_ready_slice=complete`, `boot_progress_state=ready`, and `boot_progress_source=kernel-boot-critical-policy`
- the QEMU serial log now also includes a visible boot banner, `Axion ARMv8 EFI stub`, so the developer lane has a direct live boot signal as well as file-based proof
- the current QEMU lane also recovers backend-generated `startup-shell.txt`, `startup-session.txt`, `startup-history.txt`, `startup-store.txt`, `startup-ref.txt`, and `startup-report.txt`, so the staged ARM guest is now exposing real shell/session/store state rather than only handwritten boot metadata
- the current QEMU lane also recovers backend-generated `startup-phase4.txt`, so the staged ARM guest now exposes a pure Phase 4 device-layer proof in addition to the shell/runtime artifacts
- the current `startup-phase4.txt` proves guest-bootstrap behavior from the actual Phase 4 seam:
  - `storage_binding=virtualbox-ahci`
  - `canonstore_inventory_count=20`
  - `canonstore_index_entries_per_block=17`
  - `canonstore_overflow_active=true`
  - `canonstore_recovered_entries=20`
  - `canonstore_second_cycle_entries=20`
  - `canonstore_torn_header_entries=20`
  - all 20 stored CanonRefs resolve successfully across both guest cycles
  - all 20 stored CanonRefs resolve successfully after a torn-header fallback rebuild
  - `display_binding=virtualbox-vmsvga`
  - `display_present_count=3`
  - a mutable three-present framebuffer cycle is captured from the same staged guest path
  - `network_binding=virtualbox-e1000`
  - `network_runtime_batches=2`
  - `network_tx_frames=5`
  - `network_rx_frames=5`
  - `network_roundtrip_ok=3`
  - `network_roundtrip_total=3`
- the current `startup-status.txt` exposes guest-visible Axion state from the staged ARM guest:
  - `os_name=Axion`
  - `phase=5`
  - `shell_mode=typed-builtins`
  - `kernel_boot_ready_slice=complete`
  - `boot_progress_pending=false`
  - `boot_progress_blocked=false`
  - `boot_validation_lane=qemu-armv8-guest-probe`
  - `storage_binding=virtualbox-ahci`
  - `display_binding=virtualbox-vmsvga`
  - `network_binding=virtualbox-e1000`
- the current `startup-shell.txt` exposes the staged Axion shell surface under QEMU and is generated from the real shell backend at build time:
  - `prompt=axion>`
  - `mode=typed-builtins`
  - `history_anchor=durable`
  - `session_view=local+durable`
  - command surface including `show profile`, `show session`, `show ref <canonref>`, `session show durable`, `history show session`, and `history show durable`
- the current `startup-session.txt` exposes backend-generated shell session state under QEMU:
  - `profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC`
  - `session_command_count=6`
  - `durable_ref_count=1`
  - `durable_anchor=present`
- the current `startup-history.txt` exposes a backend-generated durable-history view under QEMU:
  - `command=history show durable`
  - a durable CanonRef-backed history result from the real shell backend

For an external reviewer, the current evidence split is:

- locally proven: hosted HAL + guest-bootstrap storage/network/display path + CanonStore persistence/recovery/durability semantics
- locally unproven: official `x86_64` VirtualBox guest boot execution

## Validation Lanes

- Primary acceptance lane: `x86_64` VirtualBox host capable of boot-validating the roadmap target (`VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC`)
- Primary local developer lane: QEMU AArch64 + EDK2 on Apple Silicon for observable EFI execution and early guest bring-up
- Secondary diagnostic lane: Apple Silicon / `ARMv8` VirtualBox host used only for artifact generation, host checks, and narrow VirtualBox-specific boot-path investigation
- The local ARMv8 lane now reaches a compiled `BOOTAA64.obj`, a linkable `BOOTAA64.EFI` developer-lane shim, packaged `.img`/`.vdi`, a first-class QEMU EFI control probe, and a QEMU guest probe that confirms the staged ARM image reaches `BOOTAA64.EFI` on this machine

Program rule:

- do not retarget the roadmap to `ARMv8`
- use QEMU AArch64 as the primary local EFI/guest debugging lane
- keep local VirtualBox ARM work tactical and diagnostic only
- reserve final VirtualBox guest boot proof for an `x86_64`-capable host

If no local `x86_64` host is available, use the handoff runbook:

- [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)

## Promotion Path

Each layer graduates from `experimental/ternaryos/` to the mainline when its
RFC acceptance criteria are met (see `PROGRESS.md` promotion checklist).
Promoted sources move to `include/t81/`, `src/`, `runtime/` and become CI-gated.
