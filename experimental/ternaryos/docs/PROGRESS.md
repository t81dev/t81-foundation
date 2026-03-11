# Axion Implementation Progress

Current working release label: `Axion v0.1.0-alpha`

Working name note: the project-facing OS name is now `Axion`, while internal
paths, namespaces, and build/test identifiers still use `ternaryos`.

Current naming split:
- `T81 Foundation` = umbrella project/ecosystem
- `T81VM` = ternary runtime/execution substrate
- `Axion` = operating system
- `CanonFS` / `TISC` remain subsystem names

**Last updated:** 2026-03-11
**Commit:** `e1dfb42c`
**Branch:** `main`

Reference docs:
- Roadmap: [docs/research/ternary_os_roadmap.md](../../../docs/research/ternary_os_roadmap.md)
- RFC-00B0 (HAL): [docs/rfcs/RFC-00B0-hal-spec.md](../../../docs/rfcs/RFC-00B0-hal-spec.md)
- RFC-00B1 (MMU): [docs/rfcs/RFC-00B1-ternary-mmu.md](../../../docs/rfcs/RFC-00B1-ternary-mmu.md)
- RFC-00B2 (Drivers): [docs/rfcs/RFC-00B2-device-drivers.md](../../../docs/rfcs/RFC-00B2-device-drivers.md)
- x86_64 handoff: [virtualbox_x86_64_handoff.md](virtualbox_x86_64_handoff.md)
- Axion shell design: [axion_shell_design.md](axion_shell_design.md)

---

## Status by Phase

### Phase 1 — Bootloader & HAL ✅ COMPLETE

**Gate condition (v1.5):** TISC `NOP`/`HALT` executes with no host OS.
Status: hosted simulation passing; VirtualBox guest promotion is now the first concrete non-hosted target.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `hal/hal.hpp` | Public interface: `MemoryRegion`, `HardwareInterrupt`, `BootContext`, `hal_main`, I/O stubs | — |
| `hal/hal_c_abi.h/.cpp` | C-compatible HAL handoff bridge so freestanding guest stubs can construct a boot context without C++ STL dependencies | 2 |
| `hal/hal_main.cpp` | Ethics-first boot — validates `BootContext`, evaluates Θ₁–Θ₉, stubs T81VM handoff | 9 |
| `hal/interrupt_table.cpp` | Shadow binary dispatch table; `register_interrupt_handler`, `dispatch_interrupt`, `fire_simulated_interrupt` | (above) |
| `hal/hosted_stub.cpp` | macOS/Linux UEFI stub simulation; synthetic memory map; calls `hal_main` | (above) |
| `hal/virtualbox_platform.hpp/.cpp` | First-target VirtualBox promotion scaffold: VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC profile validation, device-map descriptors, timer-tick simulation, and `BootContext` construction | 43 |
| `hal/virtualbox_guest_devices.hpp/.cpp` | VirtualBox guest-device binding seam: maps the first supported HAL storage, network, and display profile onto AHCI/E1000/VMSVGA-shaped Phase 4 adapters, rejects unsupported NVMe/PCNet/VGA promotion paths, and bootstraps the first guest profile as a reusable runtime bundle | 39 |
| `hal/virtualbox_efi_stub.c` | Freestanding VBox EFI guest stub source: constructs the first guest `BootContext` via the C ABI bridge and serves as the source for the staged `BOOTX64.obj` artifact | — |
| `hal/virtualbox_armv8_efi_stub.c` | Freestanding ARMv8 VBox EFI guest stub source for the secondary developer lane; constructs an ARMv8 `BootContext` via the same C ABI bridge, emits the live serial banner and boot report, and now embeds a backend-generated Axion shell startup snapshot into the staged guest artifact | — |
| `hal/virtualbox_armv8_efi_control.c` | Ultra-minimal ARMv8 VBox EFI control application for the secondary developer lane; writes a standalone execution marker without touching the HAL bridge so Apple Silicon VirtualBox can be tested for "any EFI app runs at all" | — |
| `hal/virtualbox_armv8_efi_shim.c` | Temporary ARMv8 developer-lane EFI link shim: provides a freestanding `ternaryos_hal_main_c` stub so Apple Silicon hosts can emit a real `BOOTAA64.EFI` without claiming the true C++ HAL bridge is linked into PE/COFF yet | — |
| `scripts/build_virtualbox_guest_artifact.sh` | Reproducible VirtualBox guest-artifact pipeline: emits both the primary `x86_64` artifact lane and a temporary ARMv8 developer-lane artifact, stages `BOOTX64.obj` / `BOOTAA64.obj`, links a developer-lane `BOOTAA64.EFI`, and now places `STARTUP.NSH` at both the filesystem root and `EFI/BOOT` for ARM boot-path probing | — |
| `scripts/package_virtualbox_x86_64_handoff.sh` | Packages the official `x86_64` VirtualBox handoff lane into a portable bundle with the guest artifact, profile summary, demo output, and runbook for an external validator | — |
| `scripts/check_virtualbox_host.sh` | Host-capability check for local VirtualBox validation: reports whether the current machine can run the roadmap’s `x86_64` guest target or the temporary ARMv8 developer lane | — |
| `scripts/check_efi_link_toolchain.sh` | EFI-link capability check: records whether the current host has a PE/COFF linker (`lld-link` / `ld.lld`) available to turn staged EFI stub objects into final `.efi` applications | — |
| `scripts/probe_virtualbox_armv8_vm.sh` | Headless ARMv8 VirtualBox boot probe: registers a temporary VM, attaches the staged developer-lane VDI, boots VBox EFI, captures `VBox.log`, and confirms firmware-visible AHCI disk attachment before cleanup | — |
| `scripts/run_qemu_armv8_efi_control.sh` | QEMU AArch64 EFI control probe: boots the ARM developer-lane raw image under EDK2, captures serial output, and verifies whether the staged control EFI leaves its execution marker | — |
| `scripts/run_qemu_armv8_guest_probe.sh` | QEMU AArch64 guest bring-up probe: boots the staged ARM guest image under EDK2, requires the serial banner, and inspects the mutated probe image for `efi-ran.txt` plus `boot-report.txt` before validating the reported platform and `hal_main_result` | — |
| `mmu/ternary_page_alloc.hpp/.cpp` | Physical page allocator; balanced-ternary `PageState` {Free=-1, Reserved=0, Allocated=+1}; `alloc_page`, `alloc_contiguous`, `free_page` | 28 |
| `sched/tisc_context.hpp` | `TiscContext` — full TISC thread snapshot; `ThreadState` {Sleeping=-1, Ready=0, Running=+1} | — |
| `sched/context_switch.hpp/.cpp` | `context_save` / `context_restore` / `context_yield` over `t81::vm::ThreadContext` | 43 |

#### Design notes

- Hosted HAL remains the executable path today, but Phase 1 promotion is now encoded as a concrete VirtualBox x86_64 hardware profile rather than a generic future boot target.
- The first supported VM profile is intentionally narrow: VBox EFI firmware, AHCI storage, E1000 networking, VMSVGA display, and HPET/IOAPIC timing.
- NVMe remains visible in the upstream VirtualBox source tree but is explicitly deferred behind AHCI for the first persistence gate.
- A reproducible VirtualBox disk-artifact target now exists (`t81_ternaryos_virtualbox_guest_artifact`) and stages metadata, demo evidence, and a compiled `BOOTX64.obj` stub object; final `.efi` linking remains the blocking gap.
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
- Current QEMU guest-probe result is stronger than the earlier control-only lane: the staged ARM image reaches `BOOTAA64.EFI` directly (`boot_path_inference=default-bootaa64-efi`), leaves `TERNOS/efi-ran.txt`, writes `TERNOS/boot-report.txt` confirming `platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane` with `hal_main_result=0`, emits the visible serial banner `Axion ARMv8 EFI stub`, writes `TERNOS/startup-status.txt` exposing guest-visible Axion state (`phase=5`, `shell_mode=typed-builtins`, `virtualbox-ahci`, `virtualbox-vmsvga`, `virtualbox-e1000`), writes `TERNOS/startup-shell.txt` from a build-time snapshot generated by the real shell backend, writes `TERNOS/startup-session.txt` from the same backend as a compact `show session` snapshot with live profile and durable-state counts, writes `TERNOS/startup-history.txt` from that backend as a persisted durable-history view, writes `TERNOS/startup-store.txt` as a backend-derived `store ls` inventory snapshot, writes `TERNOS/startup-ref.txt` as a backend-derived `show ref <canonref>` object retrieval snapshot, and now writes `TERNOS/startup-report.txt` as a consolidated shell/session/object proof surface.
- Program decision: stop escalating local ARMv8 VirtualBox boot-layout experiments, use QEMU AArch64 as the primary local EFI/debug lane, and prepare the official `x86_64` acceptance lane as a handoff package instead; the runbook for that external validation path is now checked in.
- That handoff path is now reproducible as a single build target: the `x86_64` VirtualBox artifact, profile summary, demo transcript, and runbook can be packaged into a tarball for external validation on a real `x86_64` host.

**Phase 1 test total: 155 / 155**

---

### Phase 2 — Ternary MMU ✅ COMPLETE

**Gate condition (v1.6):** T81VM allocates from ternary page boundaries.
Status: flat page table implemented and tested; 3-ary radix trie deferred to Phase 3.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `mmu/tva.hpp` | `TernaryVirtualAddress` as `uint64_t` base-3 number; `kPageSize=59049` (3¹⁰), `kMaxVpn=3²⁰−1`, `kMaxTva=3³⁰−1`; `tva_vpn`, `tva_offset`, `tva_from_vpn_offset`, `tva_valid`, `trit_at`, `trit_weight`, `tva_to_string` | 22 |
| `mmu/page_table.hpp/.cpp` | Flat `VPN → PageTableEntry` hash map; `mmu_map` (alloc + insert), `mmu_translate` (lookup + offset), `mmu_unmap` (erase + free), `page_table_dump` | 25 |

**Address space design (RFC-00B1 §2):**
- Page offset: lower 10 trits → 3¹⁰ = 59,049 positions within a page
- VPN: next 20 trits → 3²⁰ = 3,486,784,401 virtual pages
- Total virtual space: 3³⁰ ≈ 205 TB (fits in `uint64_t`)
- Binary↔ternary gap: "narrow virtual" strategy — physical addresses stay as plain `uint64_t`; ternary structure lives entirely in the virtual address

**Phase 2 test total: 47 / 47**

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
- OQ-5 (Axion determinism under pre-emption) is still open — governance audit trail not yet extended for async interleaving.

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
| `shell/shell_session.hpp/.cpp` | Shared Phase 5 shell-session backend: executes a minimal typed shell command model (`help`, `profile`, `show profile`, `session status`, `session checkpoint`, `session show durable`, `show session`, `session refs`, `store put <text>`, `store put ref <ref>`, `store ls`, `store get <ref>`, `show ref <canonref>`, `store rm <ref>`, `history`, `history show session`, `history show durable`, `clear`) through the VirtualBox guest-bootstrap path, persists shell history through CanonStore, reboots, recovers it, and exposes deterministic transcript/framebuffer state to frontends | — |
| `apps/shell_demo.cpp` | Verbose Phase 5 backend proof: prints the built-in shell session and framebuffer evidence directly to stdout | — |
| `apps/shell_tui.cpp` | First FTXUI-based TernOS shell frontend with transcript pane, command buffer, session pane, builtins pane, framebuffer preview, interactive typed command execution, and `--snapshot` render mode | — |
| `shell/shell_startup_snapshot.cpp` | Build-time Axion shell snapshot generator: runs the real Phase 5 shell backend, emits compact startup-shell, startup-session, startup-history, startup-store, startup-ref, and startup-report artifacts, and produces the generated headers embedded by the ARM EFI developer-lane stub | — |
| `tests/shell_session_test.cpp` | Phase 5 shell acceptance test: scripted durable-history path plus interactive typed-command execution, object-native profile/session inspection, session-checkpoint persistence, durable-anchor inspection, quoted payload parsing, canonical object composition via `store put ref <ref>`, session-history inspection, store ref listing, store ref retrieval, durable ref removal, explicit durable-history inspection, transcript clearing, and parse-error handling through the guest-bootstrap seam | 100 |
| `docs/axion_shell_design.md` | Phase 5 shell design note: positioning, grammar, screen model, and milestone sequence for evolving the current built-in shell into a real Axion shell | — |

#### Design notes

- Phase 5 is now started, but still only as a narrow hosted scaffold.
- The current shell path is intentionally narrow: it now supports interactive typed command execution in the TUI, but it still does not execute TISC userland and does not introduce a syscall surface.
- Its value is architectural: user-facing output now rides the same guest-bootstrap storage and display seams already proven in Phase 4.
- The shell UI now uses the repo's established FTXUI stack instead of a one-off terminal surface, and `--snapshot` still gives it a deterministic review/debug mode.
- The old fixed transcript is gone: the current shell transcript now comes from command handlers for `help`, `profile`, `show profile`, `session status`, `session checkpoint`, `session show durable`, `show session`, `session refs`, `store put <text>`, `store put ref <ref>`, `store ls`, `store get <ref>`, `show ref <canonref>`, `store rm <ref>`, `history`, `history show session`, `history show durable`, and `clear`.
- The live TUI now accepts typed commands directly, including quoted `store put` payloads, and the shell test target now covers scripted durability, interactive typed command execution, session-state reporting, object-native session/ref inspection, shell-local durable ref inspection, explicit durable-history inspection, store ref inspection/removal, transcript clearing, and parser errors.
- The shell state now distinguishes the local session window from durable CanonStore state instead of treating them as one undifferentiated history stream, and the command surface now reflects that split explicitly.
- The shell now has its first object-native read/write grammar: `show profile`, `show session`, `show ref <canonref>`, `session show durable`, `history show session`, and `store put ref <ref>`.
- The next real Phase 5 milestone is moving from narrow built-ins to a broader user model while preserving the same durable-history and object-ref semantics.

---

## Test Summary

| Test binary | Assertions | Phase |
| :--- | :---: | :---: |
| `t81_ternaryos_hal_boot_test` | 84 | 1 |
| `t81_ternaryos_page_alloc_test` | 28 | 1 |
| `t81_ternaryos_context_switch_test` | 43 | 1 |
| `t81_ternaryos_mmu_test` | 47 | 2 |
| `t81_ternaryos_scheduler_test` | 120 | 3 |
| `t81_ternaryos_ipc_test` | 73 | 3 |
| `t81_ternaryos_device_driver_test` | 342 | 4 |
| `t81_ternaryos_shell_session_test` | 100 | 5 |
| **Total** | **837** | |

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
| OQ-4 | TISC interrupt semantics — frozen ISA has no trap-return opcode; shadow dispatch table is the current workaround | Phase 4 |
| OQ-5 | Axion determinism under pre-emption — governance model must be extended for async context switches | Phase 3 |
| OQ-6 | Phase 3 radix-trie page table (3-ary, 10-trit levels) — not yet designed | Phase 3 |
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
