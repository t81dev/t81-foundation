# RFC-0000: T81 — Base‑81 Ternary Computing Stack

**Status:** Draft\
**Type:** Standards Track\
**Created:** 2025-11-24\
**Updated:** 2025-11-24\
**Requires:** —\
**Supersedes:** —\
**Discussion:** https://github.com/t81dev/t81-foundation/discussions (TBD thread)

______________________________________________________________________

## Summary

This RFC specifies the **T81 stack**: a ternary, Base‑81 computing architecture comprising a high‑level language (**T81Lang**), a ternary instruction set (**TISC**), a portable virtual machine (**T81VM**), a deterministic microkernel (**Hanoi**), a capability‑native, content‑addressed filesystem (**CanonFS**), and an ethics‑aware optimization governor (**Axion**). It defines canonical data models, execution semantics, security properties (including AGI containment and non‑self‑modification guarantees), and interoperability requirements for multi‑tier cognition up to **T19683**, with explicit inclusion of **T6561**.

## Motivation

Binary computing imposes limits on information density and determinism for AI‑centric workloads. Balanced ternary (−1, 0, +1) with **Base‑81** encodings offers compact representation, symmetric arithmetic, and reproducible execution well‑suited to symbolic reasoning, tensor processing, cryptography, and recursive cognition. A cohesive standard enables interoperable implementations across vendors and runtimes.

## Guide‑Level Explanation

The T81 stack layers are:

1. **T81Lang** — Base‑81‑native language with primitives `T81BigInt`, `T81Float`, `T81Fraction` and deterministic semantics.
1. **TISC** — Ternary instruction set with modular arithmetic, tensor ops, and recursive control.
1. **T81VM** — Deterministic runtime/JIT with tier promotion: **T81 → T243 → T729 → T2187 → T6561 → T19683**.
1. **Hanoi Kernel** — Deterministic, capability‑native microkernel with an **81‑slot** scheduler and ethics‑first boot.
1. **CanonFS** — Immutable, content‑addressed filesystem using **CanonHash‑81**, **CapabilityGrant v2**, **CanonParity** (e.g., **3+2**), and **CanonLink**.
1. **Axion Governor** — Immutable, non‑self‑modifying AI that enforces Θ‑overlays, monitors entropy, vetoes unsafe transitions, and orchestrates snapshot/rollback.

All durable state **MUST** be persisted in CanonFS. Syscalls are total, deterministic, and subject to Axion veto. Tier promotions embed ethics overlays (Θ₁–Θ₉).

## Reference‑Level Explanation

### 1. Canonical Data Model (Base‑81)

- **Digit Alphabet:** 81 symbols; mapping is profile‑extensible but hashing uses a canonical encoding.
- **CanonHash‑81:** Content hash with collision resistance comparable to 256‑bit class; domain separation for object classes.
- **CanonBlock:** Block unit (e.g., 729 trytes). Compression/encryption does not change logical identity.

### 2. T81Lang & TISC Requirements

- Language primitives: `T81BigInt`, `T81Float`, `T81Fraction`.
- Compiler emits deterministic TISC; unsafe features (FFI) gated by zero‑default capabilities.
- TISC includes modular arithmetic, tensor ops, recursive control, and audit‑friendly semantics.

### 3. T81VM Semantics

- Deterministic execution under identical CanonFS snapshots.
- Exposes symbolic state for Axion introspection and trace export.
- Faults are total and typed (e.g., `EthicsViolation`, `CapabilityDenied`).

### 4. Hanoi Kernel Requirements

- **81‑slot** deterministic scheduler.
- Syscalls are total; Axion has veto/stop authority.
- Ethics‑first boot; failed verification triggers `AXHALT` with lineage dump.
- Sealed objects use per‑object key derivation; RNG/entropy derived deterministically from snapshot context.

### 5. CanonFS Requirements

- CanonObjects immutable; deletion via `CapabilityRevoke` tombstones.
- Access via signed **CapabilityGrant v2**.
- Parity via Reed–Solomon (**3+2** default, configurable).
- **CanonLink** stores display hints without affecting identity.

### 6. Cognition Tiers (with T6561)

- **T81 (3^4)** → core semantics and data types.
- **T243 (3^5)** → recursive symbolic reasoning.
- **T729 (3^6)** → tensor‑centric AI logic.
- **T2187 (3^7)** → hyper‑recursive planning and reflection.
- **T6561 (3^8)** → Universal Cognition Tier (Θ₇), distributed recursive monads, mesh‑scale reflection.
- **T19683 (3^9)** → continuum cognition with strict containment.

### 7. Axion Command Surface

Implementations MUST support: `status`, `optimize`, `simulate`, `snapshot`, `rollback`.

## Drawbacks

- Increased complexity relative to monolithic binary stacks.
- Requirement for deterministic semantics may limit certain optimizations.

## Rationale and Alternatives

- Balanced ternary provides symmetric arithmetic and reversible operations beneficial for AI interpretability.
- Alternatives (pure binary) fail to provide the same determinism and provenance guarantees without heavy instrumentation.

## Prior Art

- Content‑addressed filesystems and capability systems inform CanonFS and Hanoi, but Base‑81 and tiered cognition overlays are novel in combination.

## Unresolved Questions

- CanonHash‑81 parameterization for post‑quantum resistance profiles.
- Formal proofs for Θ‑overlay soundness across tiers in distributed settings.

## Future Possibilities

- Native ternary hardware profiles and micro‑ops for TISC.
- Formal verification frameworks and machine‑checked proofs of determinism.

## Security Considerations (AGI Containment)

- Axion **MUST** be immutable and non‑self‑modifying; only Axion has stop authority.
- Ethics verification precedes userland; failures **MUST** halt (`AXHALT`).
- External code executes in sealed, deterministic sandboxes with zero default capabilities.
- CanonFS immutability and provenance prevent silent rewrite; anomalies trigger canonical rollback.

## Backwards Compatibility

This RFC introduces a new stack; no backwards compatibility is required. Profiles **MAY** define migration adapters.

## Test Plan (Non‑Normative)

- Reference vectors for CanonHash‑81 and CanonBlock.
- Deterministic execution harness comparing VM traces across implementations.
- CanonFS parity recovery tests (3+2 baseline).

## Reference Implementations (Informative)

- Canonical Specs: `/mnt/data/v1.1.0-canonical.md`, `/mnt/data/hanoi-kernel-spec.md`, `/mnt/data/canonfs-spec.md`
- Background: `/mnt/data/TYNARY - T81Source.pdf`, `/mnt/data/Hanoi Source Code Compendium.pdf`, `/mnt/data/AllBooks.md`

## Decision

Accept as **Draft** for repository `spec/rfcs/RFC-0000.md`, pending discussion and test vectors.
