# RFC-0000: T81 — Base‑81 Ternary Computing Stack

**Status:** Accepted\
**Type:** Standards Track\
**Created:** 2025-11-24\
**Updated:** 2026-03-15\
**Requires:** —\
**Supersedes:** —\
**Discussion:** [github.com/t81dev/t81-foundation/discussions](https://github.com/t81dev/t81-foundation/discussions)

______________________________________________________________________

## Summary

This RFC specifies the **T81 stack**: a ternary, Base‑81 computing architecture comprising a high‑level language (**T81Lang**), a ternary instruction set (**TISC**), a portable virtual machine (**T81VM**), a deterministic microkernel (**Axion OS**), a capability‑native, content‑addressed filesystem (**CanonFS**), and an ethics‑aware optimization governor (**Axion**). It defines canonical data models, execution semantics, security properties (including AGI containment and non‑self‑modification guarantees), and interoperability requirements for multi‑tier cognition up to **T19683**, with explicit inclusion of **T6561**.

## Motivation

Binary computing imposes limits on information density and determinism for AI‑centric workloads. Balanced ternary (−1, 0, +1) with **Base‑81** encodings offers compact representation, symmetric arithmetic, and reproducible execution well‑suited to symbolic reasoning, tensor processing, cryptography, and recursive cognition. A cohesive standard enables interoperable implementations across vendors and runtimes.

## Guide‑Level Explanation

The T81 stack layers are:

1. **T81Lang** — Base‑81‑native language with primitives `T81BigInt`, `T81Float`, `T81Fraction` and deterministic semantics.
2. **TISC** — Ternary instruction set with modular arithmetic, tensor ops, and recursive control.
3. **T81VM** — Deterministic runtime/JIT with tier promotion: **T81 → T243 → T729 → T2187 → T6561 → T19683**.
4. **Axion OS** — Deterministic, capability‑native microkernel with a governed scheduler and ethics‑first boot.
5. **CanonFS** — Immutable, content‑addressed filesystem using **CanonHash‑81**, **CapabilityGrant**, **CanonParity**, and **CanonLink**.
6. **Axion Governor** — Immutable, non‑self‑modifying AI that enforces Θ‑overlays, monitors entropy, vetoes unsafe transitions, and orchestrates snapshot/rollback.

All durable state **MUST** be persisted in CanonFS. Syscalls are total, deterministic, and subject to Axion veto. Tier promotions embed ethics overlays (Θ₁–Θ₉). AI execution follows the deterministic contract defined in RFC-0031, ensuring bit-exact reproducibility across platforms.

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

### 4. Axion OS Requirements

- Deterministic, policy-governed scheduler (RFC-00B3).
- Syscalls are total; Axion has veto/stop authority.
- Ethics‑first boot; failed verification triggers `AXHALT` with lineage dump.
- Sealed objects use per‑object key derivation; RNG/entropy derived deterministically from snapshot context.

### 5. CanonFS Requirements

- CanonObjects immutable; deletion via `CapabilityRevoke` tombstones.
- Access via signed **CapabilityGrant**.
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

- Content‑addressed filesystems and capability systems inform CanonFS and Axion OS, but Base‑81 and tiered cognition overlays are novel in combination.

## Unresolved Questions

- **CanonHash‑81 post-quantum parameterization** — The current SHA3-512/256-truncated implementation provides pre-quantum 256-bit resistance. A formal post-quantum variant (e.g., lattice-based content addressing) is tracked as future work and does not block acceptance; the CanonHash-81 domain-separation scheme is designed to be algorithm-agnostic.

- **Θ-overlay formal proofs in distributed settings** — Machine-checked soundness proofs for Θ-overlay composition across distributed tiers remain future research. The invariants are specified normatively in RFC-0003 and enforced at runtime; formal verification is a quality-of-proof concern, not a correctness blocker.

## Future Possibilities

- Native ternary hardware profiles and micro‑ops for TISC.
- Formal verification frameworks and machine‑checked proofs of determinism.
- Post-quantum CanonHash-81 variant.

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

- Canonical specs in this repository: `spec/index.md`, `spec/supplemental/hanoi-kernel-spec.md`, `spec/supplemental/canonfs-spec.md`
- RFC catalog: `spec/rfcs/index.md`
- AI execution model: `RFC-0031` (Deterministic AI Execution Contract)
- AI-native opcodes: `RFC-0026` (AI-Native Inference Opcodes)
- Policy-gated tensor loading: `RFC-0025` (Policy-Gated Tensor Loading via CanonFS)

______________________________________________________________________

## Acceptance Criteria

| ID | Criterion | Component RFC(s) | Status |
| :--- | :--- | :--- | :--- |
| [A-0000-01] | Canonical data model: `T81BigInt`, `T81Float`, `T81Fraction` fully specified and implemented with deterministic semantics | RFC-0010, RFC-0030 | met |
| [A-0000-02] | T81Lang compiler emits deterministic TISC bytecode; `kBuiltinTable` is single source of truth; tier-gate and purity enforcement active | RFC-0007, RFC-0029 | met |
| [A-0000-03] | TISC ISA frozen at v0.4; structural opcodes, VLoad/VStore/VAdd/VFma, ChkShape, and ReadIsaVersion operational | RFC-0005 | met |
| [A-0000-04] | T81VM executes TISC deterministically; cross-layer invariants proven across 27-program conformance suite | RFC-0001, RFC-0002 | met |
| [A-0000-05] | Deterministic Trace-JIT: canonical trace hash, flat register file, CanonFS JIT cache, Axion OSR, repro oracle | RFC-0028 | met |
| [A-0000-06] | Deterministic GC: mark-and-sweep, compact_heap, GcSafepoint opcode, byte-threshold, Axion events, policy veto | RFC-0006 | met |
| [A-0000-07] | Axion safety model: AXREAD/AXSET/AXVERIFY mediation, fail-closed policy, deterministic audit log, tier-supervision invariant, instruction ceiling | RFC-0003 | met |
| [A-0000-08] | Axion Policy Language (APL v2): s-expression and YAML/JSON policy surface active in CLI, compiler, and runtime | RFC-0022 | met |
| [A-0000-09] | Axion match logging and segment-trace canonical metadata active in runtime, spec, and CLI | RFC-0019, RFC-0020 | met |
| [A-0000-10] | CanonFS: immutable objects, CapabilityGrant, CanonHash-81, policy-gated tensor loading (`TLOADHASH`) | RFC-0025 | met |
| [A-0000-11] | Canonical tensor semantics: immutable shape tuples, 1-based handle pool, TVECADD/TMATMUL with shape guards, IR lowering, Axion shape-metadata hooks | RFC-0004 | met |
| [A-0000-12] | AI-native inference opcodes: ATTN, QMATMUL, EMBED, WLOAD operational in TISC | RFC-0026 | met |
| [A-0000-13] | Deterministic AI execution contract: ternary codec, Axion hooks, T81VmBackend, EvidenceCollector, all 5 promotion phases complete | RFC-0031, RFC-0032 | met |
| [A-0000-14] | T81 Native type (`t81::T81`): AVX2+scalar addition, multiplication, subtraction, negation, conversion | RFC-0017, RFC-0018 | met |
| [A-0000-15] | Deterministic math subsystem: `t81_soft_math` integer-backed transcendentals; `FDIV` IEEE 754 exactly-rounded; no host libm dependency | RFC-0030 | met |
| [A-0000-16] | T81Lang print canonical runtime: `Opcode::Print`, Int/Bool/Float/Fraction/Symbol rendering, determinism verified | RFC-0023 | met |
| [A-0000-17] | Spec-as-executable conformance model wired into CMake/CTest | RFC-0027 | met |
| [A-0000-18] | Dual TUI frontends: `t81 studio` (7 views, palette, REPL) and `t81 agent` (14 slash cmds, session save/load) | RFC-0033 | met |
| [A-0000-19] | Axion OS kernel: TVA layout, radix page table, MMU fault model, device driver boundary, governed event-interrupt model, minimal syscall/capability boundary | RFC-00B0, RFC-00B1, RFC-00B2, RFC-00B3, RFC-00B5, RFC-00B6 | met |
| [A-0000-20] | Userland service contract and pager service ABI stable before syscall widening | RFC-00B4, RFC-00B7 | met |
| [A-0000-21] | Deterministic parallel execution (DPE): task graph primitives, epoch execution, canonical commit, DAG-ordered multi-task, level-parallel, thread pool, timeout, audit events, history ring | RFC-DPE-0001 through RFC-DPE-0009 | met |
| [A-0000-22] | Cognition tiers (T81–T6561) architecture defined and Tier4 reflection/cognition pathway specified | RFC-0021, RFC-0031 | met |
| [A-0000-23] | C++23 wording alignment: no normative changes, consistent wording, CI unaffected; 344/344 tests pass | RFC-0024 | met |

## Acceptance Note (2026-03-15)

All foundational stack layers of the T81 umbrella are implemented and accepted at the component-RFC level. The 22 fully-met acceptance criteria cover: canonical data types, T81Lang compiler, TISC ISA v0.4, T81VM interpreter, Deterministic Trace-JIT, GC, Axion safety model, APL v2, CanonFS, tensor semantics, AI-native inference opcodes, deterministic AI contract, T81 Native SIMD type, deterministic math, print runtime, conformance suite, dual TUI frontends, the complete Axion OS kernel/HAL stack, userland service contract, pager ABI, and the full DPE series.

All 23 acceptance criteria are fully met. RFC-0021 (Tier4 Cognition) was accepted 2026-03-15, closing the previously partial criterion [A-0000-22].

The two formerly unresolved questions (CanonHash-81 post-quantum parameterization and Θ-overlay formal proofs) are acknowledged as future work and are not correctness blockers.

## Decision

Accept as **Accepted**. All component RFCs (RFC-0001 through RFC-0033, RFC-00B0 through RFC-00B7, RFC-DPE-0001 through RFC-DPE-0009) are accepted; all 23 acceptance criteria are met. The T81 stack is fully specified and implemented to the extent required for stable release.
