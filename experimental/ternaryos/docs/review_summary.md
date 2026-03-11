# Axion Phase 4 Review Summary

## Current State

Working release label: `Axion v0.1.0-alpha`

Axion Phases 1 through 3 are implemented and passing. Phase 4 is implemented
as a VirtualBox-first hosted simulation path with guest-owned storage, network,
and display seams:

Naming note:
- `Axion` is the OS name
- `T81 Foundation` remains the umbrella project name
- `T81VM`, `CanonFS`, and `TISC` remain subsystem/runtime names

- storage: `AHCI`-shaped binding over `IBlockDevice`
- network: `E1000`-shaped binding over ternary packet/frame translation
- display: `VMSVGA`-shaped binding over the ternary framebuffer + TTF renderer

The next kernel-integration path is now explicit in
[RFC-00B3: Axion Kernel Architecture](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md).

The official promotion target remains:

- `x86_64`
- `VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC`

The next kernel slice is now tracked explicitly in:

- [kernel_execution_plan.md](kernel_execution_plan.md)

## What Is Proven Locally

Hosted proof is strong on the current branch:

- all 8 TernOS test binaries pass
- total assertions: `1767`
- `t81_ternaryos_device_driver_test`: `342/342`
- `t81_ternaryos_hal_boot_test`: `855/855`
- `t81_ternaryos_shell_session_test`: `183/183`
- `t81_ternaryos_mmu_test`: `87/87`

Kernel integration proof now also includes:

- a real `hal_main -> axion_kernel_main(...)` handoff
- kernel-visible MMU fault reporting with deterministic `InvalidTva`,
  `Unmapped`, and `PermissionDenied` classification
- a persistent kernel runtime state seeded from `BootContext`, now owning the
  allocator, page table, scheduler substrate, IPC bus, and fault log
- active device arbitration for the supported VBox EFI/AHCI/E1000/VMSVGA
  profile attached to that same runtime boundary
- deterministic scheduler dispatch and CanonRef-safe IPC execution flowing
  through that same runtime-owned kernel state
- a deterministic kernel-step loop with runtime counters and active AHCI claim/release behavior
- deterministic FIFO fault delivery from the kernel loop over recorded MMU faults
- delivered MMU faults now route into per-thread runtime state, quarantining the faulting thread and preserving a thread-local fault inbox
- the owning process group now enters a blocked fault state and must be explicitly acknowledged before a drained thread inbox can recover
- audit-only governance events are now recorded deterministically for fault delivery, quarantine, process-group fault entry, acknowledgement, and recovery
- supervisor-facing recovery/report status is now exposed through the same service boundary, including pending-group state, acknowledgement counts, recovered-group counts, and deterministic last-acknowledged/last-recovered group tracking
- deterministic device claim/release requests now exist through that same service boundary, with healthy groups allowed to arbitrate devices and faulted groups rejected consistently
- request-side and action-side rejection semantics are now explicit across the stable kernel service boundary
- stable audit summaries and per-device ownership details are now exposed through that same service boundary for healthy callers
- a first kernel-owned service runtime layer now exists above the current supervisor/process-group contract, now including deterministic service registration, deterministic service unregister, stable service detail, and richer supervisor-owned inventory
- that service-runtime layer now also includes deterministic service suspend/resume with stable suspended-state diagnostics in service detail and supervisor inventory views
- same-supervisor process groups can now suspend/resume managed services through the same stable action surface without widening into a new ABI
- explicit service health transitions now exist through that same stable action surface, exposing unhealthy-state diagnostics and deterministic unavailable-service rejection
- successful service lifecycle transitions are now visible through the same deterministic audit summary surface
- supervisor-owned inventory now also retains the latest managed-service lifecycle transition metadata
- compact supervisor status now also exposes managed-service lifecycle counts and latest-transition metadata
- supervisor recovery status now also exposes managed-service lifecycle counts and latest-transition metadata
- fault summary now also exposes managed-service lifecycle counts and latest-transition metadata
- runtime status now also exposes managed-service lifecycle counts and latest-transition metadata
- audit summary now also exposes managed-service lifecycle counts and latest-transition metadata
- device summary now also exposes managed-service lifecycle counts and latest-transition metadata
- service status now also exposes the latest service lifecycle kind and sequence
- supervisor inventory entries now also expose each managed service's latest
  lifecycle kind and sequence
- the next kernel slice is to keep this service-runtime layer stable and only add another narrow lifecycle action if it is truly needed, not a broad ABI or syscall surface

Phase 4 storage proof now covers:

- reboot persistence through the VirtualBox guest bootstrap path
- recovery after header corruption
- recovery after torn-header metadata
- metadata scaling beyond the original 17-entry root-header threshold
- interrupted-flush durability semantics

Other locally proven Phase 4 behavior:

- TTF text rendering into the guest display path
- ternary ethernet packet/frame round-trip through the guest network path

ARM diagnostic result:

- local QEMU AArch64 + EDK2 does execute the ARM EFI control app and is now
  the primary local developer lane
- local QEMU AArch64 can also boot-probe the staged ARM guest image and inspect
  its execution markers directly; current probes show the staged image reaches
  `BOOTAA64.EFI` without needing shell fallback and writes a boot report with
  `hal_main_result=0`
- local QEMU serial output now also includes `Axion ARMv8 EFI stub`, giving the
  staged ARM guest a direct live boot signal
- local QEMU guest probes now also recover `startup-status.txt`, exposing
  guest-visible Axion state and confirming the current staged bindings for
  shell, storage, display, and network
- local QEMU guest probes now also recover `startup-shell.txt`, exposing the
  staged Axion shell prompt, durable-history stance, and current typed-builtin
  command surface from a build-time snapshot generated by the real shell backend,
  now including durable transcript import via `session import <ref>`
- local QEMU guest probes now also recover `startup-session.txt`, exposing a
  backend-generated `show session` snapshot with the active profile and durable
  state counts
- local QEMU guest probes now also recover `startup-history.txt`, exposing a
  backend-generated durable-history view derived from the real Axion shell backend
- local QEMU guest probes now also recover `startup-store.txt`, exposing a
  backend-generated `store ls` inventory snapshot from the same shell backend
- local QEMU guest probes now also recover `startup-ref.txt`, exposing a
  backend-generated `show ref <canonref>` object retrieval snapshot from the
  same shell backend
- local QEMU guest probes now also recover `startup-report.txt`, exposing a
  consolidated backend-generated shell/session/history/store/ref proof surface
- local QEMU guest probes now also recover `startup-phase4.txt`, exposing a
  pure Phase 4 device-layer proof surface derived from the real guest bootstrap:
  20-object CanonStore recovery across two guest cycles, active overflow
  metadata past the 17-entry root-header threshold, successful torn-header
  recovery for the same 20 objects, through `virtualbox-ahci`, a mutable
  three-present VMSVGA cycle, and a two-batch five-frame E1000 workload
- local VirtualBox ARM remains non-observable for EFI execution and is now only
  a secondary diagnostic lane
- conclusion: the remaining blind spot is the local VirtualBox ARM path, not
  the basic ARM EFI artifact shape

## What Is Not Yet Proven

The main remaining unknown is external to this machine:

- does the official `x86_64` VirtualBox guest lane actually boot and expose the
  staged TernOS payload?

That is the current program blocker for the promotion path.

## Reviewer Ask

Use the packaged `x86_64` handoff bundle and run the official VirtualBox lane on
an `x86_64` host.

Primary goal:

- prove whether VBox EFI discovers and executes the staged guest path at all

Useful first success signals:

- EFI sees the disk
- EFI shell sees the filesystem
- the staged payload runs
- any deterministic boot marker appears

## Inputs For The Reviewer

See:

- [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)

Expected bundle contents:

- guest `.vdi`
- raw guest image
- profile summary
- demo transcript
- handoff runbook

## What To Report Back

- host OS and VirtualBox version
- confirmation that the host/guest lane was truly `x86_64`
- exact VM settings
- `VBox.log`
- screenshots or firmware text
- whether the staged disk was visible
- whether any boot path executed

## Program Recommendation

Do not treat subsystem growth as the only path forward. The current local
kernel path is now:

- keep the external `x86_64` VirtualBox validation ask open
- use RFC-00B3 as the implementation path for kernel integration after `hal_main`
- harden the existing service-facing runtime contract before adding more
  actions or widening the boundary further
