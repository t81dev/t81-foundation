# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**Review Summary:** [review_summary.md](review_summary.md)
**x86_64 Handoff:** [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
**Shell Design:** [axion_shell_design.md](axion_shell_design.md)
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [spec/rfcs/RFC-00B0-hal-spec.md](../../../spec/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [spec/rfcs/RFC-00B1-ternary-mmu.md](../../../spec/rfcs/RFC-00B1-ternary-mmu.md)
**RFC-00B2 (Drivers):** [spec/rfcs/RFC-00B2-device-drivers.md](../../../spec/rfcs/RFC-00B2-device-drivers.md)
**RFC-00B3 (Kernel):** [spec/rfcs/RFC-00B3-axion-kernel-architecture.md](../../../spec/rfcs/RFC-00B3-axion-kernel-architecture.md)

Prototype implementation of Axion, the current working name for the ternary-native
OS kernel on the T81VM runtime. Phases 1 through 3 are complete, Phase 4
device-driver work is in progress, and Phase 5 now has a first typed shell/TUI
path on top of the hosted guest-bootstrap path.

Kernel integration direction is now tracked explicitly in RFC-00B3. That RFC is
the current path for turning the existing HAL/MMU/scheduler/IPC subsystems into
one kernel-owned runtime unit instead of letting them grow organically.

Current working release label: `Axion v0.1.0-alpha`

For now, internal paths, namespaces, CMake targets, and test names still use
`ternaryos` while the external/project-facing name shifts to `Axion`.

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
  axion_shell_design.md Phase 5 shell design note
  virtualbox_x86_64_handoff.md External x86_64 VirtualBox runbook

hal/
  hal.hpp              HAL public interface (MemoryRegion, HardwareInterrupt,
                       BootContext, hal_main)
  hal_c_abi.h/.cpp     C ABI bridge for freestanding guest stubs
  hal_main.cpp         Ethics-first boot (Θ₁–Θ₉ via Axion) → T81VM handoff
  interrupt_table.cpp  Shadow binary interrupt dispatch table
  hosted_stub.cpp      Hosted (macOS/Linux) simulation — stand-in for UEFI stub
  virtualbox_platform.hpp/.cpp  First-target VirtualBox VM profile scaffold
  virtualbox_guest_devices.hpp/.cpp  VirtualBox profile-to-device binding helpers
  virtualbox_efi_stub.c  Freestanding VBox EFI stub source for BOOTX64 handoff
  virtualbox_armv8_efi_stub.c  Freestanding VBox EFI stub source for BOOTAA64 handoff
  virtualbox_armv8_efi_control.c  Minimal ARMv8 EFI control app for execution probes
  virtualbox_armv8_efi_shim.c  Temporary ARMv8 developer-lane EFI link shim

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
  hal_boot_test.cpp          Phase 1 — 84 assertions
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
# Expected: 1031/1031 assertions, 8/8 tests pass
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
- the radix MMU now classifies invalid-TVA, unmapped, and permission-denied access faults
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

Local hosted proof as of the current branch:

- all 8 TernOS test binaries pass
- `t81_ternaryos_device_driver_test` is `342/342`
- `t81_ternaryos_shell_session_test` is `183/183`
- `t81_ternaryos_mmu_test` is `72/72`
- total TernOS assertions are `1016`
- guest-bootstrap storage coverage now includes:
  - repeated reboot persistence
  - header corruption fallback
  - torn-header fallback
  - multi-block CanonStore metadata persistence
  - interrupted-flush durability semantics

## VirtualBox Artifact

To generate the first reproducible VirtualBox guest artifact bundle:

```sh
cmake --build build --target t81_ternaryos_virtualbox_guest_artifact
```

Outputs:

- `build/ternaryos/virtualbox/ternos_virtualbox_guest.img`
- `build/ternaryos/virtualbox/ternos_virtualbox_guest.vdi`
- `build/ternaryos/virtualbox/BOOTX64.obj`
- `build/ternaryos/virtualbox/staging/TERNOS/profile.txt`
- `build/ternaryos/virtualbox/staging/TERNOS/demo-output.txt`

To package the official `x86_64` handoff bundle for an external validator:

```sh
cmake --build build --target t81_ternaryos_virtualbox_x86_64_handoff
```

Outputs:

- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff/`
- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff.tar.gz`
- `build/ternaryos/handoff/ternos_virtualbox_x86_64_handoff.tar.gz.sha256`

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

Current status:

- the image is FAT-formatted and VirtualBox-ready as a disk artifact
- it stages the current guest profile, captured demo evidence, and a compiled `BOOTX64.obj` stub object
- it is not EFI-bootable yet; final PE/COFF `.efi` linking is still missing, so `BOOTX64.EFI` is not produced yet
- on this Apple Silicon host, `VBoxManage list systemproperties` currently reports `Supported platform architectures: ARMv8`, so the `x86_64` guest target cannot be boot-validated locally
- the ARMv8 developer lane now goes one step further locally: VirtualBox firmware can boot headless, open the staged VDI through AHCI, and emit a captured `VBox.log`
- with `lld` installed, the ARMv8 lane now emits a real `BOOTAA64.EFI`, but it is still a developer-lane shim rather than the true C++ HAL bridge
- a separate control `BOOTAA64_CTRL.EFI` now exists for the ARMv8 lane and is staged ahead of the shim-backed app in `STARTUP.NSH`; current local probes still show no `startup-ran.txt`, `efi-ctrl-ran.txt`, or `efi-ran.txt` markers, which strongly suggests the local blocker is VirtualBox ARM EFI execution/boot selection rather than the TernOS HAL bridge
- that VirtualBox-specific conclusion is now stronger because the same ARM image executes under local QEMU AArch64 + EDK2 and leaves `TERNOS/efi-ran.txt`; the current blind spot is therefore the local VirtualBox ARM path, not the basic ARM EFI control artifact
- the QEMU lane is now usable for actual staged guest bring-up too: it can boot the staged ARM guest image, capture serial output, and inspect the mutated probe image; current local probes show `efi-ran.txt`, `boot-report.txt`, and `boot_path_inference=default-bootaa64-efi`, so QEMU is reaching the staged `BOOTAA64.EFI` directly rather than via shell fallback
- the current `boot-report.txt` confirms the staged ARM guest reaches the Axion handoff stub with `platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane` and `hal_main_result=0`
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
