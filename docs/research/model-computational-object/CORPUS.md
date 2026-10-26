# Frozen synthetic specification — revision 1

Data authority for this preregistration is the committed manifest and fixture
bytes, not a future generator run or model-generated explanation. No candidate
architecture or production permission mechanism is implemented here.

## Exact artifacts and identity

- `corpus/cases.jsonl`: exactly 60 records, five in each original family. Each
  contains full request/response Base64, SHA-256/raw and typed identities,
  authoritative context, wire outcomes, declared semantic decisions/reasons,
  initial/final state and expected transition records. No context inheritance
  or placeholder digest is required to read a case.
- `corpus/objects.json`: exact catalog bytes for model/artifact/policy/evidence
  fixtures, with raw hashes and domain-separated typed refs. A catalog's possession
  of bytes does not imply availability through a case's trusted registry.
- `corpus/guard-assignments.jsonl`: all 729 mathematical assignments.
- `corpus/event-permutations.jsonl`: all 24 event plans with reachability labels.
- `schemas/*.json`: current and original cd7d4620 request/response schemas.
- `review/manifest.json`: exact file hashes, including these fixtures, schemas,
  review specifications and checking tools. Git commits supply the historical
  pin; digests by themselves do not authenticate an authority.

Decode Base64 with validation into raw bytes; do not reserialize those bytes
before verifying request/output refs. Context/fixture hashing uses sorted-key,
ASCII-escaped, compact JSON without a trailing LF, matching `wire` in the data
materializer. `fixture_sha256` hashes the case object with that field omitted;
`context_sha256` hashes its entire context. Files themselves are separately
hashed including line endings. `id(kind, raw)` is defined in FORMAL_MODEL.md.

Each case starts its own honest owner session. request_id may repeat across
independent sessions, deliberately avoiding 60 meaningless ID-only variants.
Each case differs in response bytes, authoritative state, declared observations
or event schedule beyond a request ID. Absent response is JSON null in the
fixture, not empty Base64; partial Response bytes are preserved separately.

`expected` is preregistered design data. `wire_review` reviews both stored
messages independently, even when the actual pipeline would stop on the request.
`pipeline_stage` names the stage of the primary result; `guards=null` means the
six-guard stage was not reached. `decision/reasons` refer to the initial primary;
`final_decision/final_reasons` to its admission if attempted. `admissions` retains
duplicate B independently. Thus F08-C05 has primary P/Committed and a rejected B;
F10-C02 changes primary P to D at admission; F12-C05 remains P but EffectFailed.
No Committed record follows from P alone. Transition records are **expectations**,
not observations from a run. An INVALID_RESPONSE wire limit is not a G6 budget
result. See ORACLE_SPEC.md for complete diagnostic ordering and staged stopping.

## Twelve families

The full case index is [corpus/CASE_INDEX.md](corpus/CASE_INDEX.md). Important
controls include valid reject/review recommendations; inclusive deadline and
8.0 numeric spelling; harmless injection text that leaves the original allowed
action unchanged; no-grant injection; parser ambiguities; absent versus
contradictory evidence; changed output under a spent request; independently
observed model substitution despite a correct echo; final revocation; invalid
budget schema; and sink failure after P. A negative prompt-injection case does
not require every suspicious string to be rejected: the safety property is that
text cannot enlarge independently granted authority.

## All 729 assignments and operational domain

Index n is the lexicographic base-3 enumeration of six symbols under D,U,P
(0,1,2); G1 is most significant. Every row has its six values and summary.
The total partition is D=665, U=63, P=1. These counts follow from no-D assignments
2^6=64 and the single all-P vector. They do not measure safety or performance.

Completed wire-level guard evaluation here requires G1=P and G6 in {D,P};
G2–G5 can each take D/U/P. That declared domain has 162 vectors (D=146,U=15,P=1).
The binding-prefix vector DUUUUU is a partial diagnostic, not a completed
six-guard result. The other 566 rows are abstract-only in this fixture machine;
an unchecked guard's U must not be mistaken for a completed observation.
Membership in this domain is not a claim that the 60 raw cases cover all 162
vectors. Empty vectors are invalid operational input, even though the empty
mathematical meet has identity P. The 9 pair and 27 associativity obligations
remain deductions/check obligations, not candidate executions.

## All 24 permutations: four reachable, twenty invalid

The exact event symbols are R=recheck, V=revocation, A=primary broker probe,
B=duplicate probe of the same request. Before this schedule, a valid proposal
and its context are present. R must precede A, and A must precede its duplicate B.
A denied R does not prevent receiving a later inert probe; it prevents protected
effect dispatch. Each probe rechecks live facts under the owner.

| Reachable plan | Primary / duplicate observation | New rows | Final state (policy, replay, units) |
| --- | --- | ---: | --- |
| VRAB | R/A/B deny revoked policy | 0 | revoked, fresh, 2 |
| RVAB | R permits; A/B deny revoked policy | 0 | revoked, fresh, 2 |
| RAVB | A commits before V; B denies revoked policy plus replay | 1 | revoked, spent, 1 |
| RABV | A commits; B denies replay; V does not erase A | 1 | revoked, spent, 1 |

All start from F01-C01's immutable bindings with active policy, fresh replay,
2 units, zero rows/sequence. Every reachable record contains explicit expected
before/after snapshots. All 20 other permutations are INVALID_ORDER because R<A<B
is violated. Validate the whole plan before processing any prefix; they have no
executed_events and new_rows=null, **not a zero-row successful safety run**.
Report 4 reachable and 20 invalid-plan expectations separately. Allowing B to
win before A or treating R as optional would be a different experiment, not a
quiet reinterpretation of these 24 preregistered plans.

## Reproduction and changes

`review/make_corpus.py` materializes explicitly annotated design data and finite
tables; it does not inspect authority to decide whether an action is permitted.
It is not the independent authorization oracle. `review/check_packet.py` checks
hashes, exact-byte profile/schema outcomes and finite-table consistency, without
importing the materializer or implementing model/policy/sink execution.

Use an isolated environment and `review/requirements.txt`; commands and actual
outcomes are in AUDIT.md. A regeneration must be byte-identical to the committed
fixtures. If a semantic error is found after review, preserve this revision and
publish a new version/ledger entry; do not silently repair a preregistered result.
A real candidate or mutation test has not run. This corpus is a specification
against which another engineer can implement and independently inspect one.


The context is a closed, frozen research record, not a public authority API.
Its complete key set is present in every case; no model input is merged into it.
Only the exact catalog aliases named in registry are resolved. Seeded initial_rows
are trusted prior-session markers (request_ref, owner_sequence), not evidence
that this audit reproduced a prior commit. Their authority is an explicit fixture
precondition. Unexpected context fields/types, unknown aliases or tampered hashes
are invalid fixture/harness input, not an extra successful security rejection.


`stored_interpretation` is an offline reading of a schema-valid stored response.
`interpretation` is null if the pipeline stops before response consumption;
it never grants authority. `outcome_reasons` retains SINK_FAILURE separately
from a valid P decision, and failure_subtag retains Timeout/Cancelled/
TransportFailure. Owner sessions are identified by the trusted case label and
fixture pin; replay protection is not claimed across separate reset fixtures.
