# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**Review Summary:** [review_summary.md](review_summary.md)
**x86_64 Handoff:** [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)
**RFC-00B2 (Drivers):** [docs/rfcs/RFC-00B2-device-drivers.md](../../docs/rfcs/RFC-00B2-device-drivers.md)

Prototype implementation of Axion, the current working name for the ternary-native
OS kernel on the T81VM runtime. Phases 1 through 3 are complete, Phase 4
device-driver work is in progress, and Phase 5 now has a first built-in shell/TUI
path on top of the hosted guest-bootstrap path.

For now, internal paths, namespaces, CMake targets, and test names still use
`ternaryos` while the external/project-facing name shifts to `Axion`.

Naming rule for now:
- `T81 Foundation` is the umbrella project/ecosystem
- `T81VM` is the ternary runtime/execution substrate
- `Axion` is the operating system
- existing subsystem names like `CanonFS` and `TISC` stay as-is

## Structure

```
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
  page_table.hpp/.cpp  Flat VPN→physical page map; mmu_map/translate/unmap

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

demo.cpp               Phase 4 hosted presentation demo
shell_session.hpp/.cpp Phase 5 shell session backend over guest bootstrap
shell_demo.cpp         Phase 5 verbose shell backend proof
shell_tui.cpp          Phase 5 FTXUI shell frontend with snapshot mode

tests/
  hal_boot_test.cpp          Phase 1 — 84 assertions
  ternary_page_alloc_test.cpp Phase 1 — 28 assertions
  context_switch_test.cpp    Phase 1 — 43 assertions
  mmu_test.cpp               Phase 2 — 47 assertions
  scheduler_test.cpp         Phase 3 — 120 assertions
  ipc_test.cpp               Phase 3 — 73 assertions
  device_driver_test.cpp     Phase 4 — 342 assertions
```

## Build & Test

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R ternaryos -V
# Expected: 737/737 assertions, 7/7 tests pass
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
- one scripted shell transcript is persisted through CanonStore over the
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
  - `store put`
  - `history`
- a live selection/execution loop in the interactive TUI:
  - `Up` / `Down` or `j` / `k` selects a built-in
  - `Enter` or `Space` executes it into the transcript

What it is not yet:

- TISC userland code
- a general command parser or process manager

Local hosted proof as of the current branch:

- all 7 TernOS test binaries pass
- `t81_ternaryos_device_driver_test` is `342/342`
- total TernOS assertions are `737`
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

To run the ARM EFI control app in a more observable local AArch64 EFI
environment under QEMU:

```sh
/bin/zsh experimental/ternaryos/scripts/run_qemu_armv8_efi_control.sh \
  build/ternaryos/virtualbox_armv8/ternos_virtualbox_armv8_dev_guest.img \
  build/ternaryos/qemu_armv8_control
```

Outputs:

- `build/ternaryos/qemu_armv8_control/qemu-armv8-control-serial.log`
- `build/ternaryos/qemu_armv8_control/edk2-aarch64-vars.fd`

Current status:

- the image is FAT-formatted and VirtualBox-ready as a disk artifact
- it stages the current guest profile, captured demo evidence, and a compiled `BOOTX64.obj` stub object
- it is not EFI-bootable yet; final PE/COFF `.efi` linking is still missing, so `BOOTX64.EFI` is not produced yet
- on this Apple Silicon host, `VBoxManage list systemproperties` currently reports `Supported platform architectures: ARMv8`, so the `x86_64` guest target cannot be boot-validated locally
- the ARMv8 developer lane now goes one step further locally: VirtualBox firmware can boot headless, open the staged VDI through AHCI, and emit a captured `VBox.log`
- with `lld` installed, the ARMv8 lane now emits a real `BOOTAA64.EFI`, but it is still a developer-lane shim rather than the true C++ HAL bridge
- a separate control `BOOTAA64_CTRL.EFI` now exists for the ARMv8 lane and is staged ahead of the shim-backed app in `STARTUP.NSH`; current local probes still show no `startup-ran.txt`, `efi-ctrl-ran.txt`, or `efi-ran.txt` markers, which strongly suggests the local blocker is VirtualBox ARM EFI execution/boot selection rather than the TernOS HAL bridge
- that VirtualBox-specific conclusion is now stronger because the same ARM image executes under local QEMU AArch64 + EDK2 and leaves `TERNOS/efi-ran.txt`; the current blind spot is therefore the local VirtualBox ARM path, not the basic ARM EFI control artifact

For an external reviewer, the current evidence split is:

- locally proven: hosted HAL + guest-bootstrap storage/network/display path + CanonStore persistence/recovery/durability semantics
- locally unproven: official `x86_64` VirtualBox guest boot execution

## Validation Lanes

- Primary acceptance lane: `x86_64` VirtualBox host capable of boot-validating the roadmap target (`VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC`)
- Secondary developer lane: Apple Silicon / `ARMv8` VirtualBox host used for artifact generation, host checks, and boot-pipeline preparation only
- The ARMv8 lane now reaches a compiled `BOOTAA64.obj`, a linkable `BOOTAA64.EFI` developer-lane shim, packaged `.img`/`.vdi`, and a headless VirtualBox boot probe that confirms firmware-visible AHCI disk attachment

Program rule:

- do not retarget the roadmap to `ARMv8`
- use the local ARMv8 host to keep artifact/tooling work moving
- reserve final VirtualBox guest boot proof for an `x86_64`-capable host

If no local `x86_64` host is available, use the handoff runbook:

- [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)

## Promotion Path

Each layer graduates from `experimental/ternaryos/` to the mainline when its
RFC acceptance criteria are met (see `PROGRESS.md` promotion checklist).
Promoted sources move to `include/t81/`, `src/`, `runtime/` and become CI-gated.
