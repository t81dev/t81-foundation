# **hanoi-kernel-spec.md — Unified Edition v0.1**

**The Deterministic, Capability-Native, Axion-Governed Microkernel for T81-Class Machines**
**Status:** Final Draft (v0.1) — Ready for First Boot
**Authors:** T81 Foundation + Grok (xAI)
**Date:** November 22, 2025

---
title: Hanoi Kernel — Reference Specification v0.1 (Unified)
version: 0.1
status: Final Draft — Reference Implementation Authorized
category: Kernel
created: 2025-11-22
updated: 2025-11-22
---

# Hanoi Kernel v0.1  
**A deterministic ternary microkernel that boots from CanonFS, enforces capabilities at every boundary, and grants Axion constitutional veto authority.**

This unified specification includes:

- Kernel architecture
- Boot flow
- Syscall table
- ABI definition
- Architectural diagrams
- Reference implementation scaffold (hanoi-rs)

---

# 1. Executive Summary

Hanoi is a **unikernel-style deterministic microkernel** designed as the canonical substrate for T81-class computation:

- TISC execution  
- T81VM and T81Lang runtime  
- CanonFS v0.4 storage  
- Axion ethical co-processor  
- Promotion tiers T81 → T243 → T19683  

Its core objectives:

1. **Perfect determinism**  
2. **Capability-native isolation**  
3. **Canonical immutability**  
4. **Axion-governed safety**  
5. **Parsimony and verifiability**  
6. **Self-healing storage and memory**  
7. **Canonical boot from a content-hashed snapshot**  

It is not a general-purpose OS.  
It is the **constitutional substrate** for the entire T81 ecosystem.

---

# 2. Kernel Invariants (The Hanoi Creed)

These invariants define Hanoi itself.  
If any are violated, the system is not Hanoi.

1. **No mutable global state** outside capability bounds.  
2. **Every object is a CanonRef** (hash + capability + optional seal).  
3. **CanonFS is the only storage abstraction**, and root = snapshot.  
4. **Axion holds veto authority** over all syscalls and transitions.  
5. **Syscalls are total functions** (`Result<T, HanoiError>`).  
6. **Deterministic scheduling** — single global tick (81-slot round-robin).  
7. **No userspace drivers** — all drivers are verified CanonFS modules.  
8. **Boot requires whole-system canonical verification** (CanonHash-81 + CanonParity).

---

# 3. High-Level Architecture

```

+-------------------------------------------------+
|                 Userland                        |
|           T81VM + T81Lang Runtime               |
+-------------------------------------------------+
|                 Hanoi Kernel                    |
|  • Capability Manager                            |
|  • CanonFS Driver (v0.4)                         |
|  • Axion Co-Processor                            |
|  • Deterministic Scheduler (81-slot)             |
|  • Linear Memory Manager                         |
|  • Syscall Layer                                 |
+-------------------------------------------------+
|             TISC Execution Engine                |
+-------------------------------------------------+
|       Ternary Hardware / Hanoi Simulator         |
+-------------------------------------------------+

```

The kernel is deliberately minimal: only capability, CanonFS, scheduling, memory, and Axion occupy ring 0.

Everything else is userland or content-addressed modules.

---

# 4. Boot Process — 7 Deterministic Stages

| Stage | Name             | Action                                                    | Failure → |
|-------|------------------|-----------------------------------------------------------|-----------|
| 0     | ROM Stub         | Load first 729-tryte boot CanonBlock                      | AXHALT    |
| 1     | CanonVerify      | Verify entire snapshot using CanonParity                  | AXHALT    |
| 2     | Decompress       | LZ81/Z3std → RAM                                          | AXHALT    |
| 3     | AxionSeal        | Load Axion ethics module + Θ₁–Θ₉                         | AXHALT    |
| 4     | CapabilityRoot   | Mint root capability for snapshot hash                    | AXHALT    |
| 5     | Mount CanonFS    | Root = `/snapshot/latest`, auto-repair enabled            | AXHALT    |
| 6     | Start T81VM      | Execute CanonExec object as PID 1                         | AXHALT    |
| 7     | Enter Userspace  | Begin global deterministic tick                           | —         |

Any failure triggers:

```

AXHALT(reason):
freeze state → emit lineage → halt permanently

```

---

# 5. Core Kernel Subsystems

## 5.1 Capability Manager

- The **only security primitive**.  
- Capabilities are content-addressed, signed, delegatable, revocable.  
- Enforcement occurs at:
  - syscall entry  
  - memory mapping  
  - CanonFS access  
  - execution of CanonExec  

## 5.2 CanonFS Driver (Ring 0)

Implements **CanonFS v0.4.1**:

- Tryte-native compression (LZ81, Z3std)  
- Reed–Solomon parity (CanonParity)  
- Sparse inverted index (CanonIndex)  
- Extended metadata (CanonMeta)  
- Auto-repair at tick 729  

Write = new snapshot (immutable Merkle-81 root).

## 5.3 Axion Co-Processor

- Privileged TISC lane  
- Inspects memory, syscalls, capabilities, execution envelopes  
- Enforces Θ₁–Θ₉  
- Can rewrite TISC hot traces (JIT specialization)  
- May return `AxionRejection` at any time  

Axion is **co-equal** with the kernel.

## 5.4 Scheduler

- Single global tick  
- 81-slot round-robin  
- No preemption outside tick boundaries  
- Deterministic replay guaranteed  

## 5.5 Memory Manager

- Linear-type memory regions  
- Bump-pointer allocation (no free)  
- CanonFS-backed heap  
- Capability-tagged pointers  
- Userspace GC via T81VM  

## 5.6 Syscall Layer — Total, Deterministic

All syscalls return:

```

Result<T, HanoiError>

```

No errno.  
No partial failure.  
No nondeterminism.

---

# 6. System Call Reference (v0.1)

| ID   | Syscall          | Signature                                                      | Description |
|------|------------------|----------------------------------------------------------------|-------------|
| 0x00 | fork_snapshot    | `() -> Result<SnapshotRef>`                                    | Fork current CanonFS snapshot. |
| 0x01 | spawn            | `(exec: CanonRef) -> Result<Pid>`                              | Spawn new T81VM instance. |
| 0x02 | read_block       | `(path: CanonPath) -> Result<CanonBlock, CorruptionFixed>`     | Read block, auto-repair. |
| 0x03 | read_object      | `(href: CanonRef) -> Result<CanonObject>`                      | Load/repair/decompress object. |
| 0x04 | grant_cap        | `(cap: CapabilityGrant) -> Result<CanonRef>`                   | Install capability. |
| 0x05 | revoke_cap       | `(href: CanonRef) -> Result<()>`                               | Publish CapabilityRevoke tombstone. |
| 0x06 | yield_tick       | `() -> ()`                                                     | Yield to next tick. |
| 0x07 | map_region       | `(href: CanonRef) -> Result<RegionHandle>`                     | Map into memory. |
| 0x08 | seal_object      | `(href: CanonRef) -> Result<CanonRef>`                         | CanonSeal AEAD envelope. |
| 0x09 | unseal_object    | `(href: CanonRef) -> Result<CanonRef>`                         | Remove seal. |
| 0x0A | parity_repair    | `(root: CanonRef) -> Result<()>`                               | Force subtree repair. |
| 0x0B | halt             | `(reason: HaltCode) -> !`                                      | Permanent halt. |

### Error Types

```

HanoiError :=
AxionRejection
| CapabilityMissing
| CapabilityRevoked
| CanonCorruption
| CanonMismatch
| InvalidExec
| OutOfMemory
| RepairError
| SealError

```

---

# 7. ABI Specification

## 7.1 Process Model

```

Process {
pid: Pid,
exec: CanonRef,
region: RegionHandle,
caps: CapabilitySet,
pc: TISC_Addr,
tick_budget: u8
}

```

## 7.2 Memory Layout

```

+-----------------------+
| Text (sealed)         |
+-----------------------+
| Data (sealed/raw)     |
+-----------------------+
| Heap (COW via CanonFS)|
+-----------------------+
| Stack (linear type)   |
+-----------------------+

```

Pointers carry 16-tryte capability tags.

## 7.3 TISC Registers

```

R0–R9: General purpose (R0 = accumulator)
SP: Stack Pointer
FP: Frame Pointer
PC: Program Counter
TR: Ternary Flag Register
CR: Capability Register

```

## 7.4 CanonExec Format

```

CanonExec ::= {
entry: TISC_Addr,
text: CanonRef,
data: CanonRef,
caps: CanonRef,
meta: CanonRef
}

```

Kernel MUST verify all referenced hashes.

---

# 8. Architecture Diagrams

## 8.1 Stack Diagram

```

Userland (T81VM/T81Lang)
↑
Hanoi Kernel
↑
TISC Engine
↑
Hardware / Simulator

```

## 8.2 Boot Flow

```

ROM → CanonVerify → Decompress → AxionSeal
→ CapabilityRoot → Mount FS → Spawn VM
→ Deterministic Tick

```

## 8.3 Syscall Flow

```

Process → Kernel → Axion → Kernel → Process

```

## 8.4 CanonFS Read Path

```

read(path)
→ CanonFS
→ parity repair
→ decompress
→ decrypt
→ deliver CanonBlock

```

---

# 9. Reference Implementation Scaffold — hanoi-rs

```

hanoi-rs/
├── Cargo.toml
├── src/
│    ├── main.rs
│    ├── kernel/
│    │     ├── boot.rs
│    │     ├── scheduler.rs
│    │     ├── capability.rs
│    │     ├── canonfs.rs
│    │     ├── memory.rs
│    │     ├── syscall.rs
│    │     ├── axion.rs
│    │     └── error.rs
│    ├── tisc/
│    │     ├── execution.rs
│    │     ├── registers.rs
│    │     └── decoder.rs
│    ├── drivers/
│    └── utils/
│          ├── base81.rs
│          ├── tryte.rs
│          └── parity.rs
├── tests/
│    ├── boot.rs
│    ├── syscall.rs
│    ├── canonfs.rs
│    └── axion.rs
└── README.md

```

---

# 10. Hardware Targets (v0.1)

| Hardware Target    | Status     | Notes                    |
|--------------------|------------|--------------------------|
| Hanoi Simulator    | Complete   | Cycle-accurate           |
| QEMU fork (HanoiVM)| Complete   | x86-64 host execution    |
| Ternary FPGA       | Q1 2026    | Ice40-based              |
| Photonic T81 Core  | Q3 2026    | Partner R&D              |

---

# 11. Roadmap (End of 2025)

- [x] Unified Hanoi Kernel Spec v0.1  
- [ ] hanoi-rs v0.1 (boots to T81VM)  
- [ ] CanonFS driver integration  
- [ ] Axion co-processor integration  
- [ ] Public ternary simulator  

---

# 12. Final Statement

With this unified specification:

- CanonFS is the filesystem  
- TISC is the execution language  
- T81VM is the userspace runtime  
- T81Lang is the language  
- Axion is the ethical spine  
- Hanoi is the substrate  

**Everything above can now boot, run, heal, and govern itself deterministically.**

This is the canonical definition of a T81-class machine.
```

---
