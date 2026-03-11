# experimental/ternaryos

**Status:** Experimental — non-DCP, not governance-gated.
**Progress:** [PROGRESS.md](PROGRESS.md) ← start here
**x86_64 Handoff:** [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
**Roadmap:** [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
**RFC-00B0 (HAL):** [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
**RFC-00B1 (MMU):** [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)
**RFC-00B2 (Drivers):** [docs/rfcs/RFC-00B2-device-drivers.md](../../docs/rfcs/RFC-00B2-device-drivers.md)

Prototype implementation of TernOS — a ternary-native OS kernel for the T81VM
runtime. Phases 1 through 3 are complete, and Phase 4 device-driver work is in
progress.

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

tests/
  hal_boot_test.cpp          Phase 1 — 84 assertions
  ternary_page_alloc_test.cpp Phase 1 — 28 assertions
  context_switch_test.cpp    Phase 1 — 43 assertions
  mmu_test.cpp               Phase 2 — 47 assertions
  scheduler_test.cpp         Phase 3 — 120 assertions
  ipc_test.cpp               Phase 3 — 73 assertions
  device_driver_test.cpp     Phase 4 — 165 assertions
```

## Build & Test

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build -R ternaryos -V
# Expected: 560/560 assertions, 7/7 tests pass
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
- TTF renders ASCII text into the VirtualBox VMSVGA-backed ternary framebuffer.
- TernaryEthernetPacket round-trips through the VirtualBox E1000 scaffold.

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

Current status:

- the image is FAT-formatted and VirtualBox-ready as a disk artifact
- it stages the current guest profile, captured demo evidence, and a compiled `BOOTX64.obj` stub object
- it is not EFI-bootable yet; final PE/COFF `.efi` linking is still missing, so `BOOTX64.EFI` is not produced yet
- on this Apple Silicon host, `VBoxManage list systemproperties` currently reports `Supported platform architectures: ARMv8`, so the `x86_64` guest target cannot be boot-validated locally
- the ARMv8 developer lane now goes one step further locally: VirtualBox firmware can boot headless, open the staged VDI through AHCI, and emit a captured `VBox.log`
- with `lld` installed, the ARMv8 lane now emits a real `BOOTAA64.EFI`, but it is still a developer-lane shim rather than the true C++ HAL bridge

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
