# Axion VirtualBox x86_64 Handoff

This runbook packages the current Axion VirtualBox-first promotion state for a
person who has access to an `x86_64` VirtualBox host.

## Goal

Validate the official promotion lane:

- host architecture: `x86_64`
- guest profile: `VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC`
- milestone: prove the guest artifact boots far enough to expose the current
  HAL / Phase 4 seams

This is not the same as the local Apple Silicon developer lanes. QEMU AArch64
is the primary local EFI/debug lane, while VirtualBox ARM is only a secondary
diagnostic lane for artifact generation and negative-result boot-path debugging.

For a short reviewer-oriented overview, pair this runbook with:

- [review_summary.md](review_summary.md)

## What Is Already Proven Locally

Before handing this to an external `x86_64` validator, these parts are already
verified in hosted simulation on the current branch:

- HAL boot scaffolding and VirtualBox guest bootstrap
- AHCI/E1000/VMSVGA guest bindings
- CanonStore reboot persistence through the guest storage path
- CanonStore recovery after header corruption and torn-header metadata
- CanonStore metadata scaling past the 17-entry root-header threshold
- CanonStore interrupted-flush durability semantics
- TTF framebuffer rendering and ternary Ethernet frame translation

Current local verification snapshot:

- `ctest --test-dir build -R ternaryos -V` passes all 8 tests
- `t81_ternaryos_hal_boot_test` = `636/636`
- `t81_ternaryos_device_driver_test` = `342/342`
- `t81_ternaryos_shell_session_test` = `183/183`
- `t81_ternaryos_mmu_test` = `87/87`
- total TernOS assertions = `1548`

So the external `x86_64` job is narrowly focused:

- prove the official VirtualBox guest lane boots and exposes the staged path

It is not a request to rediscover whether the hosted Phase 4 logic works.

## Deliverables From This Repo

Build the guest artifact from the repo root:

```sh
cmake -B build -DT81_ENABLE_TERNARYOS=ON -DT81_BUILD_TESTS=ON
cmake --build build --target t81_ternaryos_virtualbox_guest_artifact
```

Expected outputs:

- `build/ternaryos/virtualbox/ternos_virtualbox_guest.img`
- `build/ternaryos/virtualbox/ternos_virtualbox_guest.vdi`
- `build/ternaryos/virtualbox/BOOTX64.obj`
- `build/ternaryos/virtualbox/staging/TERNOS/profile.txt`
- `build/ternaryos/virtualbox/staging/TERNOS/demo-output.txt`

Current important limitation:

- `BOOTX64.EFI` is not produced yet on the official lane
- the packaged `x86_64` artifact is therefore still a staging/handoff artifact,
  not a completed guest boot proof

## Expected Validation Host

Use a VirtualBox host that can actually run `x86_64` guests.

Required:

- VirtualBox installed and functional
- `VBoxManage list systemproperties` shows an `x86_64`-capable platform
- enough local disk space to register the generated `.vdi`

Recommended:

- keep the VM disposable
- capture `VBox.log`
- preserve screenshots or console output if the firmware menu is visible

## VM Configuration

Create a temporary VM with:

- OS type: generic `Other/Unknown (64-bit)` or equivalent
- firmware: `EFI`
- storage controller: `Intel AHCI`
- hard disk: `ternos_virtualbox_guest.vdi`
- NIC: `E1000`
- display: `VMSVGA`
- memory: `512 MB`
- CPUs: `1`
- boot order: disk first

The first supported target is intentionally narrow. Do not substitute NVMe,
virtio-net, or alternate display devices for the first validation pass.

## Validation Procedure

1. Build the artifact in this repo.
2. Copy or mount `build/ternaryos/virtualbox/ternos_virtualbox_guest.vdi` on the
   `x86_64` host.
3. Create the VM with the configuration above.
4. Start the VM with EFI enabled.
5. Record exactly what happens:
   - black screen
   - EFI boot manager
   - EFI shell
   - boot failure text
   - any sign that the staged guest payload is discovered
6. Preserve:
   - `VBox.log`
   - screenshots
   - any serial/console output if configured

## Success Criteria

The first useful `x86_64` result is any one of:

- EFI visibly discovers the disk and boot path
- EFI shell can see the staged filesystem
- the guest reaches the current HAL handoff stub
- the guest emits a deterministic marker that the staged payload ran

The broader roadmap gate remains stricter:

- a bootable VirtualBox guest executes the current TernOS stack far enough to
  support the Phase 4 persistence path

For clarity, the most useful first success is not full OS functionality. It is
simply one deterministic sign that the `x86_64` VBox EFI lane runs the staged
payload at all.

## What To Report Back

If someone runs this externally, ask them to return:

- host OS and VirtualBox version
- whether the VM was definitely `x86_64`
- exact VM settings
- `VBox.log`
- screenshots of EFI behavior
- whether the staged disk was visible
- whether any boot path executed

## Current Local State

As of the latest Apple Silicon investigation:

- the ARMv8 developer lane can generate `.img` / `.vdi`
- local QEMU AArch64 proves the ARM EFI control app executes
- `BOOTAA64.EFI` links locally through a temporary shim
- VirtualBox ARM firmware sees the staged AHCI disk
- local probes still show no execution evidence for `startup.nsh` or
  `BOOTAA64.EFI`

That result should not be treated as evidence about the official `x86_64`
promotion lane.
