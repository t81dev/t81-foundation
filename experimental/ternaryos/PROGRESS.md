# TernOS Implementation Progress

**Last updated:** 2026-03-10
**Commit:** `256f86fd`
**Branch:** `main`

Reference docs:
- Roadmap: [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
- RFC-00B0 (HAL): [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
- RFC-00B1 (MMU): [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)
- RFC-00B2 (Drivers): [docs/rfcs/RFC-00B2-device-drivers.md](../../docs/rfcs/RFC-00B2-device-drivers.md)

---

## Status by Phase

### Phase 1 — Bootloader & HAL ✅ COMPLETE

**Gate condition (v1.5):** TISC `NOP`/`HALT` executes with no host OS.
Status: hosted simulation passing; VirtualBox guest promotion is now the first concrete non-hosted target.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `hal/hal.hpp` | Public interface: `MemoryRegion`, `HardwareInterrupt`, `BootContext`, `hal_main`, I/O stubs | — |
| `hal/hal_main.cpp` | Ethics-first boot — validates `BootContext`, evaluates Θ₁–Θ₉, stubs T81VM handoff | 9 |
| `hal/interrupt_table.cpp` | Shadow binary dispatch table; `register_interrupt_handler`, `dispatch_interrupt`, `fire_simulated_interrupt` | (above) |
| `hal/hosted_stub.cpp` | macOS/Linux UEFI stub simulation; synthetic memory map; calls `hal_main` | (above) |
| `hal/virtualbox_platform.hpp/.cpp` | First-target VirtualBox promotion scaffold: VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC profile validation, device-map descriptors, timer-tick simulation, and `BootContext` construction | 43 |
| `hal/virtualbox_guest_devices.hpp/.cpp` | VirtualBox guest-device binding seam: maps the first supported HAL storage and network profile onto AHCI/E1000-shaped Phase 4 adapters, rejects unsupported NVMe/PCNet promotion paths, and bootstraps the first guest profile as a reusable runtime bundle | 28 |
| `mmu/ternary_page_alloc.hpp/.cpp` | Physical page allocator; balanced-ternary `PageState` {Free=-1, Reserved=0, Allocated=+1}; `alloc_page`, `alloc_contiguous`, `free_page` | 28 |
| `sched/tisc_context.hpp` | `TiscContext` — full TISC thread snapshot; `ThreadState` {Sleeping=-1, Ready=0, Running=+1} | — |
| `sched/context_switch.hpp/.cpp` | `context_save` / `context_restore` / `context_yield` over `t81::vm::ThreadContext` | 43 |

#### Design notes

- Hosted HAL remains the executable path today, but Phase 1 promotion is now encoded as a concrete VirtualBox x86_64 hardware profile rather than a generic future boot target.
- The first supported VM profile is intentionally narrow: VBox EFI firmware, AHCI storage, E1000 networking, VMSVGA display, and HPET/IOAPIC timing.
- NVMe remains visible in the upstream VirtualBox source tree but is explicitly deferred behind AHCI for the first persistence gate.

**Phase 1 test total: 142 / 142**

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
| `dev/canon_store.hpp/.cpp` | Content-addressed CanonBlock store over `IBlockDevice`; dedup, flush, rebuild, corruption detection | 29 |
| `dev/framebuffer.hpp/.cpp` | 81×27 ternary framebuffer with ASCII dump | 18 |
| `dev/ttf.hpp/.cpp` | Minimal Ternary Text Format codec + framebuffer text renderer for ASCII terminal output | 12 |
| `dev/net_packet.hpp` | Ternary Ethernet packet wrapper with payload validation, canonical content hash, and binary frame encode/decode | 18 |
| `demo.cpp` | Presentation demo: VirtualBox guest bootstrap over hosted storage binding, reboot-persistent CanonStore, TTF framebuffer output, and Ethernet frame round-trip | — |
| `tests/device_driver_test.cpp` | Phase 4 acceptance tests AC-D1 through AC-D8 plus VirtualBox AHCI/E1000 adapter scaffolds, hosted TTF rendering, and Ethernet frame translation checks | 138 |

#### Design notes

- Logical block size is fixed at 729 bytes (one CanonBlock / 3^6 trytes), keeping device I/O aligned with CanonFS primitives.
- `CanonStore` reserves LBA 0 for a single-block index header (`CST1`), leaving data blocks at LBA 1+; the current Phase 4 cap is 17 unique entries.
- `HostedBlockDev::save()` / `load()` provides the current reboot-cycle simulation path for the v2.0 gate.
- `ttf_encode_ascii()` / `ttf_render_text()` now provide a minimal hosted TTF terminal path over the ternary framebuffer.
- The network layer now translates between ternary packet structs and a binary Ethernet-like frame format, but no RX/TX device path exists yet.
- AHCI is now represented as a first-target VirtualBox adapter scaffold, and the HAL-side VirtualBox profile can bind its storage path through that adapter while still delegating to hosted storage underneath.
- E1000 is now represented as a first-target VirtualBox NIC scaffold, and the HAL-side VirtualBox profile can bind its guest network path through that adapter while still using hosted loopback queues underneath.
- The demo path now boots through the VirtualBox guest bootstrap twice, showing the same hosted persistence story through the VM-targeted HAL/device seam rather than through a manually assembled hosted device stack.
- Real NVMe/ethernet hardware adapters remain open.

**Phase 4 test total: 138 / 138**

---

### Phase 5 — Userland Ecosystem 🔲 NOT STARTED (deferred v2.x)

1. Ternary Shell (TUI) — pure TISC CLI
2. Canonical TCP/IP translation stack

---

## Test Summary

| Test binary | Assertions | Phase |
| :--- | :---: | :---: |
| `t81_ternaryos_hal_boot_test` | 71 | 1 |
| `t81_ternaryos_page_alloc_test` | 28 | 1 |
| `t81_ternaryos_context_switch_test` | 43 | 1 |
| `t81_ternaryos_mmu_test` | 47 | 2 |
| `t81_ternaryos_scheduler_test` | 120 | 3 |
| `t81_ternaryos_ipc_test` | 73 | 3 |
| `t81_ternaryos_device_driver_test` | 138 | 4 |
| **Total** | **517** | |

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
| OQ-1 | Guest image format for the first VirtualBox target: raw disk, ISO, or another VBox-friendly package | Phase 1 promotion |
| OQ-2 | First supported VirtualBox device profile is intentionally narrow (VBox EFI + AHCI + E1000 + VMSVGA + HPET/IOAPIC); implementation still needs to be scoped into concrete tasks | Phase 1 promotion |
| OQ-3 | CI target remains unresolved: headless VirtualBox vs. QEMU for automation, with VirtualBox reserved for demo/dev validation | Phase 1 promotion |
| OQ-4 | TISC interrupt semantics — frozen ISA has no trap-return opcode; shadow dispatch table is the current workaround | Phase 4 |
| OQ-5 | Axion determinism under pre-emption — governance model must be extended for async context switches | Phase 3 |
| OQ-6 | Phase 3 radix-trie page table (3-ary, 10-trit levels) — not yet designed | Phase 3 |
| OQ-7 | Real AHCI / E1000 / VMSVGA-facing adapters are not implemented; Phase 4 currently satisfies the gate only in hosted simulation | Phase 4 promotion |
| OQ-8 | CanonStore index is single-block and capped at 17 entries; chained index pages or a larger metadata format are deferred | Phase 4 scaling |
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
