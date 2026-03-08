# RFC-0027: Spec-as-Executable Conformance Model

**Status:** accepted
**Type:** standards-track
**Applies-To:** `spec/` (all normative docs), `spec/rfcs/`, T81Lang compiler, Axion policy engine
**Created:** 2026-03-01
**Updated:** 2026-03-08
**Supersedes:** —
**Superseded-By:** —
**Discussion:** —

---

## Summary

Status note: accepted because the executable conformance corpus is now wired
into repository build/test workflows. Proposed annotation extensions remain
follow-on work and do not block the core model.

T81's identity claim — *conceived by AI, for AI* — requires that the
specification itself be machine-verifiable, not merely human-readable. This
RFC defines the **Spec-as-Executable** model: normative invariants from
the core spec docs are expressed as T81Lang programs that constitute a
living, runnable conformance suite. Any conformant T81VM implementation
can be verified by an AI that reads the spec and derives test inputs from
it, without depending on a hand-written test suite.

---

## Motivation

The current state:

- Normative invariants live in prose (`spec/*.md`).
- Conformance is verified by a hand-written C++ test suite (`tests/cpp/`).
- The test suite and the spec are maintained independently; they can drift.
- An AI reading the spec cannot independently verify a T81VM implementation
  without also reading the test code.

The gap: if T81 is designed for AI, the spec should be something an AI can
*execute*, not just read. A spec that is also a runnable program is:

1. **Self-verifying** — the spec tests itself on every CI run
2. **AI-derivable** — an AI can generate conformant test inputs from the spec
   program, without human-written test fixtures
3. **Drift-resistant** — a normative invariant expressed as T81Lang cannot
   silently diverge from the test suite; they are the same artifact
4. **Portable** — any future T81VM implementation (in any host language)
   can be verified by running the same spec programs

---

## Proposal

### 1. Spec Invariant Programs

Each normative spec document gains a companion directory
`spec/conformance/<doc-name>/` containing T81Lang programs that encode
its normative invariants. These programs are the **canonical conformance
suite** for that spec section.

```
spec/
  conformance/
    t81-data-types/
      canonical-encoding.t81        # §5 canonicalization rules
      widening-order.t81            # §11.8 numeric widening invariants
      type-kind-completeness.t81    # §11 all 34 Type::Kind values reachable
    tisc/
      opcode-determinism.t81        # each opcode produces bit-exact output
      bounds-fault-contract.t81     # bounds fault fires at correct address
      tier-restriction.t81          # AX* opcodes fault in Tier 1
    t81vm/
      determinism-profile.t81       # Tier A: identical input → identical output
      axion-log-completeness.t81    # every privileged op emits an AxionEvent
    axion-kernel/
      policy-deny-requires-reason.t81  # every Deny verdict has canonical reason
      segment-trace-strings.t81    # canonical segment=X addr=Y action=Z form
    cognitive-tiers/
      tier-annotation-enforcement.t81  # @tier(N) blocks N+1 opcodes
```

### 2. T81Lang Invariant Expression Pattern

Spec programs use a standard assertion pattern:

```t81
// spec/conformance/t81-data-types/widening-order.t81
// Normative ref: spec/t81-data-types.md §11.8
// Invariant: T81Qutrit widens to i2 widens to i8 ... widens to T81Float

@tier(1) @pure
fn check_widening_order() -> bool {
    let q: T81Qutrit = 1t;
    let i: i8 = q as i8;       // must not fault
    let b: T81BigInt = i as T81BigInt;
    let f: T81Float = b as T81Float;
    f == 1.0f
}

@axion_verify
fn main() -> void {
    assert check_widening_order();
}
```

Each program MUST:
- Reference the normative section it encodes in a comment header
- Use `@axion_verify` on `main` so Axion records the conformance event
- Be pure and deterministic (Tier 1 or Tier 2 only)
- Produce a single boolean result (pass/fail)

### 3. AI-Derivable Test Vectors

Spec programs carry structured metadata that an AI can use to derive
additional test inputs without reading the source program:

```t81
// @spec-ref: spec/t81-data-types.md §11.8
// @invariant: widening-monotone
// @input-domain: T81Qutrit({-1,0,1})
// @expected: no TypeFault, result bit-exact
```

An AI given this metadata can enumerate the full input domain and verify
the invariant holds across all inputs — without a hand-written fixture.

### 4. RFC-0026 Integration

AI-Native Inference Opcodes (RFC-0026) MUST ship with spec programs:

```
spec/conformance/tisc/
  attn-determinism.t81       # ATTN produces bit-exact output across platforms
  qmatmul-scale-order.t81    # dequantize-then-multiply ordering enforced
  wload-policy-gate.t81      # WLOAD with invalid provenance → SecurityFault
```

No new opcode class may be accepted without corresponding spec programs.
This is a **hard acceptance criterion** for all future ISA RFCs.

### 5. CI Integration

```yaml
# ci.yml addition
spec-conformance:
  runs-on: [ubuntu-latest, macos-latest]
  steps:
    - run: cmake --preset ci
    - run: ctest --test-dir build -R spec_conformance_ --output-on-failure
```

The CMake target `spec_conformance_all` compiles and runs every
`spec/conformance/**/*.t81` program. Failure blocks merge.

### 6. Spec Document Annotation

Each normative section that has a companion spec program gains a
reference annotation:

```markdown
## §11.8 Numeric Widening Order

...

> **Conformance program:** [`spec/conformance/t81-data-types/widening-order.t81`](../conformance/t81-data-types/widening-order.t81)
```

---

## Determinism / Safety Considerations

Spec programs are Tier 1 or Tier 2 only and are `@pure`. They MUST NOT
access CanonFS, perform I/O, or use any non-deterministic primitive.
Axion MUST record a `spec_conformance pass|fail segment=meta` event for
each `@axion_verify main()` execution so the conformance run is auditable.

---

## Compatibility

This RFC is additive. Existing tests in `tests/cpp/` are not removed; they
complement the spec programs by covering implementation internals. The spec
programs cover the *spec surface* (what the spec guarantees); the C++ tests
cover the *implementation surface* (how the VM achieves it).

---

## Implementation Plan

| Milestone | Scope | Target |
| :--- | :--- | :--- |
| SE-M1 | `spec/conformance/` directory structure; CMake `spec_conformance_all` target | 2026-04-01 |
| SE-M2 | Spec programs for `t81-data-types.md` §5 and §11.8 (6 programs) | 2026-04-15 |
| SE-M3 | Spec programs for `tisc-spec.md` §5 opcode determinism (10 programs) | 2026-04-30 |
| SE-M4 | Spec programs for `axion-kernel.md` §1 policy enforcement (5 programs) | 2026-05-15 |
| SE-M5 | AI-derivable metadata annotations on all spec programs | 2026-05-30 |
| SE-M6 | CI gate: `spec-conformance` job blocks merge on failure | 2026-06-01 |
| SE-M7 | RFC-0026 conformance programs (ATTN, QMATMUL, WLOAD) | per RFC-0026 AI-M milestones |

---

## Open Questions

1. **T81Lang compiler maturity:** SE-M2 requires the T81Lang compiler to
   handle `@axion_verify` and `@pure` annotations correctly. If the compiler
   is not ready, spec programs may need to be expressed as C++ host programs
   that drive the VM directly. The C++ fallback format should be defined.

2. **Negative conformance programs:** Should spec programs encode *expected
   fault* conditions (e.g., "this input MUST raise BoundsFault")? A
   `@expect_fault(BoundsFault)` annotation is proposed but not yet in the
   T81Lang spec.

3. **Spec program versioning:** When a normative section changes (e.g., a
   freeze exception), the companion spec program MUST change in the same
   commit. Enforcement mechanism (CI lint?) is TBD.

---

## Acceptance Criteria

- `spec/conformance/` contains at least 21 passing spec programs covering
  `t81-data-types.md`, `tisc-spec.md`, and `axion-kernel.md`
- CMake target `spec_conformance_all` compiles and passes on Linux x86-64
  and macOS ARM64
- CI `spec-conformance` job is green and blocks merge
- Each passing spec program emits a `spec_conformance pass segment=meta`
  AxionEvent visible in the CI trace log
- All normative sections with companion programs carry the conformance
  program cross-reference annotation
- RFC-0026 acceptance criteria include passing spec programs for ATTN,
  QMATMUL, and WLOAD
