# spec/conformance — Spec-as-Executable Conformance Suite

**Authority:** RFC-0027 (Spec-as-Executable Conformance Model)\
**Status:** RFC-0027 conformance corpus activated in CMake/CTest (`spec_conformance_all`) with current executable set\
**Last Revised:** 2026-03-05

---

## What This Is

T81's identity claim — *conceived by AI, for AI* — requires that the specification itself be
machine-verifiable, not merely human-readable. This directory contains the **Spec-as-Executable**
conformance suite: normative invariants from the core spec documents expressed as T81Lang programs
that constitute the canonical conformance standard for T81VM.

Any conformant T81VM implementation can be verified by an AI that reads the spec and derives test
inputs from it, without depending on a hand-written test suite.

---

## Directory Structure

```text
spec/conformance/
  README.md                        ← this file
  CMakeLists.txt                   ← CMake target: spec_conformance_all
  t81-data-types/                  ← Invariants from spec/t81-data-types.md
    widening-order.t81             ← §11.8 numeric widening invariants
    canonical-encoding.t81         ← §5.1 canonicalization rules
    type-kind-completeness.t81     ← §11 all type kinds reachable (SE-M2)
  tisc/                            ← Invariants from spec/tisc-spec.md
    tier-restriction.t81           ← §5.10 AX* opcodes fault in Tier 1 (except AXVERIFY)
    opcode-determinism.t81         ← §5 each opcode produces bit-exact output (SE-M3)
    bounds-fault-contract.t81      ← §5 bounds fault fires at correct address (SE-M3)
  t81vm/                           ← Invariants from spec/t81vm-spec.md
    determinism-profile.t81        ← §1 identical input → identical output (SE-M3)
    axion-log-completeness.t81     ← §5 every privileged op emits an AxionEvent (SE-M3)
  axion-kernel/                    ← Invariants from spec/axion-kernel.md
    policy-deny-requires-reason.t81  ← §1.9 every Deny verdict has canonical reason
    segment-trace-strings.t81       ← §1.8 canonical segment=X addr=Y action=Z (SE-M4)
  cognitive-tiers/                 ← Invariants from spec/cognitive-tiers.md
    tier-annotation-enforcement.t81  ← §1 @tier(N) blocks N+1 opcodes (SE-M6)
```

---

## Annotation Conventions

Every spec program MUST use the following conventions (per RFC-0027 §2–3):

### Required Annotations

| Annotation | Meaning |
| :--- | :--- |
| `@tier(N)` | Cognitive tier constraint (1 or 2 for spec programs) |
| `@pure` | Function is side-effect-free and deterministic |
| `@axion_verify` | On `main()` — causes Axion to record a `spec_conformance pass\|fail segment=meta` event |

### Required Comment Header (per program)

```t81
// spec/conformance/<domain>/<name>.t81
// Normative ref: spec/<doc>.md §N.M
// Invariant: <one-line description>
//
// @spec-ref: spec/<doc>.md §N.M
// @invariant: <slug>
// @input-domain: <type description>
// @expected: <outcome description>
```

### Optional Annotations (proposed — see RFC-0027 §5)

| Annotation | Meaning | Status |
| :--- | :--- | :--- |
| `@expect_fault(F)` | Program expects deterministic fault `F` | Proposed — not yet in T81Lang spec |

---

## Conformance Program Rules

Each program MUST:

1. Reference the normative section it encodes in the comment header
2. Use `@axion_verify` on `main()` so Axion records the conformance event
3. Be pure and deterministic (`@tier(1)` or `@tier(2)` only)
4. Produce a single boolean assertion result (pass = `assert true`, fail = assertion error)
5. Not access CanonFS, perform I/O, or use any non-deterministic primitive

---

## How to Run

Run the executable conformance suite:

```bash
cmake --preset ci
ctest --test-dir build -R spec_conformance_ --output-on-failure
```

Or build the specific target:

```bash
cmake --build build --target spec_conformance_all
```

Each passing program emits a deterministic execution record through normal VM/Axion paths.
This provides the runnable audit record for the encoded invariant.

> **Activation status:** `spec_conformance_all` is wired to runnable `ctest` coverage via
> `spec_conformance_*` tests. RFC-0027 executable invariants are now first-class CI runnable
> artifacts for the currently executable corpus.

---

## Coverage Matrix

| Program | Normative Ref | CI Status | Milestone |
| :--- | :--- | :--- | :--- |
| `t81-data-types/widening-order.t81` | §11.8 | ✅ Pass | SE-M1 |
| `t81-data-types/canonical-encoding.t81` | §5.1 | ✅ Pass | SE-M1 |
| `t81-data-types/widening-upper-chain.t81` | §11.8 | ✅ Pass | SE-M2 |
| `t81-data-types/widening-binary-interop.t81` | §11.4, §11.8 | ✅ Pass | SE-M2 |
| `t81-data-types/canonical-ordering.t81` | §5.1.2 | ✅ Pass | SE-M2 |
| `t81-data-types/type-kind-completeness.t81` | §11 | ✅ Pass (stable representative subset) | SE-M2 |
| `tisc/tier-restriction.t81` | §5.10 + cog §1 | ✅ Pass | SE-M1 |
| `tisc/arithmetic-determinism.t81` | §5.2 | ✅ Pass | SE-M3 |
| `tisc/division-truncation.t81` | §5.2 | ✅ Pass | SE-M3 |
| `tisc/ternary-logic-canonical.t81` | §5.3 | ✅ Pass | SE-M3 |
| `tisc/comparison-total-order.t81` | §5.4 | ✅ Pass | SE-M3 |
| `tisc/fraction-normalization.t81` | §5.2 | ✅ Pass | SE-M3 |
| `tisc/conversion-determinism.t81` | §5.9 | ✅ Pass | SE-M3 |
| `tisc/bitwise-determinism.t81` | §5.14 | ✅ Pass | SE-M3 |
| `tisc/bitwise-shift-masking.t81` | §5.14 | ✅ Pass | SE-M3 |
| `tisc/bounds-fault-contract.t81` | §5.6 | ✅ Pass | SE-M3 |
| `t81vm/determinism-profile.t81` | §1, §2 | ✅ Pass | SE-M3 |
| `t81vm/axion-log-completeness.t81` | §5 | ✅ Pass | SE-M3 |
| `axion-kernel/policy-deny-requires-reason.t81` | §1.9 | ✅ Pass | SE-M1 |
| `axion-kernel/segment-trace-strings.t81` | §1.8 | ✅ Pass | SE-M4 |
| `axion-kernel/tier-supervision-invariant.t81` | §1.4 | ✅ Pass | SE-M4 |
| `axion-kernel/metadata-determinism.t81` | §1.5 | ✅ Pass | SE-M4 |
| `axion-kernel/policy-enforcement-allow-deny.t81` | §1.9 | ✅ Pass | SE-M4 |
| `cognitive-tiers/tier-annotation-enforcement.t81` | §1 | ✅ Pass | SE-M6 |

Acceptance target: executable conformance corpus wired to CI runnable target — **met**.

---

## Relationship to C++ Tests

Spec programs and C++ tests are complementary, not duplicative:

- **Spec programs** cover the *spec surface* — what the spec guarantees across any
  conformant T81VM implementation, in any host language.
- **C++ tests** (`tests/cpp/`) cover the *implementation surface* — how the reference
  VM achieves those guarantees internally.

A spec program that compiles and passes on an independent T81VM port is stronger
evidence of conformance than a C++ test of the reference implementation.
