
---
title: T81 Foundation Specification — Axion Kernel
nav:
  - [Overview](t81-overview.md)
  - [Data Types](t81-data-types.md)
  - [TISC Specification](tisc-spec.md)
  - [T81 Virtual Machine](t81vm-spec.md)
  - [T81Lang](t81lang-spec.md)
  - [Axion Kernel](axion-kernel.md)
  - [Cognitive Tiers](cognitive-tiers.md)
---

[← Back to Spec Index](index.md)

# Axion Kernel Specification  
Version 0.2 — Draft (Standards Track)

Status: Draft → Standards Track  
Applies to: T81VM, TISC, T81Lang, Cognitive Tiers

The **Axion Kernel** is the supervisory intelligence of the T81 Ecosystem.  
It enforces:

- deterministic execution invariants  
- safety and recursion bounds  
- ethics / policy constraints  
- canonicalization of complex transformations  
- tier transitions for advanced reasoning workloads  

Axion does **not** execute user code; rather, it governs **how** code may execute.

This document defines Axion’s responsibilities, invariants, hooks, visibility, and interaction model.

---

# 0. Architectural Role

Axion sits **above** all executable layers:

```

┌─────────────────────────────┐
│     Cognitive Tiers         │
├─────────────────────────────┤
│        Axion Kernel         │  ← THIS LAYER
├─────────────────────────────┤
│ T81VM  |  TISC  |  DataTypes│
├─────────────────────────────┤
│           System            │
└─────────────────────────────┘

````

Axion:

- supervises T81VM state transitions  
- vets TISC privileged instructions  
- verifies safety and determinism  
- enforces complexity and recursion limits  
- anchors cross-tier invariants  

---

# 1. Responsibilities

Axion has seven core responsibilities.

## 1.1 Determinism Stewardship

Axion MUST:

- receive a complete trace of T81VM state transitions  
- detect nondeterministic patterns  
- reject any state transition that violates deterministic semantics  
- enforce canonical memory after GC or compaction  

Axion is the final arbiter of “deterministic enough.”

## 1.2 Safety & Ethics Enforcement

Axion MUST enforce:

- safe memory operations  
- security boundaries (especially around AXSET, AXREAD, AXVERIFY)  
- resource ceilings  
- recursion depth bounds  
- purity / effect constraints at runtime  
- cognitive escalation gating  

Axion MAY terminate execution if invariants are violated.

## 1.3 Complexity Measurement

Axion measures:

- recursion depth  
- call graph complexity  
- tensor and matrix operation complexity  
- shape explosion  
- branching factor and path divergence  

These metrics guide tier transitions.

## 1.4 Tier Supervision

Axion:

- promotes/demotes computation across cognitive tiers  
- ensures that tier-appropriate invariants hold  
- blocks entry into tiers requiring explicit annotation (e.g., Tier 3 deep recursion)  

Tier rules are defined in `cognitive-tiers.md`.

## 1.5 Metadata Hosting

Axion maintains the META segment:

- fault history  
- safety markers  
- canonicalization logs  
- GC and relocation logs  
- configuration versions  
- recursion depth counters  

Axion MUST keep metadata deterministic and canonical.

## 1.6 Privileged Instruction Arbitration

Axion is the arbiter of:

- `AXREAD`
- `AXSET`
- `AXVERIFY`

All privileged operations MUST invoke Axion before evaluating their effects.

## 1.7 VM Integration

Axion is not technically part of T81VM, but all VM behavior MUST be visible and conformant to Axion's expectations.

---

# 2. Subsystems

Axion consists of **five deterministic subsystems**.

## 2.1 Deterministic Trace Subsystem (DTS)

DTS receives:

- executed instruction address  
- all register deltas  
- memory writes (structural granularity)  
- faults  
- GC movements  
- scheduling decisions  

DTS MUST maintain a **canonical record** of the program's observable behavior.

## 2.2 Verification Subsystem (VS)

Responsible for:

- verifying every privileged instruction  
- checking safety invariants  
- validating canonicalization of values  
- enforcing limits (recursion, tensor rank, VM resources)  

If verification fails:
- VS emits a deterministic **Axion Fault**, which MUST halt or redirect execution.

## 2.3 Constraint Resolution Subsystem (CRS)

CRS ensures:

- tier-appropriate constraints  
- purity constraints for non-effectful functions  
- memory safety constraints  
- shape and algebraic constraints for tensor ops  
- deterministic aliasing rules  

## 2.4 Recursion Control Subsystem (RCS)

Tracks and checks:

- recursion depth  
- recursive call signatures  
- call patterns  
- dynamic termination behavior  
- loop iteration bounds  

RCS MAY impose a hard stop if recursion becomes unbounded or unsafe.

## 2.5 Tier Transition Subsystem (TTS)

Decides when execution should:

- remain in Tier 1 (pure logic and arithmetic)  
- advance to Tier 2 (structured algorithms, branching)  
- enter Tier 3 (deep recursion or symbolic reasoning)  
- escalate to Tier 4+ (cognitive reasoning layers)

TTS requires metadata from:

- RCS  
- VS  
- CRS  

---

# 3. Recursion Controls

Axion enforces deterministic recursion limits:

### 3.1 Static Bound

If T81Lang provides an annotation like:

```t81
@bounded(100)
````

Axion MUST enforce it strictly.

### 3.2 Meta-Structural Bound

Axion evaluates recursion signatures:

* argument shrinkage
* fixed-point approach
* structural decrease

If a recursion does not appear to converge, Axion MAY halt it.

### 3.3 Tier-Dependent Bound

Higher tiers allow deeper recursion but require explicit annotation.

### 3.4 Catastrophic Recursion Detection

If execution resembles:

* unbounded branching
* shape explosion
* exponential stack growth
* runaway tensor expansion

Axion MUST halt with a deterministic Axion Fault.

---

# 4. Tier Model

Axion is the **gatekeeper** of cognitive tiers.

### Tier 1 — Pure Deterministic Computation

Simple arithmetic, no recursion, no large tensors.

### Tier 2 — Structured Algorithms

Controlled loops, branching, bounded recursion.

### Tier 3 — Recursive / Symbolic Reasoning

Complex recursion, structural transformations, limited tree search.

### Tier 4+ — Cognitive Reasoning Layers

Only allowed with explicit tier annotation and Axion approval.

Axion MUST verify the program's declared tier intent matches its behavior.

---

# 5. Privileged Instructions

Axion defines normative semantics for all `AX*` instructions.

### 5.1 AXREAD

Used to read Axion metadata.

* Allowed only for whitelisted tags
* Returns canonical values
* Unrecognized or restricted tags MUST produce a **Security Fault**

### 5.2 AXSET

Used to request Axion to alter metadata or policy.

* Allowed only for authorized tags
* Axion MUST validate requested value
* Conflicting or unsafe changes MUST produce a **Policy Fault**

### 5.3 AXVERIFY

Used to request verification of the current execution state.

* Axion MUST validate deterministic invariants
* Returns a canonical code describing success/failure
* MUST NOT fault unless verification infrastructure itself fails deterministically

---

# 6. T81VM Integration

Axion MUST interact with T81VM as follows:

### 6.1 Pre-instruction Hook

Before T81VM executes:

* privileged instructions
* unsafe memory writes
* CALL/RET that cross tiers
* tensor operations with rank > 2
* recursion-critical steps

Axion receives the machine state and may veto.

### 6.2 Post-instruction Hook

After execution:

* Axion records deltas
* updates recursion depth
* records memory write descriptors
* updates tier analysis metadata

### 6.3 Fault Propagation

On any fault:

* VM halts normal execution
* Axion receives full fault context
* Axion decides termination or rehabilitation

---

# 7. Axion Faults

Axion faults are distinct from VM/TISC faults.

## 7.1 Types of Axion Faults

* **Determinism Fault**
  Execution deviated from deterministic rules.

* **Safety Fault**
  Unsafe memory or control-flow pattern detected.

* **Policy Fault**
  Axion policy violation (security, ethics, recursion policies).

* **Tier Fault**
  Execution attempted to enter a higher tier without annotation.

* **Canonicalization Fault**
  Value failed canonical validation.

## 7.2 Behavior on Fault

Axion MUST:

1. Halt VM execution
2. Store structured fault metadata in META
3. Emit deterministic reason code
4. Optionally capture a full VM snapshot

Faulting MUST never leave VM state ambiguous.

---

# 8. Interoperability Summary

Axion MUST interoperate with:

* **TISC**: verifying privileged instructions
* **T81VM**: supervising execution and metadata
* **T81Lang**: enforcing purity and tier semantics
* **Data Types**: canonicalization of values
* **Cognitive Tiers**: providing constraints and safety boundaries

---

# Cross-References

## Overview

* **Axion Position in Architecture** → [`t81-overview.md`](t81-overview.md#2-architectural-layers)

## Data Types

* **Canonicalization Requirements** → [`t81-data-types.md`](t81-data-types.md#5-canonicalization-rules-critical-normative-section)

## TISC

* **Privileged Instructions** → [`tisc-spec.md`](tisc-spec.md#510-axion-privileged-instructions)

## T81VM

* **Trace and Metadata Requirements** → [`t81vm-spec.md`](t81vm-spec.md#51-observation)
* **Determinism and Fault Rules** → [`t81vm-spec.md`](t81vm-spec.md#2-determinism-constraints)

## T81Lang

* **Purity, Effects, and Tier Intent** → [`t81lang-spec.md`](t81lang-spec.md#3-purity-and-effects)

## Cognitive Tiers

* **Tier Definitions and Rules** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)

---
