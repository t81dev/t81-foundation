## title: Hanoi Kernel — Reference Specification v0.1.1 (Unified) version: 0.1.1 status: Final Draft — Reference Implementation Authorized category: Kernel created: 2025-11-22 updated: 2025-11-22

# Hanoi Kernel v0.1.1

**A Deterministic, Capability-Native, Axion-Governed Microkernel for T81-Class Machines**

This specification defines the complete behavior of the Hanoi kernel:
architecture, boot flow, syscall interface, ABI, deterministic entropy model,
CanonSeal key derivation, diagrams, and implementation scaffold.

This is the canonical v0.1.1 revision.

______________________________________________________________________

# 1. Executive Summary

Hanoi is a **deterministic ternary microkernel** serving as the constitutional OS substrate for the T81 ecosystem:

- TISC instruction execution
- T81VM and T81Lang runtime
- CanonFS v0.4.1 as the only storage
- Axion Θ₁–Θ₉ ethical enforcement
- T243 → T19683 cognitive tier promotion

It is designed to be:

- Fully deterministic
- Capability-native
- Immutable
- Perfectly replayable
- Axion-supervised at every transition
- Self-healing
- Rooted in content addressing and CanonFS snapshots

Hanoi is not a general-purpose OS.\
It is the *execution substrate* for T81-class machines.

______________________________________________________________________

# 2. Kernel Invariants (The Hanoi Creed)

Violation of any invariant means the system is **not Hanoi**.

1. **No mutable global state** outside capability boundaries.
1. **Every object is a CanonRef** (content hash + capability + optional sealing).
1. **CanonFS is the sole storage abstraction**; the root is always a snapshot.
1. **Axion veto authority** extends to all syscalls and state transitions.
1. **Syscalls are total functions** returning `Result<T, HanoiError>`.
1. **Deterministic scheduling** — global 81-slot tick.
1. **No userspace drivers** — all kernel drivers are CanonFS modules.
1. **Boot requires full canonical verification** (CanonHash-81 + CanonParity).
1. **Entropy is deterministic**, seeded from the snapshot; no nondeterministic RNG.
1. **Sealed objects use derived per-object keys**, no mutable key state.

______________________________________________________________________

# 3. High-Level Architecture

```

+-------------------------------------------------+
|                     Userland                    |
|            T81VM + T81Lang Runtime              |
+-------------------------------------------------+
|                 Hanoi Kernel                    |
|   • Capability Manager                           |
|   • CanonFS Driver (v0.4.1)                      |
|   • Axion Co-Processor                           |
|   • Deterministic Scheduler (81-slot)            |
|   • Linear Memory Manager                         |
|   • Syscall Layer                                 |
|   • DRBG / deterministic time                     |
+-------------------------------------------------+
|              TISC Execution Engine               |
+-------------------------------------------------+
|        Ternary Hardware / Hanoi Simulator        |
+-------------------------------------------------+

```

______________________________________________________________________

# 4. Boot Process — 7 Deterministic Stages

| Stage | Name | Action | Failure → |
|-------|------------------|--------------------------------------------------|-----------|
| 0 | ROM Stub | Load first 729-tryte CanonBlock | AXHALT |
| 1 | CanonVerify | Verify snapshot via CanonParity | AXHALT |
| 2 | Decompress | LZ81/Z3std → RAM | AXHALT |
| 3 | AxionSeal | Load Axion ethics module (Θ₁–Θ₉) | AXHALT |
| 4 | CapabilityRoot | Mint root capability from snapshot hash | AXHALT |
| 5 | Mount CanonFS | Root = `/snapshot/latest` | AXHALT |
| 6 | Start T81VM | Exec CanonExec as PID 1 | AXHALT |
| 7 | Enter Userspace | Begin deterministic global tick | — |

On any failure:

```

AXHALT(reason):
freeze state → emit canonical lineage dump → halt permanently

```

______________________________________________________________________

# 5. Core Kernel Subsystems

## 5.1 Capability Manager

- The exclusive security mechanism.
- Capabilities are:
  - Content-addressed
  - Delegatable
  - Revocable
  - Validated by Axion at every transition
- No UID/GID, no ACLs, no POSIX permissions.

## 5.2 CanonFS Driver (Ring 0)

Implements full **CanonFS v0.4.1**:

- Automatic parity repair
- Tryte-native compression (LZ81 / Z3std)
- Sparse indexing (CanonIndex)
- Extended metadata (CanonMeta)
- Deterministic write → new Merkle-81 root

## 5.3 Axion Co-Processor

- Privileged TISC lane
- Monitors syscall arguments, memory, capabilities, object graphs
- Enforces Θ₁–Θ₉
- May veto via `AxionRejection`
- May rewrite TISC hot paths

## 5.4 Deterministic Scheduler

- 81-slot round robin
- No priorities, no nondeterministic preemption
- Tick = global replay boundary

## 5.5 Memory Manager

- Linear-type memory
- Bump-pointer allocation (`no free()`)
- CanonFS-backed heap
- Capability-tagged pointers

## 5.6 Syscall Layer

Total functions only:

```

Result<T, HanoiError>

```

______________________________________________________________________

# 6. Snapshot Lifecycle (New in v0.1.1)

Snapshots now have **three distinct phases**:

### 6.1 fork_snapshot()

```

fork_snapshot() -> Result<SnapshotRef, HanoiError>

```

Creates a new Merkle-81 root **without** making it active.

### 6.2 commit_snapshot(snapshot)

```

commit_snapshot(snapshot: SnapshotRef)
-> Result<(), HanoiError>

```

Writes new snapshot into the timeline.\
Requires:

- Capability with `A` (admin) bit
- Axion approval

### 6.3 switch_root(snapshot)

```

switch_root(snapshot: SnapshotRef)
-> Result<(), AxionRejection>

```

Atomically:

1. Suspends all processes
1. Drops all mapped regions
1. Replaces root snapshot
1. Re-spawns PID 1 (T81VM)
1. Resumes deterministic tick

This guarantees a fully immutable, deterministic root-switch.

______________________________________________________________________

# 7. System Call Reference (v0.1.1)

| ID | Name | Signature | Notes |
|------|-------------------|----------------------------------------------------------------|-------|
| 0x00 | fork_snapshot | `() -> Result<SnapshotRef>` | Create new snapshot root. |
| 0x01 | commit_snapshot | `(snapshot) -> Result<()>` | Write snapshot to timeline. |
| 0x02 | switch_root | `(snapshot) -> Result<()>` | Replace active snapshot (Axion-guarded). |
| 0x03 | spawn | `(exec: CanonRef) -> Result<Pid>` | Launch T81VM instance. |
| 0x04 | read_block | `(path) -> Result<CanonBlock, CorruptionFixed>` | Auto-repair on read. |
| 0x05 | read_object | `(href) -> Result<CanonObject>` | Load + repair + decompress. |
| 0x06 | grant_cap | `(cap) -> Result<CanonRef>` | Install capability. |
| 0x07 | revoke_cap | `(href) -> Result<()>` | Publish tombstone. |
| 0x08 | yield_tick | `() -> ()` | Yield to next global tick. |
| 0x09 | map_region | `(href) -> Result<RegionHandle>` | Map object. |
| 0x0A | seal_object | `(href) -> Result<CanonRef>` | Wrap in CanonSeal. |
| 0x0B | unseal_object | `(href) -> Result<CanonRef>` | Unseal using derived key. |
| 0x0C | drbg | `() -> DeterministicRandomTrytes` | Deterministic entropy. |
| 0x0D | parity_repair | `(root) -> Result<()>` | Repair subtree. |
| 0x0E | halt | `(reason) -> !` | Permanent halt. |

### Errors

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

______________________________________________________________________

# 8. Deterministic Randomness & Time (New in v0.1.1)

Hanoi has **no nondeterministic time** and **no nondeterministic entropy**.

There is no:

- RTC
- monotonic clock
- hardware RNG
- timing jitter

### 8.1 DRBG syscall (0x0C)

```

drbg() -> 81_trytes

```

### 8.2 Deterministic seed derivation

```

seed = SHAKE-tryte(
snapshot_root
|| AxionΘ
|| process.pid
|| capability_envelope_hash
)

```

This ensures:

- deterministic output
- replayability
- sealed state cannot be forged
- no external nondeterminism leaks into Hanoi

### 8.3 Time

There is only:

```

global_tick (incremented by scheduler)

```

No wall-clock time exists inside Hanoi.

______________________________________________________________________

# 9. CanonSeal Key Derivation (New in v0.1.1)

CanonSeal AEAD keys are **not** stored in memory.\
They are **derived deterministically per object**, per snapshot.

### 9.1 KDF Formula

```

seal_key = HKDF-tryte(
input = snapshot_hash
|| object_hash
|| AxionΘ
|| cap_tag,
info  = "CanonSeal-v0.1",
len   = 81_trytes
)

```

Key properties:

- Regenerable at any time
- No persistent secret key in kernel
- Snapshot-dependent
- Capability-scoped
- Axion-integrated

This eliminates key management vulnerabilities entirely.

______________________________________________________________________

# 10. ABI Specification

## 10.1 Process Model

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

## 10.2 Memory Layout

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

## 10.3 TISC Registers

```

R0–R9: General purpose
SP: Stack Pointer
FP: Frame Pointer
PC: Program Counter
TR: Ternary Flag Register
CR: Capability Register

```

## 10.4 CanonExec Format

```

CanonExec ::= {
entry: TISC_Addr,
text: CanonRef,
data: CanonRef,
caps: CanonRef,
meta: CanonRef
}

```

Kernel verifies all hashes and asks Axion for approval.

______________________________________________________________________

# 11. Architecture Diagrams

## 11.1 Stack Diagram

```

Userland (T81VM/T81Lang)
↑
Hanoi Kernel
↑
TISC Engine
↑
Hardware / Simulator

```

## 11.2 Boot Flow

```

ROM → CanonVerify → Decompress → AxionSeal
→ CapabilityRoot → Mount FS → Spawn VM
→ Deterministic Tick

```

## 11.3 Syscall Flow

```

Process → Kernel → Axion → Kernel → Process

```

## 11.4 CanonFS Read Path

```

read(path)
→ CanonFS
→ parity repair
→ decompress
→ decrypt
→ deliver CanonBlock

```

______________________________________________________________________

# 12. Reference Implementation Scaffold — hanoi-rs

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

______________________________________________________________________

# 13. Hardware Targets

| Target | Status | Notes |
|----------------------|------------|----------------------------|
| Hanoi Simulator | Complete | Cycle-accurate |
| QEMU HanoiVM fork | Complete | x86-64 host |
| Ternary FPGA board | Q1 2026 | Ice40 |
| Photonic T81 Core | Q3 2026 | Partner R&D |

______________________________________________________________________

# 14. Roadmap (End of 2025)

- [x] Unified Hanoi Kernel Spec v0.1.1
- [ ] hanoi-rs v0.1 (boots to T81VM)
- [ ] Axion v0.1 integration
- [ ] CanonFS kernel driver v1.0
- [ ] Public ternary simulator

______________________________________________________________________

# 15. Final Statement

With v0.1.1:

- Snapshot lifecycle is explicit and Axion-governed
- Deterministic entropy and time are formalized
- CanonSeal has a real KDF
- The syscall table is complete
- The kernel remains deterministic, immutable, and ethical by construction

This is the **canonical definition of a T81-class microkernel**.

Choose any.
