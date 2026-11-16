# RFC-00D7-F01-FA01 — proposed Area F amendment

**PROPOSED / UNAPPLIED / NON-NORMATIVE / NOT IMPLEMENTED / NOT EXECUTED.**

This is standalone review material. It changes neither PR #805 nor the harness.
The current instruction authorizes preparation of this proposal only. Approval
of an experiment protocol, authorization to prepare code, qualification of a
platform and authorization to execute are separate human decisions.

## 1. Exact review identities and candidate protocol

| Item | Identity |
|---|---|
| Reviewed harness | `43b49541ddea76ba504031765229e455ef80c4ea` |
| Current protocol commit | `c8ae2200bc37064282ffc4aaa683ed21d94d567c` |
| Current protocol SHA-256 | `0c7e7da921d3af3c6c314ec4a8bf23478f9e05695b28b808721a45475106975c` |
| Contract source | `ec22a7588282d37ad027cff61c807bcf159ac2e0` |
| Actual RFC-00D3 schema SHA-256 | `a7c2cf69c9fc3b2030075cca2ea902fc5c28c53ebd28dc80b65ac4f5c3b8cbe8` |
| Proposed protocol revision | **3**, plan `F-structural-corpus-v1` |
| Candidate full protocol | `PROTOCOL-r3.proposed.md` |
| Candidate protocol SHA-256 | `d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3` |
| Literal corpus SHA-256 | `e356618677ad5282703b9c0a38e903140d287f24a8f62031572bad184abf0158` |

The new digest identifies **proposed bytes**, not an approved protocol or a new
Git head. A human may adopt those exact bytes after review; any editorial or
semantic change requires a new digest and review. `F_SECTION.proposed.md` is
the exact candidate §8, supplied for comparison. The candidate also updates
revision/authorization text and F-specific ordering/evidence declarations.
The text of A–E's experiment sections and all ceiling values are unchanged.
`source-pins.json` preserves fourteen source identities, including the old
protocol and historical F generator; `SHA256SUMS` identifies this review packet.

Author: Codex assistant, with prior RFC/harness-review context. The model
identity is not independently recorded and remains null. The same assistant
prepared earlier candidate/oracle source: separate roles or this document's
authorship are not independent scientific validation. H07 is the canonical
context; this amendment does not redefine H01–H08 or alter research/release holds.

## 2. Structural argument and its assumptions

Let S be the exact unchanged RFC-00D3 schema. Let P be its root `properties` set,
k the decoded JSON name `model_canon_hash81`, M the manifest root and R a valid,
non-null model reference. The pinned P has eleven names and excludes k. Root
`additionalProperties` is false and root `patternProperties` is absent.

```text
S(M)                       implies keys(M) is a subset of P
k not in P                 implies S(M) requires k absent from M
Stage2(M,R)                implies own root member k is present on M
therefore no M,R satisfies S(M) and Stage2(M,R), under the stated reading
```

This uses a necessary condition of Stage 2, not a model of full admission. The
schema's root key closure is sufficient; no claim about tensor semantics,
weight authenticity, state-schema encoding or policy authorization follows.
Stage 1/3 and stronger conditions cannot repair the conjunction's emptiness.

The argument is conditional on all of the following:

1. Both checks inspect the **same JSON data value and own-member set** at the
   root, derived from the same manifest artifact. Host-memory pointer reuse is
   unnecessary. Validating a child while binding a wrapper is a different claim.
2. M is ordinary immutable JSON data, without prototypes, inherited properties,
   defaults, injected fields, aliases, mutation or projection between checks.
   Duplicate names are rejected; escapes are decoded; exact names are compared
   without case-folding or coercion.
3. The selected schema is the pinned Draft 2020-12 document, with record version
   `t81.model-manifest.v1`. No wrapper schema, extension, adapter, later RFC-00D3
   revision or different version is silently selected.
4. The draft's `manifest.model_canon_hash81` denotes that root's own member.
   R contains a defined, well-formed non-null digest; absence/null/type failure
   cannot satisfy the equality through null-equals-null or host-language lookup.
5. The same interpretation is disclosed to both models and the reviewer. A
   disagreement about the draft's intended object/version yields
   `UNDERDETERMINED`, not an automatic specification repair.

The proof does not need `state_schema_hash81` to be a root property. That
location and its valid codec remain unresolved. The fixture strings `test` and
`test-state` are symbolic labels used only for structural predicate boundaries;
they are not evidence of a well-formed ModelRef or valid cryptographic binding.
The proof's R is a separate quantified symbol constrained by the assumptions.

## 3. Literal corpus, controls and coverage

`fixtures.proposed.json` contains **eight complete literal JSON fixtures**.
It is proposal data, not generated experiment results. Expected labels are
predictions and must never be supplied to the candidate or reference as answers.

| ID | Role / boundary | Predicted S(M) | Predicted own-root model equality |
|---|---|---|---|
| F00 | Existing valid-metadata mandatory control | true | false: absent |
| F01 | Existing extra-property control; explicit accept-all mutant | false | false: absent |
| F02 | Matching model member alone | false | true |
| F03 | Matching model plus opaque state member | false | true |
| F04 | Mismatching model plus opaque state member | false | false |
| F05 | Null model member | false | false |
| F06 | Wrong record-version label | false | false: absent |
| F07 | Wrapper containing an otherwise valid child | false for wrapper root | true for wrapper root |

F00/F01 retain the reviewed control manifest values and `weight_hash="test"`.
The positive control must expose an all-reject validator. The negative control
must compare an explicitly defective accept-all output to the independent schema
verdict, exposing acceptance of an extra property. That mutant's one output
replaces the ordinary candidate output in that control visit; do not call an
additional third model without counting its state. Input/expected semantics are
preserved; making the defective output explicit is part of the future code plan.

The corpus covers the contradiction's absent/present/matching/mismatching/null,
version-selection and root-identity distinctions. It is not comprehensive JSON
schema, parser, weight-mutation, tensor-shape, numeric-boundary, encoding,
provenance-nesting or interoperability coverage. Omitted dimensions remain
explicit limitations and part of the retained historical obligation.

## 4. Phases and counted-state bound

No phase has run. This table specifies the proposed actual `visit` accounting,
including inputs, repeated observations and every observable intermediate state.

| Phase | Visits | States per visit | Total |
|---|---:|---:|---:|
| F00/F01 mandatory controls, four rotations | 8 | 4 | 32 |
| One structural proof, 24 fact nodes | 1 | 78 | 78 |
| F02–F07 boundary corpus, four rotations | 24 | 6 | 144 |
| **Total** | | | **254 / 256** |

The fixed seeds remain `0,1,42,804`. Rotate `[F00,F01]` with n=2 and
`[F02,F03,F04,F05,F06,F07]` with n=6 using `(i+seed mod n) mod n`. Identical
orders from different seeds are still separate visits and charges. Controls
precede the proof; the proof runs once; boundary visits follow even after the
first structural witness. No Cartesian enumeration or cache omits a visit.

Control cost is input 1 + control working/comparison slot 1 + candidate-or-mutant
and reference outputs 2. Boundary cost retains the reviewed F six-state shape:
input 1 + outputs 2 + F working slot 1 + two aggregate result rows 2.

Proof cost retains that outer shape and adds **24*(1+1+1)**: one input descriptor,
one candidate transition and one independent reference transition per fact.
Thus `1+2+1+72+2=78`. The exact 24 facts are numbered in candidate §8. Record
intermediate observations in one proof trace; do not create extra result-row
states for each fact while claiming the same count. The two final rows report
premises/identity and conditional satisfiability. Added observable nodes,
comparison calls, repeated proof visits or per-fact rows must be charged.
There are only two states of headroom; actual instrumentation disagreement
blocks execution or stops inconclusively. There is no allowance for automatic
minimization, retries, dropped evidence or cap transfer.

Evidence reservations are eight controls at 4 KiB, 48 boundary plus two proof
ledger rows at 128 bytes, and 512 KiB aggregate raw witnesses including a proof
trace capped at 32 KiB: **563,456 bytes**, below F's unchanged 4 MiB allowance.
Existing suite metadata reservations remain. This is a source-level plan, not
a measured memory or time result. Real runner limits still require qualification.

Suite ceilings stay **49,408 states, 60 seconds wall, 50 seconds CPU, one CPU,
256 MiB aggregate memory, one active 128 MiB runner, 25 MiB artifacts**. F remains
256 states/10 seconds/4 MiB. A–E's cap and domain issues are not repaired here.

## 5. Historical obligation and honest verdicts

Preserve `F288-v1` as **DEFERRED / UNEXECUTED**, pinned to reviewed harness
`43b49541ddea76ba504031765229e455ef80c4ea`, `experiments/rfc_00d7_f01/domains.py`.
Its source identity is in `source-pins.json`. Its `9*(8+2+22)=288` inputs and
1,760-state full-path plan remain historical research obligations. They were
not tested, passed, falsified or made equivalent to this smaller corpus.
The absence of runtime results is itself preserved; nothing is silently deleted
or relabeled completed. An unexecuted obligation is not negative runtime evidence.

Only the conditional structural proof can establish the draft satisfiability
falsifier in this amended scope. Boundary comparisons validate model/reference
agreement and identity distinctions; a bug in the harness is `INVALID_RUN`, not
a newly discovered RFC defect. Mutant controls increment negative-control counts,
never admissible-falsifier counts. Four repeated witnesses are not four
independent scientific replications. Missing assumptions yield underdetermined;
missing visits/limits yield incomplete coverage, never an exhaustive pass.

## 6. Independent acceptance criteria

An independent reviewer, other than this packet's author, must:

- Reconstruct P and the Stage 2 necessary condition from the raw pinned sources,
  rather than trust this proof, generated summaries or fixture expectations.
- Check every JSON identity/version assumption against the proposed interface.
  Retain competing wrapper/version interpretations and their source rationale.
- Write or inspect a distinct proof checker without importing candidate proof,
  fixture-generation or schema-subset helpers. Disclose shared Python/JSON and
  cryptographic primitives. Independence is methodological, not a role label.
- Examine all eight literal inputs and each expected distinction. Confirm
  F00/F01 retain the existing required controls and that F07 cannot validate
  its child while claiming wrapper-root validity. Check the null and version
  cases without treating symbolic digest strings as valid artifacts.
- Inspect the independent full D3-schema oracle and the candidate's separate
  implementation. The expected table cannot be the oracle. Any disagreement
  must be resolved against source, retaining invalid/negative evidence.
- Trace all actual visit sites, including 24 proof descriptors and both sets
  of intermediate observations. Reconcile the 254 bound with emitted rows and
  evidence limits before declaring a future harness ready for authorization.

Review must identify reviewer/author relationships and exact revisions. No such
independent acceptance or proof execution is claimed by preparing this packet.

## 7. Exact change plan after approval — no changes applied now

| Surface | Required future change |
|---|---|
| PR #805 `PROTOCOL.md` | Only after explicit approval, adopt reviewed revision-3 bytes; retain revision-2/source pins and approval record. If these bytes change, recalculate the hash and re-review. |
| Protocol packet metadata | Update `proposal.json` method/coverage/uncertainty, append `PREPARATION.md`, retain history, extend `source-pins.json`, publish the approved literal corpus, and regenerate `SHA256SUMS`. Keep execution authorization unrequested. |
| Harness `domains.py` | Add a distinct literal-corpus/one-proof F plan selected by approved protocol hash. Preserve F288-v1 recipe/source history and its blocked/deferred identity; do not overload its coverage name. |
| `candidate.py` / `oracle.py` | Prepare separately authored/interpretable 24-node proof paths and necessary-root-member predicates; retain independent schema validation. No shared proof-transition or expectation lookup. |
| `evaluate.py` | Make the accept-all mutant explicit within its existing output slot; implement six-state boundary comparisons and the 78-state proof path with two final rows only. Counter disagreements are invalid harness evidence. |
| `run.py` | Replace F's current BLOCKED guard only for the explicitly approved new plan. Require all controls/rotations/proof/boundaries; do not exit F at the first structural witness. Preserve A–E behavior and caps. |
| `evidence.schema.json` | Publish a versioned v2 extension described below; preserve v1 readers/records and all existing disjoint counters. Do not edit RFC-00D3 or Commons governance schemas. |
| `static_check.py` / `CAP_ACCOUNTING.*` | Pin/inspect literal IDs, phases, proof facts, rotations and charge sites; retain the old 288-domain audit, and statically check the new arithmetic without running models. |
| `identities.json` / supervisor gate | Pin adopted protocol, amendment/corpus/source hashes, v2 evidence schema, reviewed new code SHA and interpretation registry. Reject old/mismatched plans, unknown/missing metadata and unapproved implementation selections. |
| Dependency/qualification records | Do not install/re-lock/probe under this task. Existing macOS lock still blocks Linux until separately authorized reviewed qualification work. |

The future evidence-schema v2 must require, for an amended F record:

1. `area_f_plan_id=F-structural-corpus-v1`, protocol revision/hash, amendment ID,
   amendment/corpus hashes, exact harness/oracle revisions and source pins.
2. Explicit assumption IDs, acceptance status and attributable review reference;
   `proof_status` distinguishing conditional agreement, disagreement/invalid,
   underdetermined and incomplete; 24 expected proof nodes with retained actual
   IDs/observations and the bounded raw-trace hash/path.
3. Phase, fixture ID, rotation seed and occurrence for every visit; no duplicate
   suppression. Expected mandatory/boundary/proof visits 8/24/1, observed counts,
   state breakdown 32/144/78 as plan bounds and separately actual charges.
4. Separate existing `admissible_falsifiers`, `negative_controls`, and
   `underdetermined_cases`; declared counter units and coverage completion. A
   proven witness must not erase missing visits or a later cap failure.
5. A required `legacy_obligation` naming `F288-v1`, its source/blob/hash,
   expected 288 inputs, `status=deferred_unexecuted`, and `executed_cases=0` for
   this amended run. This is not a record of a completed legacy experiment.

Use required keys, exact enums and closed objects for new fields; preserve old
schema versions as historical formats. The future authorization/qualification
schema must name plan/corpus/protocol hashes as well as the newly reviewed
harness SHA. Schema shape and machine-readable status do not authenticate a
human decision. Runtime fields above are not populated with invented results now.

Sequence: independent amendment review → attributable human protocol decision →
explicit preparation authorization → narrowly scoped code/evidence changes →
new committed harness SHA and dependency identities → independent source review
and static checks → separately authorized Linux qualification → separate one-run
authorization identifying all approved hashes. Never reuse the old harness SHA
or qualification receipt as evidence that a changed plan is approved.

## 8. Preparation boundary

Only this new proposal directory is written. No PR, remote branch, active
protocol, harness implementation, RFC, production code, frozen ISA, governance
gate or release hold is changed. No A–F validator/model/proof/control has run;
no Linux probe or dependency installation occurred. JSON parsing, source/hash
identity checks, closed-form arithmetic and Commons proposal-envelope validation
are preparation checks only. Their actual results are in `PREPARATION.md`.
