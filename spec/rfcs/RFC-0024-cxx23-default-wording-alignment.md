______________________________________________________________________

# RFC-0024 — C++23 Default Wording Alignment for Spec and Governance Docs

Version 0.2 — Standards Track\
Status: Accepted\
Updated: 2026-03-15\
Author: T81 Foundation\
Applies to: spec text, governance documentation, tooling language

vote: +1

______________________________________________________________________

# 0. Summary

This RFC proposes a documentation-only, governance-safe alignment:

1. Update spec/governance wording that still states or implies a C++20-default
   frontend/toolchain.
2. Standardize language to:
   - C++23 is the default build/toolchain mode.
   - A temporary C++20 compatibility lane remains supported via
     `-DT81_USE_CXX23=OFF`.
3. Preserve semantics and normative behavior unchanged.

______________________________________________________________________

# 1. Motivation

Repository implementation and CI now treat C++23 as the default language mode,
while several normative/spec-adjacent texts still mention a C++20 frontend as
current reality. This creates avoidable ambiguity for contributors and tooling
authors.

The mismatch is contextual rather than semantic, but context drift in
governance documents can still cause incorrect operational assumptions.

______________________________________________________________________

# 2. Design / Specification

### 2.1 Scope

In-scope updates are wording-level only:

- Replace stale “C++20 frontend/toolchain” statements where they describe
  present-day defaults.
- Add explicit note that C++20 remains available as a compatibility lane.

Out-of-scope:

- No change to language semantics.
- No change to opcode, VM, Axion, CanonFS, or determinism invariants.
- No change to wire formats (`.tisc`, traces, contracts).

### 2.2 Candidate Files

Primary candidate references include:

- `spec/t81lang-spec.md` (contextual compiler wording)
- RFC historical text that describes implementation baseline rather than
  immutable history (case-by-case review)

### 2.3 Acceptance Criteria

1. No normative behavior changes (`MUST`/`SHOULD` semantics unchanged).
2. Wording is internally consistent across spec/governance docs.
3. CI/build rituals remain unchanged and passing.

______________________________________________________________________

# 3. Rationale

This proposal keeps constitutional stability while eliminating contextual
confusion. It also aligns with the current AGENTS contract requiring C++23 as
the primary path.

______________________________________________________________________

# 4. Backwards Compatibility

Fully backwards compatible.

- Existing C++23 users are unaffected.
- Existing C++20 compatibility workflow remains explicitly documented.

______________________________________________________________________

# 5. Security Considerations

No security-surface changes. This is wording alignment only.

______________________________________________________________________

# 6. Open Questions

1. Should historical RFCs retain original “C++20” language when describing
   past milestones, or should they append an editor note for current defaults?
   **Resolved:** Historical RFCs describing past milestones retain their original
   wording. Only present-tense normative/prescriptive statements are updated.
2. Should the compatibility lane sunset date be captured in a separate RFC?
   **Resolved:** No separate RFC needed; the lane remains until explicitly retired.

______________________________________________________________________

# 7. Acceptance Note

**Status advanced `Draft → Accepted` on 2026-03-15.**

All three acceptance criteria from §2.3 are satisfied:

| Criterion | Evidence |
| :--- | :--- |
| [A-0024-01] No normative behavior changes | Wording-only update; no `MUST`/`SHOULD` semantics altered |
| [A-0024-02] Internally consistent wording | `spec/t81lang-spec.md:232` updated (`C++20` → `C++23`); all remaining C++20 references in historical RFCs describe past milestones and are correctly scoped |
| [A-0024-03] CI/build rituals unchanged | No build or test changes required; 344/344 tests continue passing |

Open questions resolved inline above. C++23 is the confirmed default; C++20
compatibility lane (`-DT81_USE_CXX23=OFF`) remains supported.
