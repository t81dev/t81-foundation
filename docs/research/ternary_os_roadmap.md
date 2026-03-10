# Ternary Operating System: Feasibility and Roadmap

This document explores the detailed requirements, current inventory, phased delivery plan, and exit criteria for evolving the T81 Foundation stack into a standalone, fully functional Ternary Operating System (TernOS).

## 1. Introduction

The T81 Foundation provides a determinism-first, ternary-native computing stack (`T81`) that achieves native ternary scaling properties (base-3 / base-81) simulated over standard hardware. While the current architecture operates as a Virtual Machine (`T81VM`) over a binary host OS, the ultimate theoretical target is a standalone Ternary OS capable of bare-metal or hypervisor execution.

This roadmap is scoped to what can be realistically achieved in incremental phases from the existing codebase, without discarding the frozen TISC ISA or Axion governance contracts.

---

## 2. Inventory: What We Already Have

We have successfully established the foundational computational layer and execution boundaries.

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

---

## 3. The Gap: What We Need to Accomplish

To transition from a hosted VM to a standalone OS, five engineering domains must be addressed. They are listed in dependency order — each phase is a prerequisite for the next.

### Phase 1 — Bootloader & HAL (prerequisite for everything)

- **Binary-to-Ternary Bootstrap:** A minimalist Type-1 hypervisor guest or UEFI bootloader that initializes hardware (CPU, memory, storage, network) and transfers full control to `T81VM` with no host OS residual.
- **Hardware Abstraction Layer (HAL):** Translates binary CPU interrupts, I/O port signals, and physical memory addresses into canonical TISC representations. See [RFC-00B0: HAL Specification](../rfcs/RFC-00B0-hal-spec.md) for the scoping decision between unikernel, Type-1 hypervisor, and raw UEFI approaches.

### Phase 2 — Ternary Memory Management

- **Ternary Paging:** Virtual memory where page boundaries are powers of 3 (e.g., 3¹⁰ = 59,049 trytes per page). Requires an address translation design that maps the host's 48-bit virtual address space to a trit-addressed space — this is non-trivial and warrants a dedicated RFC.
- **Process Isolation:** Axion engine extended to enforce rigid memory sandboxing between isolated TISC processes. `RegisterFrame` (line 204, `vm/state.hpp`) provides the per-thread register snapshot; wiring it to a context-switch path is the primary implementation target.
- **Allocator Integration:** `TieredMemoryPool` (`advanced_memory_manager.hpp:57`) becomes the kernel allocator; its 10-phase tier model maps naturally to Cognitive Tier boundaries.

### Phase 3 — Kernel Scheduling & IPC

- **Pre-emptive Ternary Scheduler:** Generalize the Hanoi 81-slot cooperative scheduler (`kMaxSlots = 81`, `in_memory_kernel.cpp:125`) into a pre-emptive, priority-aware dispatcher managing multiple concurrent `T81VM` instances.
- **Ternary Context Switching:** Pause a TISC thread mid-execution, snapshot ternary registers and flags via `RegisterFrame`, and restore them on reschedule.
- **IPC via CanonFS:** Define canonical message-passing contracts using CanonFS object identity — processes exchange CanonRef handles rather than raw pointers, preserving determinism and Axion audit trails.

### Phase 4 — Device Drivers & I/O

- **Binary Protocol Wrappers:** PCIe, NVMe, and Ethernet drivers receive binary streams and translate them into canonical ternary structs at the system boundary.
- **Ternary Text Format (TTF):** Character encoding standard bridging ASCII/UTF-8 into packed trytes for framebuffer and terminal rendering.

### Phase 5 — Userland Ecosystem (deferred to v2.x)

- **Ternary Shell (TUI):** Deterministic CLI running pure TISC code for process, volume, and policy management.
- **Network Stack:** Canonical TCP/IP translation layer for deterministic network routing over binary hardware.

---

## 4. Phased Delivery Plan

| Phase | Milestone | Key Deliverable | Gate Condition | Target |
| :---: | :--- | :--- | :--- | :--- |
| 1 | HAL RFC + Bootloader PoC | RFC-00B0 ratified; UEFI stub boots T81VM | TISC `NOP` executes with no host OS | v1.5 |
| 2 | Ternary MMU | Address translation RFC; ternary paging prototype | T81VM allocates from ternary page boundaries | v1.6 |
| 3 | Pre-emptive Scheduler | Generalized Hanoi scheduler + context-switch | Two concurrent TISC threads run deterministically | v1.7 |
| 4 | Driver Layer | NVMe + framebuffer binary wrappers | CanonFS read/write survives a reboot cycle | v2.0 |
| 5 | Userland | Ternary shell + network stack | Interactive TISC session over bare metal | v2.x |

---

## 5. Minimal Viable Boot Target (v1.5 Gate)

The first concrete success criterion is:

> **A UEFI or Type-1 hypervisor guest loads the T81VM binary, executes a TISC program (`NOP` loop or `HALT`), and terminates cleanly — with no Linux/macOS host process involved.**

This single milestone validates the bootloader and HAL and unlocks all subsequent phases.

---

## 6. Open Questions & Risks

1. **Host portability decision (UEFI vs. unikernel vs. Type-1):** Must be resolved in RFC-00B0 before Phase 1 begins. The three paths have very different scope and complexity.
2. **Ternary address space design:** A 48-bit binary virtual address does not divide cleanly into trit boundaries. Padding strategy or address-space narrowing must be specified.
3. **TISC interrupt semantics:** The frozen ISA has no interrupt or trap-return opcode. An extension mechanism (or a shadow binary dispatch table in the HAL) must be defined without breaking ISA immutability.
4. **Determinism under pre-emption:** The Axion engine currently assumes cooperative execution. Pre-emptive scheduling introduces non-deterministic interleaving — the governance model must be extended to bound this.

---

## 7. Conclusion

The most mathematically complex components — TISC ISA, compiler toolchain, CanonHash-81, Reed-Solomon parity, and the Axion policy engine — are already shipped and Beta-stable. The primary remaining effort is systems integration: a minimalist bootable abstraction (Phase 1), ternary virtual memory (Phase 2), pre-emptive scheduling (Phase 3), and binary I/O wrappers (Phase 4). Userland (Phase 5) is explicitly deferred to v2.x.

The critical path runs through RFC-00B0 (HAL scoping) and the ternary address-space design. Both should be initiated immediately.
