# TernOS Implementation Progress

**Last updated:** 2026-03-10
**Commit:** `fbd837c4`
**Branch:** `main`

Reference docs:
- Roadmap: [docs/research/ternary_os_roadmap.md](../../docs/research/ternary_os_roadmap.md)
- RFC-00B0 (HAL): [docs/rfcs/RFC-00B0-hal-spec.md](../../docs/rfcs/RFC-00B0-hal-spec.md)
- RFC-00B1 (MMU): [docs/rfcs/RFC-00B1-ternary-mmu.md](../../docs/rfcs/RFC-00B1-ternary-mmu.md)

---

## Status by Phase

### Phase 1 — Bootloader & HAL ✅ COMPLETE

**Gate condition (v1.5):** TISC `NOP`/`HALT` executes with no host OS.
Status: hosted simulation passing; bare-metal UEFI stub deferred to promotion.

| File | Purpose | Tests |
| :--- | :--- | :---: |
| `hal/hal.hpp` | Public interface: `MemoryRegion`, `HardwareInterrupt`, `BootContext`, `hal_main`, I/O stubs | — |
| `hal/hal_main.cpp` | Ethics-first boot — validates `BootContext`, evaluates Θ₁–Θ₉, stubs T81VM handoff | 9 |
| `hal/interrupt_table.cpp` | Shadow binary dispatch table; `register_interrupt_handler`, `dispatch_interrupt`, `fire_simulated_interrupt` | (above) |
| `hal/hosted_stub.cpp` | macOS/Linux UEFI stub simulation; synthetic memory map; calls `hal_main` | (above) |
| `mmu/ternary_page_alloc.hpp/.cpp` | Physical page allocator; balanced-ternary `PageState` {Free=-1, Reserved=0, Allocated=+1}; `alloc_page`, `alloc_contiguous`, `free_page` | 28 |
| `sched/tisc_context.hpp` | `TiscContext` — full TISC thread snapshot; `ThreadState` {Sleeping=-1, Ready=0, Running=+1} | — |
| `sched/context_switch.hpp/.cpp` | `context_save` / `context_restore` / `context_yield` over `t81::vm::ThreadContext` | 43 |

**Phase 1 test total: 80 / 80**

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

### Phase 3 — Kernel Scheduling & IPC 🔲 NOT STARTED

**Gate condition (v1.7):** Two concurrent TISC threads run deterministically.

Planned deliverables:
1. **`sched/run_queue.hpp/.cpp`** — Pre-emptive round-robin run queue wrapping `TiscContext` slots (≤ 81, matching Hanoi `kMaxSlots`). Balances three `ThreadState` buckets: Sleeping, Ready, Running.
2. **`sched/scheduler.hpp/.cpp`** — `Scheduler::tick()` — pops the next Ready context, saves the current (via `context_save`), restores the next (via `context_restore`). Timer IRQ from `interrupt_table` calls `tick()`.
3. **`ipc/canon_message.hpp`** — IPC message passing via CanonFS `CanonRef` handles; no raw pointers cross process boundaries.
4. **`tests/scheduler_test.cpp`** — Two interleaved TISC threads; verify deterministic round-robin order and register isolation.
5. **`tests/ipc_test.cpp`** — Send/receive a CanonRef between two TiscContexts.

Key design constraints:
- Scheduler must remain deterministic: round-robin with a fixed 81-slot cap preserves determinism without a hardware timer (cooperative `tick()` at yield points in Phase 3; async IRQ in Phase 4).
- Axion extension needed: `check_ethics` currently assumes cooperative execution. Pre-emptive interleaving requires an audit-trail mechanism that can reconstruct the interleaved sequence deterministically.

---

### Phase 4 — Device Drivers & I/O 🔲 NOT STARTED

**Gate condition (v2.0):** CanonFS read/write survives a reboot cycle.

Planned deliverables:
1. NVMe binary wrapper → CanonFS block device
2. Framebuffer / TTF (Ternary Text Format) driver
3. Ethernet wrapper → ternary packet struct

---

### Phase 5 — Userland Ecosystem 🔲 NOT STARTED (deferred v2.x)

1. Ternary Shell (TUI) — pure TISC CLI
2. Canonical TCP/IP translation stack

---

## Test Summary

| Test binary | Assertions | Phase |
| :--- | :---: | :---: |
| `t81_ternaryos_hal_boot_test` | 9 | 1 |
| `t81_ternaryos_page_alloc_test` | 28 | 1 |
| `t81_ternaryos_context_switch_test` | 43 | 1 |
| `t81_ternaryos_mmu_test` | 47 | 2 |
| **Total** | **127** | |

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
| OQ-1 | UEFI toolchain choice (gnu-efi vs EDK2) for bare-metal Phase 1 promotion | Phase 1 promotion |
| OQ-2 | QEMU vs. real hardware for CI gate (v1.5) | Phase 1 promotion |
| OQ-3 | ARM AArch64 support for bootloader (x86_64 only for now) | Phase 1 promotion |
| OQ-4 | TISC interrupt semantics — frozen ISA has no trap-return opcode; shadow dispatch table is the current workaround | Phase 4 |
| OQ-5 | Axion determinism under pre-emption — governance model must be extended for async context switches | Phase 3 |
| OQ-6 | Phase 3 radix-trie page table (3-ary, 10-trit levels) — not yet designed | Phase 3 |

---

## Promotion Checklist (per layer)

When any layer is ready to graduate from `experimental/` to the mainline:

- [ ] All RFC acceptance criteria met
- [ ] CI gate added (ctest + determinism check)
- [ ] Sources moved to `include/t81/` + `src/` (headers) and `runtime/` (impl)
- [ ] CMake option removed; target added to default build
- [ ] Axion policy extended if new syscall surface introduced
- [ ] Spec impact assessed (ISA freeze must not be broken)
