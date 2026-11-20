# Determinism Threat Model

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Determinism Threat Model](#determinism-threat-model)
  - [1. Purpose](#1-purpose)
  - [2. Threat Model Scope](#2-threat-model-scope)
    - [Protected Assets](#protected-assets)
    - [Adversary Classes](#adversary-classes)
    - [Out of Scope](#out-of-scope)
  - [3. Threat Categories](#3-threat-categories)
    - [A. Compiler-Level Threats](#a-compiler-level-threats)
    - [B. VM-Level Threats](#b-vm-level-threats)
    - [C. Backend Equivalence Threats](#c-backend-equivalence-threats)
    - [D. Spec Drift Threats](#d-spec-drift-threats)
    - [E. Scheduling and Concurrency Threats](#e-scheduling-and-concurrency-threats)
    - [F. CI / Governance Bypass Threats](#f-ci--governance-bypass-threats)
    - [G. Artifact Integrity Threats](#g-artifact-integrity-threats)
    - [H. Governed AGI Control Threats](#h-governed-agi-control-threats)
  - [4. Known Residual Risks](#4-known-residual-risks)
  - [5. Determinism Breach Classification](#5-determinism-breach-classification)
  - [6. Review and Audit Process](#6-review-and-audit-process)

<!-- T81-TOC:END -->


## 1. Purpose

This document defines the threat landscape for T81's determinism guarantees. It identifies potential failure modes, adversarial vectors, and the mitigation strategies employed to ensure bit-exact reproducibility.

**References:**

* `DETERMINISM_SURFACE_REGISTRY.md` (Defines verified surfaces)
* `FREEZE_ENFORCEMENT.md` (Governs immutable components)
* `SPEC_AUTHORITY_MODEL.md` (Establishes source of truth)
* `INCIDENT_RESPONSE.md` (Defines incident handling protocol)
* `../../spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md` (backend substitution constraints)
* `../../spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md` (proof and breach model)
* `../../spec/rfcs/RFC-0044-stable-packed-trit-vector-interface.md` (shared packed-trit substrate)
* `../../spec/rfcs/RFC-0045-deterministic-memory-model.md` (memory visibility and aliasing)
* `../../spec/rfcs/RFC-0046-deterministic-scheduling-and-execution-ordering.md` (ordering and concurrency constraints)
* `../../spec/rfcs/RFC-0047-deterministic-jit-and-lowering-rules.md` (lowering constraints)
* `../../spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md` (boundary classification)
* `../../spec/rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md` (arithmetic oracle)
* `../../spec/rfcs/RFC-0050-vectorized-ternary-operations-for-tisc.md` (vector semantics)
* `../../spec/rfcs/RFC-0051-deterministic-heterogeneous-acceleration.md` (accelerator threat boundary)
* `../../spec/rfcs/RFC-0052-canonical-dataflow-and-state-driven-execution.md` (dataflow/state model)
* `../../spec/rfcs/RFC-0053-distributed-deterministic-execution-protocol.md` (distributed threat boundary)

**Clarification:**
Determinism guarantees apply exclusively to surfaces explicitly listed as **Verified** in the [Determinism Surface Registry](DETERMINISM_SURFACE_REGISTRY.md).

---

## 2. Threat Model Scope

### Protected Assets
This model protects the integrity and reproducibility of:
* **Bytecode Emission:** T81Lang compilation output.
* **Execution Traces:** Instruction-level execution logs.
* **Hash Outputs:** Cryptographic signatures of state and data.

### Adversary Classes
The following threat actors and scenarios are within scope:
* **Accidental Developer Error:** Unintentional introduction of nondeterministic code (e.g., hash map iteration).
* **Compiler/Toolchain Drift:** Variations in binary generation across compiler versions or flags.
* **Cross-Platform Inconsistencies:** Divergence between x86-64 and ARM64 execution.
* **Malicious Contributor:** Intentional injection of subtle nondeterminism or backdoors.
* **Supply Chain Tampering:** Compromise of build dependencies or artifacts.

### Out of Scope
The following are excluded from this specific threat model:
* **Physical Hardware Attacks:** Rowhammer, voltage glitching, or side-channel power analysis.
* **Kernel-Level OS Compromise:** Malicious operating system behavior.
* **Network Timing Nondeterminism:** Variations in packet arrival or latency (unless explicitly modeled).

---

## 3. Threat Categories

### A. Compiler-Level Threats

**Risks:**
* **Undefined Behavior (UB):** Reliance on C++ behaviors not guaranteed by the standard.
* **Compiler Optimization Drift:** Different optimization levels (`-O2` vs `-O3`) altering logic.
* **Standard Library Variance:** Differences in `libc` or `libstdc++` implementations (e.g., `std::sin`).
* **Fast-Math Flags:** Aggressive floating-point optimizations breaking IEEE 754 compliance.
* **ABI Drift:** Binary incompatibility between linked components.

**Mitigations:**
* **Soft-Float Usage:** Mandated use of `T81Float` for all deterministic arithmetic.
* **Repro Gate:** CI jobs enforcing bit-exact binary reproduction.
* **Cross-Arch CI:** Mandatory testing on both x86-64 and ARM64 runners.
* **Backend Equivalence Governance:** Optimized execution paths must remain scalar-oracle equivalent rather than introducing backend-specific semantics.

---

### B. VM-Level Threats

**Risks:**
* **Memory Allocation Order:** Pointer addresses leaking into logic or output.
* **Hash-Map Iteration:** Unordered containers yielding different traversal sequences.
* **Uninitialized Memory:** Reading garbage values from stack or heap.
* **Time-Dependent Behavior:** Logic branching based on `std::time` or performance counters.
* **Thread Scheduling:** Race conditions or nondeterministic interleaving.

**Mitigations:**
* **Deterministic Data Structures:** Use of ordered maps/sets or stable sorting before iteration.
* **No Wall-Clock Dependency:** Execution logic is decoupled from real time.
* **Controlled Execution Model:** Single-threaded or strictly synchronized execution for verified surfaces.
* **Canonical Memory Semantics:** Handle identity, visibility timing, and aliasing are governed at the deterministic boundary rather than inherited from host allocation behavior.

---

### C. Backend Equivalence Threats

**Risks:**
* **Scalar/SWAR/SIMD Drift:** Different execution backends produce different bytes or traps.
* **Architecture-Specific Semantics:** AVX2, NEON, or future lowering paths diverge while still passing local smoke tests.
* **Dispatch Ambiguity:** Runtime backend selection depends on timing, heuristics, or non-governed host state.
* **JIT Lowering Drift:** Lowered traces become semantically different from interpreter execution.

**Mitigations:**
* **Scalar Oracle Rule:** Scalar semantics remain the constitutional oracle for promoted backend families.
* **Backend Equivalence Suites:** Differential tests must compare scalar, SWAR, SIMD, and future backends explicitly.
* **Deterministic Dispatch:** Backend choice must depend only on governed predicates such as size, architecture support, and approved capability flags.
* **Cross-Platform Replay:** Supported-architecture replay artifacts must match exactly before deterministic claims expand.

---

### D. Spec Drift Threats

**Risks:**
* **Spec Updated Without Impl:** Implementation lags behind spec, creating a phantom standard.
* **Impl Updated Without Spec:** Code behavior changes without specification update (de facto standard).
* **Silent Semantic Change:** Subtle alteration of opcode behavior without version bump.

**Mitigations:**
* **Freeze Enforcement:** CI checks preventing modification of frozen directories.
* **Authority Model:** Strict hierarchy placing `/spec` above implementation.
* **Implementation Matrix:** Governance tracking of spec compliance.

---

### E. Scheduling and Concurrency Threats

**Risks:**
* **Nondeterministic Interleaving:** Parallel work changes observable meaning depending on host thread timing.
* **Conflict Resolution by Accident:** Overlapping writes are resolved by incidental worker order rather than governed order.
* **Abort/Timeout Leakage:** Partial state becomes observable after an aborted scheduling unit.
* **Worker Identity Leakage:** Host thread IDs or worker assignment affect deterministic outputs or traces.

**Mitigations:**
* **Canonical Commit Order:** Parallel work becomes observable only through governed ordering rules.
* **Snapshot + Deferred Visibility:** Epoch execution reads immutable input state and commits atomically.
* **Ordering Constitution:** Program order, dependency order, and canonical commit order are treated as explicit governance concepts.
* **No Worker-Semantic Coupling:** Host worker identity remains outside deterministic meaning.

---

### F. CI / Governance Bypass Threats

**Risks:**
* **Determinism Gate Disabled:** malicious or accidental removal of CI checks.
* **Artifact Upload Manipulation:** Uploading forged artifacts that match expected hashes.
* **Selective Platform Testing:** Skipping one architecture to hide divergence.

**Mitigations:**
* **Required Status Checks:** Branch protection rules requiring all gates to pass.
* **Cross-Platform Enforcement:** Workflow topology mandates multi-arch success.
* **Public Transparency:** Build logs and artifact hashes are public.

---

### G. Artifact Integrity Threats

**Risks:**
* **CanonFS Hash Manipulation:** falsifying file system metadata.
* **Build Artifact Substitution:** replacing a verified binary with a compromised one.
* **Non-Reproducible Builds:** Builds that cannot be independently verified.

**Mitigations:**
* **Hash Verification:** Cryptographic checking of all inputs and outputs.
* **Reproducibility Gates:** Automated rebuilding of artifacts to verify identity.
* **Controlled Build Process:** Hermetic build environments.

---

### H. Governed AGI Control Threats

**Risks:**
* **Policy Bypass Paths:** AGI-facing logic executes without expected Axion policy checks.
* **Nondeterministic Decision Leakage:** AGI behavior depends on unverified nondeterministic surfaces while presented as deterministic.
* **Unsafe Autonomy Escalation:** Experimental cognitive-tier behavior is exposed without explicit governance boundary classification.
* **Audit Blind Spots:** Missing trace metadata prevents post-incident reconstruction of AGI-related actions.

**Mitigations:**
* **Boundary Classification:** Every AGI-facing release change is classified as DCP, governed non-DCP, or experimental.
* **Governed Promotion Pipeline:** AGI-oriented surfaces must pass ADR, threat-model, registry, and incident-readiness gates before guarantee expansion.
* **Traceability Controls:** Policy/segment/guard metadata tests must remain green for language-to-Axion paths.
* **Incident Trigger Signals:** Severity escalation when policy bypass or deterministic-boundary misrepresentation is detected.

---

## 4. Known Residual Risks

The following risks are acknowledged but not fully mitigated:

* **Partial Compiler Reproducibility:** Full binary bit-exactness across different compiler *vendors* (Clang vs GCC) is an ongoing challenge.
* **Experimental JIT:** The Trace-JIT system is currently experimental and may exhibit subtle divergences until the backend-equivalence and conformance RFC chain is integrated.
* **Distributed Tiers:** Consensus mechanisms for distributed execution are not yet verified surfaces.
* **External Model Formats:** Importing third-party models (SafeTensors/GGUF) relies on external parsers which may have edge cases.
* **Draft Memory/Scheduling Governance:** RFC-0045 and RFC-0046 define direction, but not all current implementation assumptions are yet audited against them.
* **Draft Arithmetic / Vector / Accelerator Governance:** RFC-0049, RFC-0050, and RFC-0051 define direction, but current implementation surfaces are not yet fully audited against them.
* **Draft Dataflow / Distributed Governance:** RFC-0052 and RFC-0053 define future execution constitutions, but current reactive/distributed surfaces remain non-verified.

---

## 5. Determinism Breach Classification

Breaches of determinism are classified by severity and required response.

| Level | Severity | Impact | Required Action |
| :--- | :--- | :--- | :--- |
| **0** | **Negligible** | No impact on verified surfaces. | Standard bug fix process. |
| **1** | **Minor** | Partial regression in fixture generation or tooling. | Fix bug, add regression test, patch release. |
| **2** | **Critical** | **Verified surface regression.** Output differs across architectures. | **Immediate Revert.** Root cause analysis. Full audit. |
| **3** | **Catastrophic** | Spec-level determinism violation (e.g., ISA definition flaw). | **Major Version Invalidation.** Community notification. |

Draft governance note:

Once RFC-0043 is accepted, breach classification should be aligned with its hard-divergence / soft-divergence / UB-exposure terminology in addition to the severity table above.

---

## 6. Review and Audit Process

* **Release Review:** This threat model must be reviewed and approved before any Major or Minor release.
* **New Surfaces:** Any addition to `DETERMINISM_SURFACE_REGISTRY.md` triggers a requirement to update this threat analysis.
* **Breaking Changes:** Any breaking change proposal (RFC) must include an impact analysis referencing this document.
* **Incident Linkage:** Severity 2/3 determinism incidents must be handled under `docs/governance/INCIDENT_RESPONSE.md`.
* **Governed AGI Linkage:** AGI-facing surface promotions must remain aligned
  with the current boundary model and include explicit risk treatment updates in
  this document.
