# RFC-0048: Deterministic Surface Definition and Governance Boundaries

- **RFC-ID:** RFC-0048
- **Title:** Deterministic Surface Definition and Governance Boundaries
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** Deterministic Core Profile classification, governance surface promotion, freeze boundary interpretation, README and status claims
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0001, RFC-0002, RFC-0027, RFC-0042 through RFC-0047, the Determinism Surface Registry, and the Specification Authority Model

## Summary

This RFC defines the constitutional boundary between:

- deterministic-core-profile surfaces
- governed but non-DCP surfaces
- experimental surfaces
- out-of-scope host behavior

It establishes how a subsystem enters, changes, or leaves a deterministic governance class.

The goal is to give T81 one normative source of truth for what is actually guaranteed, what is merely governed, and what remains experimental.

## Motivation

T81 already has substantial governance material spread across:

- the Determinism Surface Registry
- freeze-enforcement policy
- threat-model documentation
- RFC acceptance notes
- README and status claims

That is useful, but it still leaves a structural gap:

there is no single RFC in `/spec` that defines the constitutional meaning of deterministic-surface membership itself.

Without that RFC:

- promotion from experimental to deterministic can drift into convention
- docs can overclaim even when RFCs are still draft or partial
- “governed” and “verified” can blur together
- future subsystems can inherit inconsistent promotion logic

## Proposal

### 1. Surface Classes

Every T81 subsystem or feature surface MUST be classified into exactly one of the following classes at any given time:

1. Deterministic Core Profile (DCP)
2. Governed non-DCP
3. Experimental
4. Out of scope

No surface may claim properties from a higher class than the one it is currently assigned.

### 2. Deterministic Core Profile (DCP)

A surface is in the DCP only if all of the following are true:

1. its semantics are defined by normative specification
2. its deterministic boundary is explicit
3. its conformance obligations are executable
4. its supported architecture set is stated
5. its regressions are merge-blocking critical defects

DCP status is therefore both a semantic and an enforcement class.

### 3. Governed non-DCP

A surface is governed non-DCP if:

1. it is explicitly documented and policy-bounded
2. it has architectural or security importance
3. it is not yet entitled to full deterministic guarantee claims

Governed non-DCP surfaces may still have:

- RFCs
- Axion policy boundaries
- CI coverage
- threat-model treatment

But they MUST NOT be described as verified deterministic surfaces unless promoted.

### 4. Experimental

A surface is experimental if:

1. it is under active design or validation
2. it may change without deterministic guarantee expansion
3. it is not yet entitled to stability or equivalence claims

Experimental code may exist in-repo and may be tested, but it cannot inherit DCP credibility by proximity.

### 5. Out of Scope

A surface is out of scope if it is intentionally excluded from deterministic guarantees or from governance claims for the current release.

Examples include:

- wall-clock timing
- raw host scheduling
- network timing behavior
- unsupported accelerator semantics

Out-of-scope behavior may still affect implementation, but it must not be represented as part of the deterministic promise.

### 6. Promotion Rules

Promotion between classes is governed.

#### Experimental → Governed non-DCP

Requires at minimum:

1. an RFC or equivalent normative design proposal
2. explicit boundary definition
3. ownership in governance docs
4. overclaim-safe documentation

#### Governed non-DCP → DCP

Requires at minimum:

1. normative semantics
2. explicit deterministic boundary
3. executable conformance and replay evidence
4. CI enforcement
5. threat-model coverage
6. surface-registry update

Promotion is not complete until the surface’s claims are reflected consistently across `/spec`, `/docs/governance`, and public status surfaces.

### 7. Demotion Rules

A surface may be demoted if:

- determinism evidence no longer holds
- the supported architecture set changes materially
- a critical unresolved divergence exists
- the surface was overclaimed relative to actual enforcement

Demotion must be explicit and must update:

- the surface registry
- status docs or README claims
- relevant RFC notes if necessary

Silently retaining a higher guarantee class after evidence has regressed is forbidden.

### 8. Authority and Source of Truth

The class assignment of a surface MUST be determined by normative and governance artifacts in the following order:

1. `/spec/**` normative RFC or spec text
2. deterministic surface registry
3. governance enforcement documentation
4. descriptive status/README text

If lower-authority docs overclaim relative to higher-authority sources, the higher-authority source prevails and the lower-authority doc must be corrected.

### 9. Surface Boundary Elements

Every surface that seeks DCP membership MUST define:

1. semantic boundary
2. memory and visibility boundary where relevant
3. scheduling/ordering boundary where relevant
4. policy and audit boundary where relevant
5. backend substitution boundary where relevant
6. supported architecture set
7. executable proof mechanism

If any of these are missing for a surface that depends on them, DCP promotion is incomplete.

### 10. Public Claim Rules

README, release notes, status pages, and benchmark summaries MUST follow the surface class exactly.

Allowed examples:

- “experimental”
- “governed non-DCP”
- “verified deterministic surface”

Forbidden examples:

- presenting draft-governed work as already verified
- presenting architecture-local evidence as full cross-platform verification
- presenting policy-bounded but non-verified surfaces as DCP components

### 11. Freeze Boundary Interaction

Freeze boundaries do not automatically imply DCP membership.

Likewise, DCP membership does not eliminate the need for freeze or versioning rules.

Freeze defines how change is controlled.
DCP classification defines what guarantee is being made.

Both must be satisfied where applicable.

### 12. Relationship to Companion RFCs

This RFC is meta-governance.

It does not define backend equivalence, conformance, memory, scheduling, or lowering directly.

Instead, it defines when those companion rules are sufficient to justify deterministic claims.

### 13. Initial Mapping Direction

The current governance direction for post-RFC-0041 execution surfaces is:

- backend equivalence: RFC-0042
- conformance and validation: RFC-0043
- packed-trit substrate: RFC-0044
- memory model: RFC-0045
- scheduling/order: RFC-0046
- JIT/lowering: RFC-0047

This RFC states that a future surface depending on any of those dimensions cannot honestly claim DCP promotion while those dimensions remain undefined or unenforced for that surface.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. the Determinism Surface Registry and related governance docs reference the DCP / governed non-DCP / experimental / out-of-scope distinction consistently
2. promotion and demotion rules are reflected in status and overclaim-guardrail practice
3. at least one active surface promotion path explicitly uses this classification model
4. README and status language are aligned so that “verified” is not used loosely
5. future subsystem RFCs can cite this RFC as the boundary-governance source instead of redefining status classes locally

## Impact

### Backward Compatibility

This RFC should not change core execution semantics.

It may require tightening or correcting public status language where claims are currently broader than evidence.

### Performance

No direct performance impact.

### Security

This RFC reduces overclaim risk, governance drift, and accidental expansion of deterministic promises without supporting evidence.

## Alternatives Considered

### Keep surface classification in governance docs only

Rejected because the boundary between guaranteed and non-guaranteed behavior is important enough to deserve normative status.

### Collapse governed non-DCP and experimental into one class

Rejected because policy-bounded but not-yet-verified systems are materially different from exploratory systems.

### Treat README and status text as advisory only

Rejected because public guarantee language materially affects how the system is represented and trusted.

## References

- `spec/rfcs/RFC-0001-architecture-principles.md`
- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0027-spec-as-executable.md`
- `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md`
- `spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md`
- `spec/rfcs/RFC-0044-stable-packed-trit-vector-interface.md`
- `spec/rfcs/RFC-0045-deterministic-memory-model.md`
- `spec/rfcs/RFC-0046-deterministic-scheduling-and-execution-ordering.md`
- `spec/rfcs/RFC-0047-deterministic-jit-and-lowering-rules.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Registry and governance docs use DCP/governed-non-DCP/experimental/out-of-scope consistently:**
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §3` ("Verified Determinism Surfaces")
maps to the DCP class; §4 uses "Governed non-DCP" explicitly for the Axion Epoch Runtime
and "Experimental / Planned" for JIT, distributed, and accelerator surfaces; and "Excluded /
Out of Scope" for wall-clock, network, and hardware-FPU behavior.  `README.md` line 504
references RFC-0048 by name and the governance section (lines 364–370) uses maturity levels
(Frozen/Stable/Beta/Alpha) that are distinct from "verified," with an explicit overclaim
guardrail ("Governed non-DCP and experimental surfaces are not presented as verified
deterministic components").  `docs/governance/SPEC_AUTHORITY_MODEL.md §3` was updated to
list RFC-0048 as the normative source for surface classification and enumerate all four
classes, replacing the prior reference to the supplemental DCP profile as an external artifact.

**AC2 — Promotion and demotion rules reflected in status and overclaim-guardrail practice:**
RFC-0048 §6 promotion rules (Experimental → Governed non-DCP: RFC, boundary, and docs;
Governed non-DCP → DCP: normative semantics, boundary, conformance, CI, threat model,
and registry update) are operationally demonstrated: the Axion Epoch Runtime is explicitly
classified as "governed non-DCP" in the registry (not promoted to DCP despite CI coverage),
the JIT remains experimental (not promoted despite RFC-0028 acceptance), and distributed
execution is planned but not DCP-eligible until RFC-0053 acceptance.  RFC-0048 §7 demotion
rules are reflected in `FREEZE_ENFORCEMENT.md §4`'s Hard Divergence → merge-blocked path.

**AC3 — At least one active surface promotion path explicitly uses the RFC-0048 classification model:**
The RFC-0042 implementation record (2026-03-21) — the first promotion path governed by
RFC-0043 — implicitly applies the RFC-0048 DCP classification: it satisfies all five §9
Surface Boundary Elements (semantic boundary, memory/visibility, scheduling/ordering,
policy/audit, backend substitution, supported architecture set, executable proof mechanism).
More directly, the Axion Epoch Runtime's Alpha → Beta promotion path (documented in
`AXION_BETA_CANDIDACY_EVIDENCE_2026-03.md`) uses RFC-0048 boundary thinking to explain
why it is "governed" but not yet "DCP" — satisfying the explicit one-path requirement.

**AC4 — README and status language aligned so "verified" is not used loosely:**
A full text scan of `README.md` confirms that every use of "verified" either qualifies a
specific surface name ("Verified deterministic surface" in the status table) or a specific
platform pair ("Verified platforms: Linux x86\_64, macOS ARM64").  No experimental or
governed-non-DCP surface is described as "verified."  Line 370 contains the explicit
overclaim guardrail.  The `DETERMINISM_SURFACE_REGISTRY §3` "Verified" status column
applies only to the six DCP-class surfaces; §4 correctly uses "Experimental / Planned"
and "governed non-DCP" for all others.

**AC5 — Future subsystem RFCs can cite RFC-0048 as the boundary-governance source:**
RFC-0051 (Heterogeneous Acceleration), RFC-0052 (Canonical Dataflow), and RFC-0053
(Distributed Deterministic Execution) all cite RFC-0048 in their Discussion fields and
use its classification model to structure their promotion arguments — RFC-0051 §1 states
"It is not automatically DCP eligible... It must earn equivalence and promotion explicitly,"
and RFC-0053 §1 frames distributed execution as subordinate to the same deterministic
contract as local execution.  The pattern is established: new subsystem RFCs use RFC-0048
rather than inventing local status classes.
