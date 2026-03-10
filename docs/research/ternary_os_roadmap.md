# Ternary Operating System: Implementation Roadmap

This document captures the current implementation state, completed milestones, remaining work, and exit criteria for evolving the T81 Foundation stack into a standalone Ternary Operating System (TernOS).

## 1. Current Position

The T81 Foundation provides a determinism-first, ternary-native computing stack (`T81`) that achieves native ternary scaling properties (base-3 / base-81) simulated over standard hardware. Today, TernOS runs as a hosted prototype over standard binary platforms, but the standalone operating-system stack is no longer purely aspirational: the boot/HAL layer, ternary MMU prototype, and scheduler/IPC foundation are implemented and tested.

As of **2026-03-10**, the project has completed:

- **Phase 1:** Bootloader & HAL hosted simulation
- **Phase 2:** Ternary MMU prototype
- **Phase 3:** Kernel scheduling and IPC

The roadmap is now centered on promotion of those layers from `experimental/` into mainline, plus delivery of the Phase 4 driver layer needed for a reboot-persistent CanonFS system.

---

## 2. Implemented Inventory

The foundational computational, storage, and process primitives already exist in-repo.

### Compute Layer

| Component | Location | Status |
| :--- | :--- | :--- |
| TISC ISA (frozen) | `spec/tisc-spec.md`, `core/` | Normative / Frozen |
| T81VM interpreter | `core/vm/vm.cpp` | Beta |
| T81Lang compiler | `lang/frontend/` | Beta |
| T81BigInt / T81Float | `core/types/` | Beta |

### Storage & Governance Layer

| Component | Location | Status |
| :--- | :--- | :--- |
| CanonFS | `src/canonfs/`, `include/t81/canonfs/` | Beta |
| CanonHash-81 / CanonHash384 | `include/t81/tracing/canonhash.hpp`, `runtime/tracing/canonhash81.cpp` | Beta |
| Reed-Solomon 3+2 (GF(3^9)) | `include/t81/canonfs/rs_repair.hpp`, `gf3_9.hpp` | Beta |
| Axion Policy Kernel | `kernel/axion/` | Beta |

### Process & Memory Foundations

| Component | Location | Notes |
| :--- | :--- | :--- |
| Hanoi 81-slot scheduler | `experimental/hanoi/in_memory_kernel.cpp` (`kMaxSlots = 81`) | Ethics-first boot; `SchedulerFull` guard |
| TieredMemoryPool (10-phase) | `include/t81/memory/advanced_memory_manager.hpp` (`TieredMemoryPool`, line 57) | Nanosecond allocation timing |
| RegisterFrame / frame stack | `include/t81/vm/state.hpp` (`RegisterFrame`, line 204) | Infrastructure for context-switch discipline |
| Cognitive Tiers 1–6 | `experimental/tiers/cog/` | Tier6 distributed monads, Θ₇ containment |

### TernOS Prototype Layers

| Component | Location | Status |
| :--- | :--- | :--- |
| HAL interface + hosted boot stub | `experimental/ternaryos/hal/` | Complete for Phase 1 hosted gate |
| Ternary page allocator | `experimental/ternaryos/mmu/ternary_page_alloc.*` | Complete |
| TVA + flat page table MMU | `experimental/ternaryos/mmu/` | Complete for Phase 2 |
| TISC context switch machinery | `experimental/ternaryos/sched/context_switch.*` | Complete |
| 81-slot run queue + scheduler | `experimental/ternaryos/sched/` | Complete for Phase 3 |
| CanonRef-based IPC bus | `experimental/ternaryos/ipc/` | Complete |

---

## 3. Phase Status

TernOS still follows a five-phase dependency chain, but the first three phases are now implemented in hosted form.

### Phase 1 — Bootloader & HAL

**Status:** Complete in hosted simulation.  
**Gate:** v1.5 hosted equivalent satisfied; bare-metal UEFI promotion remains open.

- **Binary-to-Ternary Bootstrap:** A minimalist Type-1 hypervisor guest or UEFI bootloader that initializes hardware (CPU, memory, storage, network) and transfers full control to `T81VM` with no host OS residual.
- **Hardware Abstraction Layer (HAL):** Translates binary CPU interrupts, I/O port signals, and physical memory addresses into canonical TISC representations. See [RFC-00B0: HAL Specification](../rfcs/RFC-00B0-hal-spec.md) for the scoping decision between unikernel, Type-1 hypervisor, and raw UEFI approaches.

Implemented outcome:

- `hal_main` validates `BootContext`, performs ethics-first boot checks, and stubs T81VM handoff.
- A hosted macOS/Linux boot stub provides a synthetic memory map and invokes the HAL entrypoint.
- Shadow binary interrupt dispatch is implemented without modifying the frozen TISC ISA.

### Phase 2 — Ternary Memory Management

**Status:** Complete for the flat page-table prototype.  
**Gate:** v1.6 satisfied in hosted tests.

- **Ternary Paging:** Virtual memory where page boundaries are powers of 3 (e.g., 3¹⁰ = 59,049 trytes per page). Requires an address translation design that maps the host's 48-bit virtual address space to a trit-addressed space — this is non-trivial and warrants a dedicated RFC.
- **Process Isolation:** Axion engine extended to enforce rigid memory sandboxing between isolated TISC processes. `RegisterFrame` (line 204, `vm/state.hpp`) provides the per-thread register snapshot; wiring it to a context-switch path is the primary implementation target.
- **Allocator Integration:** `TieredMemoryPool` (`advanced_memory_manager.hpp:57`) becomes the kernel allocator; its 10-phase tier model maps naturally to Cognitive Tier boundaries.

Implemented outcome:

- RFC-00B1 defines a 30-trit TVA layout with a 10-trit page offset and 20-trit VPN.
- `TernaryPageAllocator` manages physical pages using balanced-ternary page states.
- The current MMU uses a flat `VPN -> PageTableEntry` map; the radix-trie follow-up is deferred.

### Phase 3 — Kernel Scheduling & IPC

**Status:** Complete.  
**Gate:** v1.7 satisfied with deterministic multi-thread scheduling tests.

- **Pre-emptive Ternary Scheduler:** Generalize the Hanoi 81-slot cooperative scheduler (`kMaxSlots = 81`, `in_memory_kernel.cpp:125`) into a pre-emptive, priority-aware dispatcher managing multiple concurrent `T81VM` instances.
- **Ternary Context Switching:** Pause a TISC thread mid-execution, snapshot ternary registers and flags via `RegisterFrame`, and restore them on reschedule.
- **IPC via CanonFS:** Define canonical message-passing contracts using CanonFS object identity — processes exchange CanonRef handles rather than raw pointers, preserving determinism and Axion audit trails.

Implemented outcome:

- The run queue preserves deterministic insertion-order round-robin behavior across 81 slots.
- `Scheduler::tick()` performs save/preempt/select/restore and returns `true` only for genuine thread switches.
- IPC is implemented as CanonRef-safe FIFO inboxes with per-recipient depth caps.

### Phase 4 — Device Drivers & I/O

**Status:** Not complete. This is the active major roadmap item.

- **Binary Protocol Wrappers:** PCIe, NVMe, and Ethernet drivers receive binary streams and translate them into canonical ternary structs at the system boundary.
- **Ternary Text Format (TTF):** Character encoding standard bridging ASCII/UTF-8 into packed trytes for framebuffer and terminal rendering.

Near-term deliverables:

- NVMe wrapper and block-device bridge for CanonFS
- Framebuffer and TTF terminal path
- Ethernet wrapper and ternary packet representation
- Reboot-cycle validation for CanonFS durability

### Phase 5 — Userland Ecosystem (deferred to v2.x)

**Status:** Deferred until the driver layer is stable.

- **Ternary Shell (TUI):** Deterministic CLI running pure TISC code for process, volume, and policy management.
- **Network Stack:** Canonical TCP/IP translation layer for deterministic network routing over binary hardware.

---

## 4. Phased Delivery Plan

| Phase | Milestone | Key Deliverable | Gate Condition | Target |
| :---: | :--- | :--- | :--- | :--- |
| 1 | HAL RFC + Bootloader PoC | Hosted HAL, boot validation, interrupt shim | TISC `NOP`/`HALT` executes with no host OS-equivalent supervisor path | v1.5 |
| 2 | Ternary MMU | TVA model, page allocator, flat page table | T81VM allocates from ternary page boundaries | v1.6 |
| 3 | Pre-emptive Scheduler | 81-slot run queue, context-switch, IPC | Two concurrent TISC threads run deterministically | v1.7 |
| 4 | Driver Layer | NVMe + framebuffer + network wrappers | CanonFS read/write survives a reboot cycle | v2.0 |
| 5 | Userland | Ternary shell + network stack | Interactive TISC session over bare metal | v2.x |

Current completion snapshot:

- **Phases 1-3:** Implemented and passing in hosted tests
- **Phase 4:** In progress / next major milestone
- **Phase 5:** Deferred

---

## 5. Current Success Criteria

The original v1.5 boot criterion has been met in hosted form. The next material success criteria are:

> **Phase 4 (v2.0): CanonFS read/write survives a reboot cycle through the device-wrapper layer.**

That gate turns the current in-memory TernOS substrate into a minimally persistent operating system. In parallel, a promotion track remains open for the stricter bare-metal interpretation of the Phase 1 gate:

> **A UEFI or Type-1 hypervisor guest loads the T81VM binary, executes a TISC program (`NOP` loop or `HALT`), and terminates cleanly with no Linux/macOS host process involved.**

---

## 6. Open Questions & Risks

1. **Phase 1 promotion target remains unresolved:** RFC-00B0 still leaves open the concrete UEFI toolchain choice (`gnu-efi` vs. EDK2), QEMU-vs-hardware CI policy, and AArch64 scope for bare-metal promotion.
2. **The physical/virtual address gap is resolved only for the current prototype:** RFC-00B1 adopts a "narrow virtual" TVA design. If future requirements exceed the current 30-trit/205 TB space, a wider VPN design will be needed.
3. **TISC interrupt semantics remain a long-term architectural constraint:** The frozen ISA still has no trap-return opcode; the shadow dispatch table is sufficient for the prototype but may constrain richer interrupt handling in later phases.
4. **Determinism under pre-emption is not fully closed:** Scheduling is deterministic today, but Axion governance has not yet been fully extended to model async interleavings.
5. **The Phase 2 radix-trie page table is deferred:** The flat hash-map MMU is adequate for the current milestone but is not the final structure.
6. **Driver correctness becomes the next systems risk:** NVMe, framebuffer, and Ethernet wrappers must preserve canonical ternary representations at the boundary without leaking binary host assumptions upward.

---

## 7. Conclusion

The most mathematically complex components — TISC ISA, compiler toolchain, CanonHash-81, Reed-Solomon parity, and the Axion policy engine — are already shipped and Beta-stable. On the TernOS path specifically, the foundational OS substrate is now in place: hosted boot/HAL, ternary paging, deterministic scheduling, context switching, and CanonRef-safe IPC are implemented and tested.

The critical path has moved. The next milestone is the device-wrapper layer and reboot-persistent CanonFS validation, while Phase 1 promotion, Axion pre-emption semantics, and the radix-trie MMU remain follow-on engineering tracks.
