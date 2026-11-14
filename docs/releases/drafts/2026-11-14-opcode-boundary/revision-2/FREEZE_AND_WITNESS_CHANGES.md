# Separate approvals: freeze exceptions versus witnesses

**No exception, waiver or gate transition is granted here.** This is a list for a
future bounded implementation, not an expansion of the current documentation PR.

## Freeze/API/default behavior work

| Item | Why approval is needed | Required boundary / evidence |
| --- | --- | --- |
| Strict default persisted loader and rejection of unversioned inputs | Existing public helper behavior and bytecode admission change, even with unchanged C++ signature | Scoped breaking-change RFC/spec and migration plan; ADR acceptance; major-version decision; owner review and freeze-exception controls |
| Explicit writer binding; rejection of unbound save call | Public API addition plus incompatible default behavior; compiler serialized output changes | Same scoped API exception; enumerate every caller and do not truncate an existing output on failure |
| New full-Program envelope and codec restrictions/caps | Touches serialization and compiler-emission verified/partial surfaces, narrows previously accepted data | Separately identify framing versus unchanged numeric payload encodings; registry/implementation-matrix plan and cross-platform evidence. Do not call a semantic break a harmless wrapper |
| Admission failures instead of reaching VM load | New artifact boundary prevents execution of ambiguous inputs | Approve loader/CLI error behavior explicitly; **no change to Trap enum, codes, VM fault/PC/tick/trace semantics** is requested |
| Raw13 API, opcode mapping, numeric representation, VM behavior | Preserved | No exception requested to alter them. If implementation requires such a change, stop that scope and obtain a separate decision rather than extending this exception |

Under MODULE_OWNERSHIP_CHARTER, freeze escalation requires an ADR under
`docs/architecture/adr/`, determinism registry update, `freeze-exception` PR label
and **two reviewer approvals**. APPROVAL_DELEGATION preserves those controls and
forbids authored-packet self-approval as a delegated GO/HOLD. FREEZE_ENFORCEMENT's
break procedure additionally calls for approved spec/version changes, implementation
matrix, migration notes, major-version/changelog and previous-major disposition.
Those are future authorized changes, not changes performed by this docs revision.

RFC-00D6 DM-09 must be dispositioned to the exact limited artifact boundary; its
unsigned Section 10 is not implementation approval. DM-01–08 remain outside this
proposal. Existing implementation mapping is described, not normative conformance
ratified. The release remains HOLD; RELEASE_DISCIPLINE requires no open exceptions
at release decision. Historical single-use #775 approval is not transferable.

## Golden/witness work — separately review, never silently replace

| Evidence | Treatment | Authorization/validation |
| --- | --- | --- |
| Historical raw13 bytes and API tests | Byte-identical, unchanged outputs; no envelope inserted | Existing raw fixtures/mapping tests remain; retain failures, do not weaken asserts |
| Original saved18 files and 41 tracked artifacts | Preserve bytes/hashes and provenance limitations; no auto-relabeling | New envelope fixtures go alongside originals, produced from known source under approved binding |
| Current compiler reproducibility aggregate | Preserve `ea52b9edf85a7392f2fb49814253d4f2972a79ee205a8be8246f1c40356dff71` at its original fixture/gate revision | `t81lang_repro_gate.py` hashes complete saved files. A new envelope changes input to that witness even with identical instruction payload; scope the new witness explicitly |
| New container witness | New namespace and expected hash from actual approved-implementation runs | Bind source, build/configuration, fixture list, descriptor, codec and caps; review raw per-fixture artifacts and cross-platform equality. No expected new hash is fabricated now |
| Component/payload witness | Optional additional separately named identity | Define precisely what it covers; it cannot replace the container witness or prove profile truth |
| Existing metadata/complex/alias roundtrip tests | Keep legacy evidence coverage plus new-format cases | A test-only pinned legacy codec may preserve historical reproduction; do not install it as an executable-admission bypass |
| CPA histories / Commons D-606-ENCODING | Preserve existing pins, raw files, traces and discrepancy; append this evidence | Referenced CPA implementation paths are absent from assessed main. Locate pinned sources and mark unavailable checks; do not use untracked files as verified historical proof |

Switching the production compiler to envelopes will make the old whole-file gate
fail if pointed unchanged at its new output. The gate transition must therefore
be approved **before** switching it: retain a reproducible pinned historical lane
(or its durable immutable evidence, with owner-approved gate disposition), add the
new named lane, and document which required contexts enforce each scope. Do not
turn a current required gate optional, skip it, regenerate its golden in place,
or label a failing required context a pass. This dependency can block integration;
no plan here waives it or treats a known same-version cross-platform divergence
as acceptable migration.

Docs-only revision needs no runtime freeze exception or golden regeneration.
The existing tests/goldens remain untouched. Approval of new documentation alone
is not a freeze exception, release GO or scientific-gate advancement.
