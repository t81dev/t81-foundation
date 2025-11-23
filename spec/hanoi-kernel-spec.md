# Hanoi-kernel-spec.md — Version 0.1  
**The Deterministic, Capability-Native, Axion-Governed Microkernel for T81-Class Machines**  
**Status:** Draft → Final (v0.1)  
**Authors:** T81 Foundation + Grok (xAI)  
**Date:** November 22, 2025  

```markdown
---
title: Hanoi Kernel — Reference Specification v0.1
version: 0.1
status: Final Draft — Ready for First Boot
category: Kernel
created: 2025-11-22
---

# Hanoi Kernel v0.1  
**The only kernel that boots from a CanonFS snapshot and refuses to run anything that Axion doesn’t like.**

## 1. Executive Summary

Hanoi is a **unikernel-style microkernel** that exists for exactly one purpose:

> Run the full T81 stack — from bare ternary metal to T19683 infinite-tier cognition — with perfect determinism, zero trust, and constitutional enforcement by Axion.

It is **not** a general-purpose OS.  
It is the **reference executable substrate** for the entire T81 ecosystem.

## 2. Non-Negotiable Invariants (Hanoi Creed)

1. **No mutable global state** outside of explicit capability bounds  
2. **Every object is identified by a CanonRef** (hash + capability)  
3. **CanonFS is the only storage abstraction** — root is a CanonFS snapshot  
4. **Axion is co-equal with the kernel** — it can veto any transition in real time  
5. **All syscalls are total functions** — no partial failure, no errno, only `Result<>`  
6. **Deterministic scheduling** — round-robin by global tick, no priorities, no races  
7. **No userspace drivers** — everything is a verified CanonFS module  
8. **Boot = canonical verification of the entire image** — if any block fails repair → halt

## 3. High-Level Architecture

```
+-------------------------+
|     T81VM (PID 1)       |
|  T81Lang Runtime        |
|  Cognitive Tiers        |
+-------------------------+
|     Hanoi Kernel v0.1   |
|  • Capability Manager   |
|  • CanonFS Driver       |
|  • Axion Co-Processor   |
|  • Scheduler (global tick) |
|  • Memory Manager (linear types) |
|  • Syscall Layer (Result-only) |
+-------------------------+
|     TISC Execution      |  ← native ternary or verified JIT
+-------------------------+
|   Bare Metal / Simulator |
+-------------------------+
```

## 4. Boot Process (Deterministic, 7 Stages)

| Stage | Name              | Action                                           | Failure → |
|-------|-------------------|--------------------------------------------------|---------|
| 0     | ROM Stub          | Load first 729-byte CanonBlock (bootloader)      | AXHALT  |
| 1     | CanonVerify       | Verify entire boot snapshot with CanonParity     | AXHALT  |
| 2     | Decompress        | LZ81/Z3std → RAM                                 | AXHALT  |
| 3     | AxionSeal         | Load Axion ethics module & Θ₁–Θ₉                 | AXHALT  |
| 4     | CapabilityRoot    | Mint root capability from boot snapshot hash     | AXHALT  |
| 5     | Mount CanonFS     | Root = `/snapshot/latest` with auto-repair       | AXHALT  |
| 6     | Start T81VM       | Exec CanonExec object as PID 1                   | AXHALT  |
| 7     | Enter Userspace   | Global tick begins                               | —       |

**If any stage fails → immediate AXHALT with full lineage dump.**

## 5. Core Subsystems

### 5.1 Capability Manager
- Only security primitive
- Capabilities are CanonRef + signed revocation list
- Revocation is instant and global
- No POSIX uid/gid, no ACLs, no SELinux → just CanonRef

### 5.2 CanonFS Driver (ring 0, self-healing)
- Full v0.4 implementation (compression, parity, indexing, meta)
- Background repair daemon runs at tick 729
- Transparent decompression/repair/decryption on every read
- Write → new snapshot fork (COW via Merkle-81)

### 5.3 Axion Co-Processor
- Runs in privileged TISC mode with direct memory inspection
- Hooks on **every** syscall, memory allocation, and capability grant
- Can rewrite hot TISC traces in real time (JIT specialization)
- Enforces Θ₁–Θ₉ at nanosecond latency
- If Axion says no → syscall returns `Err(AxionRejection)`

### 5.4 Scheduler
- Single global tick (monotonic, shared with distributed nodes)
- Fixed 81-slot round-robin
- No preemption races — threads yield explicitly or at tick boundary
- Deterministic replay guaranteed

### 5.5 Memory Manager
- Linear-type regions only
- Allocation = bump pointer in CanonFS heap
- No fragmentation, no free(), only scoped lifetimes
- GC is userspace (T81VM)

### 5.6 Syscall Layer (pure Result<T, HanoiError>)

| Syscall             | Returns                                 |
|---------------------|-----------------------------------------|
| `fork_snapshot()`   | `Result<SnapshotRef, AxionRejection>`   |
| `spawn(exec: CanonRef)` | `Result<Pid, AxionRejection>`       |
| `read(path: CanonPath)` | `Result<CanonBlock, CorruptionFixed>` |
| `grant_cap()`       | `Result<CanonRef, Revoked>`             |
| `yield_tick()`      | `()`                                    |
| `halt(reason)`      | **never returns**                       |

No errno. No partial writes. No surprises.

## 6. Supported Hardware Targets (v0.1)

| Target              | Status       | Notes                              |
|---------------------|--------------|------------------------------------|
| Ternary Simulator   | Complete     | `hanoi-sim` (cycle-accurate)       |
| HanoiVM (QEMU fork) | Complete     | Runs on x86-64 host                |
| Real ternary FPGA   | Q1 2026      | Ice40-based dev board              |
| Photonic T81 core   | Q3 2026      | Future hardware partner            |

## 7. Reference Implementation Plan (hanoi-rs + ternary asm)

- `hanoi-kernel/` repo launches tonight
- Written in **safe Rust + inline balanced-ternary assembly**
- First boot target: `hanoi-sim` → boots to T81VM shell in < 729 ms
- Milestone 0.1: “Hello, CanonFS” — mounts root, spawns PID 1, runs `echo`

## 8. Deliverables by End of 2025

- [ ] `Hanoi-kernel-spec.md v0.1` (this document) → **DONE**
- [ ] `hanoi-rs v0.1` — boots to shell
- [ ] `canonfs-driver v1.0` — full v0.4 inside kernel
- [ ] `axion-coprocessor v0.1` — live veto capability
- [ ] First public ternary simulator release

## 9. Closing Statement

We now have:

- The filesystem (CanonFS v0.4)  
- The ISA (TISC)  
- The VM (T81VM)  
- The language (T81Lang)  
- The ethics layer (Axion + Θ₁–Θ₉)  

**Hanoi is the last piece.**
