# Independent oracle specification — corpus revision 1

This is a specification for a future implementation, not a candidate runtime or
an executed enforcement oracle. It resolves the ambiguities found at cd7d4620.
For the frozen corpus, this document and CORPUS.md take precedence over earlier
illustrative language. No new production authorization semantics are adopted.

## Inputs, independence and output

Inputs are exact request/response bytes (or a typed absence event), an immutable
case context, the object catalog and a finite event plan. All bytes/contexts are
pinned in `corpus/cases.jsonl` and `corpus/objects.json`. Read JSONL records as
fixture data, not model output or trusted executable configuration. A case runs
in a fresh owner session except where its context explicitly seeds a spent row.

The oracle must not import candidate/A/B parsers, type coercions, combiners,
capability helpers, source validation, event reducers, fixture generator or
receipt builders. Use a different parser implementation for cross-language
confirmation before claiming independent end-to-end validation. The research
checker presently uses Python jsonschema; that is one implementation, not a
second lab. Sharing mathematical definitions, trust anchors and the selected
workload is unavoidable and must be listed, not described as independent evidence
of their scientific adequacy. Independently review expected case annotations;
never generate an expected authorization result by calling the candidate.

Return `(pipeline_stage, interpretation, decision, reasons with subjects,
primary outcome, admission results, final owner state)`. An interpretation is the
model's endorse/reject/review field. A decision is a **system** result; they are
not interchangeable. Primary outcome is never overwritten by a later duplicate
request: F08-C05 commits the primary, rejects B, and retains both observations.
An absent response has no interpretation or output ref. No model field is an
input to the authority store, grant, budget or replay database.

## 1. Wire interpretation and structural validation

Process each message in this order; stop at its first profile failure:

1. Check raw byte length (request <=8192, response <=4096).
2. Decode strict UTF-8; reject a leading BOM (no stripping).
3. Parse one JSON value with optional surrounding JSON whitespace. Reject trailing
   non-whitespace, duplicate keys anywhere, NaN/Infinity and invalid JSON syntax.
   Preserve exact decimal numeric values; do not underflow 1e-4000 to zero.
4. Reject unpaired surrogate code points in keys/strings, including escaped ones.
   Do not Unicode-normalize strings or repair bytes.
5. Count object/array nesting: scalar=0; container=1+maximum child-container
   depth, maximum of empty children=0. Reject depth >4.
6. Apply the frozen Draft 2020-12 schema in `schemas/request.json` or
   `schemas/response.json`. Resolve only local schema refs. Integer means a
   mathematical number with zero fractional part: 1.0 and 1e0 are integers;
   true is not. String length counts Unicode code points, not UTF-8 bytes.

These layers are distinct. Duplicate-key rejection and scalar/byte/depth rules
are a wire profile, not Draft schema assertions. Schema validation alone cannot
check cross-message equality, source authority, timing, replay or capabilities.
Profile failures are INVALID_REQUEST/INVALID_RESPONSE with their profile detail;
schema failures carry keyword and instance JSON pointer. They are not G6 budget
failures. An incomplete Response is syntax-invalid; Timeout/Cancelled/
TransportFailure is a typed MODEL_FAILURE, not the same byte string as `b''`.

Both messages are reviewed independently offline in the corpus annotations. In
an actual pipeline, invalid request/binding terminates before consuming response;
its offline schema-valid response does not authorize continuation.

Protocol order is request wire/schema, RequestBinding, then observation of the
response event. Response requires present bytes; Timeout/Cancelled/TransportFailure
requires absent bytes and no output ref. Contradictory event/byte metadata is an
invalid fixture/harness input. Request failure takes precedence over any stored
response event. A bound request with a typed absence stops at ModelFailure/D
and preserves the failure subtag before any response parse or guard evaluation.

## 2. Binding checkpoints

After a structurally valid request, compare the independently registered caller
intent: request ref must be `id(request, raw_request) == expected_request_ref`;
model/artifact/policy refs must equal approved_model_ref, expected_artifact_ref,
expected_policy_ref. Request deadline equals context deadline. Evidence refs
must be sorted and exactly equal the ordered `required_evidence` ref list.
Any mismatch produces RequestBinding / D / BINDING; stop before response.

After a valid response, check request_id, request_ref, model_ref and ordered
refs against the exact admitted request. Independently compare observed_model_ref
and observed_provider against approved_model_ref and approved_provider. This is
an externally recorded fixture observation, not an echoed model claim. A mismatch
produces ResponseBinding / D / BINDING and stops. A self-consistent rehash cannot
change caller intent, authenticate a provider or create a grant.

Request/schema failures and typed model failures yield D as a system rejection
projection, with `guards=null`. They do not mean a model returned DENY. For a
binding rejection the conceptual partial vector is DUUUUU; never fill unchecked
guards with P. The mathematical table's binding-prefix row is illustrative;
corpus records use the typed stopping stage rather than a fabricated full vector.

## 3. Declarative authorization facts

Only after both binding checkpoints succeed, determine these facts directly.
Do not implement permission by converting a model recommendation into a grant.

- **Recommendation:** endorse contributes no objection; reject is MODEL_REJECT;
  review is unresolved MODEL_REVIEW. It can veto/defer but supplies no authority.
- **Evidence:** for each required ref in request order, resolve only
  `context.registry[ref]` into a catalog object. Absent entry is EVIDENCE_MISSING.
  Otherwise recompute the domain-separated evidence digest and compare required
  artifact, predicate, source and epoch to the parsed observation. If any is
  wrong, emit EVIDENCE_INVALID for that ref, ignoring its truth bits. For a valid
  record, 10 is supported, 00 is EVIDENCE_MISSING, 01 EVIDENCE_REFUTED, 11
  EVIDENCE_CONFLICT. Keep each occurrence and its subject ref; never deduplicate
  two missing observations into an ambiguous explanation. A source-name field
  only has standing because the fixture registry/requirements are trusted inputs;
  this does not implement real-world signatures or trustworthy test execution.
- **Authority:** policy_present must be true and the grant must exactly match
  `(context.caller, request.model_ref, append_report, request.artifact_ref,
  request.policy_ref)`. Absence/wrong tuple gives one CAPABILITY reason. A
  present policy marked revoked gives POLICY_REVOKED. Unavailable authority gives
  POLICY_UNAVAILABLE. When policy_present=false its state field is inactive
  metadata, not a conflicting grant; do not infer active authority from it.
- **Freshness:** spent replay state gives REPLAY; request.policy_ref unequal to
  current_policy_ref gives STALE_POLICY. Unavailable replay lookup gives
  REPLAY_UNAVAILABLE. The key is the raw **request ref**, not request_id alone
  or output ref. A new caller-approved request is not an automatic model retry.
- **Resources:** `now_tick <= deadline_tick` is live, including equality. A
  later tick gives DEADLINE. Effective allowance is
  `min(request.budget_units, remaining_units)`; less than one gives BUDGET.
  An in-range request of 8 with a host allowance of 1 is capped, not rejected.
  Only successful commitment consumes one unit. No physical inference budget
  or parse-work charge is modeled.

Order diagnostics by recommendation (G2), evidence in ref order (G3), authority
(G4: CAPABILITY, POLICY_REVOKED, POLICY_UNAVAILABLE), freshness (G5: REPLAY,
STALE_POLICY, REPLAY_UNAVAILABLE), resources (G6: BUDGET, DEADLINE). Keep unresolved
facts even when a different hard failure dominates the decision. Binding errors
terminate earlier and therefore have a single BINDING reason. Evidence reason
subjects are explicit refs; other initial reasons name the request ref.

Permission is the direct conjunction of: binding success; endorse; all required
valid evidence records equal 10; present active policy and matching grant;
fresh request at the current policy ref; live deadline and effective allowance
>=1. This formula must be implemented independently of the candidate meet.

For diagnostics and the enum/meet comparison, any hard failure yields D; otherwise
any unresolved fact yields U; otherwise P. Hard failures are MODEL_REJECT,
EVIDENCE_REFUTED, EVIDENCE_CONFLICT, EVIDENCE_INVALID, CAPABILITY, POLICY_REVOKED,
REPLAY, STALE_POLICY, BUDGET and DEADLINE. Unresolved facts are MODEL_REVIEW,
EVIDENCE_MISSING, POLICY_UNAVAILABLE and REPLAY_UNAVAILABLE. Missing observations
cannot be made supported by text. Classify reasons first, derive the direct
Boolean permission predicate separately, and assert P iff that predicate holds.
Absent policy/empty required guard set never permits via the empty-meet identity.

## 4. Time, final admission and the sole owner

A valid message/immutable observation snapshot stays fixed through this fixture
session. Only policy_state, replay_state, remaining_units, rows and owner_sequence
may change through declared events. No clock advances implicitly: now_tick is a
fixture value. Arbitrary mutable observations, grant updates, provider swaps or
wall-clock races are outside the 24-plan suite; they need new explicit events.
This is a limitation, not permission to omit their validation in a real system.

`R` is a recheck of the primary decision, not a transferable permit token.
`A` is a primary admission **probe** to the trusted broker; `B` is its duplicate.
Probes are not protected effect dispatches. The broker recomputes all facts,
including live authority/replay/resource state, under the same owner step that
would dispatch/commit. A failed probe dispatches no protected effect. If the
recomputed decision is D/U, return that decision/reasons without changing state.
If P, dispatch exactly the one approved in-memory action. On sink_fault, return
EffectFailed/SINK_FAILURE, no row/receipt, no budget/spent/sequence update. On
success add one bound row, set replay state spent, decrement remaining_units by
one, increment owner_sequence by one and issue a Committed receipt. Checking,
writing and marking spent are one abstract linearized transition, not a claim of
crash durability or observable external-world atomicity.

`V` revokes the currently pinned policy and never undoes an earlier committed
row. There is no reactivation event. The final receipt must distinguish P before
admission from actual Committed or EffectFailed. R cannot override V or replay
state. Reconstruct every before/after state and every probe separately; a later
rejection does not erase a legitimate earlier commitment.

## 5. Shared assumptions and nonclaims

Both future arms and the oracle assume honest caller intent, known fixture
observations, approved provider identity, a continuous non-rolled-back store,
one owner, deterministic parsing/event rules and collision-resistant digests.
These assumptions, the D-dominant decision policy and workload selection are
shared scientific hypotheses. A/B/oracle agreement would establish conformance
to this contract, not its optimality, human usability, novelty or genuine model
identity on an opaque hosted service. The current checker validates data/schema
and arithmetic consistency only; it does not execute the above authorization
predicate or enact a sink transition.
