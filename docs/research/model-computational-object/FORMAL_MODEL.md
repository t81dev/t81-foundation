# Formal candidate: untrusted model, trusted effect boundary

Status: proposed mathematical contract, not an implemented T81 language type or
production security contract. The experiment in [EXPERIMENT_PROPOSAL.md](EXPERIMENT_PROPOSAL.md)
uses synthetic responses only. Existing Axion's five verdicts are not remapped
or changed by this proposal.

## 1. Object, invocation and identity

Let an object descriptor be `(role, revision_ref, input_contract, output_contract,
provider_binding, declared_operations, observation_profile)`. Granted capabilities
are supplied by a separate authority, never by this descriptor or the model.
Operations are `describe`, `propose`, `validate` and `admit_effect`; only the
trusted boundary implements the last two. Model replacement creates a new
revision/binding and requires explicit admission under the old permission ceiling.

An execution is a relation `R(model_revision, request, environment, randomness,
external_events, response)`, generally not a pure function of request. A verifier
is a total bounded function `V(request_bytes, response_bytes, trusted_snapshot)`.
Time, policy updates, unavailable lookups and resource observations must be
explicit inputs/events; omitting them does not make behavior deterministic.

| Identity | What is bound | What it does not establish |
| --- | --- | --- |
| Logical role | Caller-selected name and a versioned alias-resolution record | Immutable weights, authority or behavior |
| Local model revision | Exact manifest bytes listing weights, tokenizer, config and adapters by digest | Equivalent inference across runtimes or devices |
| Hosted binding | Endpoint/provider, requested model name and reported revision, explicitly tagged opaque | Actual hidden model identity; an advertised label can be stale or change |
| Execution | Model/contract refs, request, environment/provider declarations, policy version, run nonce and ordered observed events | Visibility of unobserved provider internals |
| Output | Exact raw response bytes | Correctness, uniqueness of execution, authorship or authorization |
| Receipt | Bound decision, observed sink outcome and verifier/authority identity | Physical occurrence outside the witness's observation scope |

For the proposed fixture protocol only, define
`id(kind, bytes) = "sha256:" + hex(SHA256(ASCII(kind) || 0x00 || bytes))`,
with kind in `model`, `request`, `output`, `execution`, `artifact`, `policy`,
`evidence`, `receipt`. Inputs are the exact stored bytes, with lengths known to
the parser; digest equality is assumed collision-resistant. Different JSON
whitespace therefore gives a different request/output identity. This is an
experiment-local binding convention, not CanonFS's production hash contract or
a new canonical serialization standard. Cryptographic attribution requires an
independently trusted signer or store; a digest alone provides none.

Local model replacement with the same interface need not preserve responses,
accuracy or latency. Replacement is admitted only if its provider is approved,
its revision and contract refs are pinned, and capabilities are no greater than
the caller's existing grant. A hosted manifest pins a description, with unknown
implementation fields explicitly unknown; never fill them with guesses.

## 2. Four distinct determinism properties

| Property | Conditional claim | What is possible for mainstream LLM integration |
| --- | --- | --- |
| A: computation | Same fully specified state, events, kernels and random choices yield the same result | Possible for a constrained audited local configuration; not established for an arbitrary model/backend. No bit-exact claim for an opaque hosted model. |
| B: interface | Same raw messages, state and event order yield the same parsing, error and wrapper transitions | Feasible in a bounded deterministic verifier, even when response content varies. Schema conformance constrains structure, not semantic truth. |
| C: authorization | Same policy version, capability state and observed request yield the same authorization decision | Feasible for an external deterministic reference monitor. Authorization still changes when revocation or other policy inputs change. |
| D: evidence | Same declared records and trust anchors yield the same verification result | Feasible for bounded record validation. Record production includes nonces, timestamps or observations and need not produce identical bytes across executions. |

Stochastic decoding changes random choices; seed reuse alone does not pin kernel
order, floating-point rounding or scheduling. Even temperature zero can leave
kernel and tie-breaking variation. Local reproducibility requires the relevant
runtime/device/thread/batching configuration as well as model and tokenizer.
[PyTorch's reproducibility notes](https://docs.pytorch.org/docs/2.14/notes/randomness.html)
explicitly bound cross-version/platform reproducibility. Hosted revisions, hidden
prompts and infrastructure add unobserved variables. External tools contribute
results and event order, not deterministic world behavior. Scheduling and wall
clocks must be captured as events for wrapper replay. Replaying a recorded
response tests B–D, not independent reexecution of A. Untrusted content cannot
modify the verifier or become its policy input by asserting that it is trusted.

## 3. Exact request and response shapes

These embedded JSON Schemas use Draft 2020-12. They are a proposed frozen fixture
contract. No remote schema resolution or execution is permitted. In addition to
schema validation, reject duplicate JSON keys, non-UTF-8, trailing material,
non-JSON numbers, nesting beyond 4 containers, more than 8,192 input bytes or
4,096 response bytes. Process exactly one complete object; never repair invalid
output or interpret a rationale as instructions. JSON string limits count Unicode
code points; byte bounds apply before parsing. The host owns all pins and budgets.

Request schema:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "additionalProperties": false,
  "required": ["schema", "request_id", "model_ref", "operation", "artifact_ref", "policy_ref", "evidence_refs", "budget_units", "deadline_tick"],
  "properties": {
    "schema": {"const": "mco.request.v1"},
    "request_id": {"type": "string", "pattern": "^[0-9a-f]{32}$"},
    "model_ref": {"$ref": "#/$defs/ref"},
    "operation": {"const": "append_report"},
    "artifact_ref": {"$ref": "#/$defs/ref"},
    "policy_ref": {"$ref": "#/$defs/ref"},
    "evidence_refs": {"type": "array", "minItems": 1, "maxItems": 3, "uniqueItems": true, "items": {"$ref": "#/$defs/ref"}},
    "budget_units": {"type": "integer", "minimum": 1, "maximum": 8},
    "deadline_tick": {"type": "integer", "minimum": 0, "maximum": 65535}
  },
  "$defs": {
    "ref": {"type": "string", "pattern": "^sha256:[0-9a-f]{64}$"}
  }
}
```

Untrusted model response schema:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "additionalProperties": false,
  "required": ["schema", "request_id", "request_ref", "model_ref", "recommendation", "evidence_refs", "rationale"],
  "properties": {
    "schema": {"const": "mco.response.v1"},
    "request_id": {"type": "string", "pattern": "^[0-9a-f]{32}$"},
    "request_ref": {"$ref": "#/$defs/ref"},
    "model_ref": {"$ref": "#/$defs/ref"},
    "recommendation": {"enum": ["endorse", "reject", "review"]},
    "evidence_refs": {"type": "array", "minItems": 1, "maxItems": 3, "uniqueItems": true, "items": {"$ref": "#/$defs/ref"}},
    "rationale": {"type": "string", "maxLength": 512}
  },
  "$defs": {
    "ref": {"type": "string", "pattern": "^sha256:[0-9a-f]{64}$"}
  }
}
```

Request evidence refs must be lexicographically sorted and must equal the
host policy's required set for this artifact/operation. The response must echo
that exact ordered set, request ID, computed request ref and admitted model ref.
Hash verification does not validate the truth of the referenced observations.
A required ref absent from the registry is missing evidence (pair 00); a known
entry with a mismatched source, digest, predicate or epoch is invalid evidence.
The verifier resolves refs only in an immutable, preloaded fixture registry;
URLs, arbitrary paths and model-supplied credentials are never dereferenced.

A trusted context has this closed product grammar (not model-writable JSON):

- `caller`, approved `model_ref`, expected request/artifact/policy refs;
- `policy_state ∈ {active, revoked, unavailable}` and current policy ref;
- grant tuple `(caller, model_ref, append_report, artifact_ref, policy_ref)`;
- evidence map: each required ref binds artifact, observation-predicate,
  source and epoch plus independent Boolean `supports` and `refutes` bits;
- `replay_state ∈ {fresh, spent, unavailable}` and a serialized sink owner;
- `now_tick ∈ [0,65535]`, `remaining_units ∈ [0,8]`, `deadline_tick` and
  a bounded observed response event or `Timeout | TransportFailure | Cancelled`.

A matching grant is necessary; a request's policy_ref or budget_units is only a
claim until checked against that context. The first fixture experiment uses a
single honest authority/store and logical ticks, not signed remote attestations.

## 4. Decision algebra and loss of information

For the narrow all-required-guards operation, use `S={D,U,P}` for
DENY, DEFER, PERMIT, ordered `D < U < P`. Define `a ∧ b = min(a,b)`:

| ∧ | D | U | P |
| --- | --- | --- | --- |
| D | D | D | D |
| U | D | U | U |
| P | D | U | P |

This operation is associative, commutative and idempotent because min on a total
order is; P is its identity and D its absorbing element. Thus an all-required
composition is P iff every guard is P, D iff at least one is D, otherwise U.
This is a decision meet, not a complete new logic, probability or confidence
score. Its conjunction table matches the familiar three-value conjunction under
F/U/T relabeling, but policy meaning comes from the guard definitions. Do not
invent negation, permit-overrides or an empty-policy permit from that resemblance.
The mathematical empty meet P must be overridden operationally: absent required
policy or guard set is a denial, not a vacuous authorization.

Three states cannot losslessly encode four independent evidence configurations:
`(supports, refutes) = 00,10,01,11`. By the pigeonhole principle any map into S
merges at least two. The proposal preserves the pair and reason codes, projecting
00 to U (missing), 10 to P (supported), 01 to D (refuted), 11 to D (conflict).
A denial projection does not erase whether evidence was false or contradictory.
The richer comparator is informed by [FDE's Both/Neither distinction](https://cdn.aaai.org/ocs/17599/17599-77711-1-PB.pdf);
this security projection is not adoption of every FDE designated value.

| Representation | Expressiveness and verification burden | Execution/state-space implication |
| --- | --- | --- |
| Boolean acceptance plus explicit tagged error/defer outcomes | Can express exactly the same outcomes; error paths need tests | Ordinary binary machine representation; no intrinsic loss if tags are retained |
| Binary-encoded enum D/U/P | Isomorphic to S via relabeling; use the same table | Two bits suffice per independent enum; actual ABI may use more |
| Formal three-state meet | Gives simple composition laws and explicit unresolved state | k guards still have 3^k valuations; implementation can be the same binary enum |
| Four-valued evidence plus lifecycle/errors | Retains conflict versus absence and independent execution outcomes | More distinctions to verify, but hiding them in rationale does not remove their security relevance |

For six guards there are 3^6=729 abstract assignments in both enum and meet arms.
A naive two-Boolean encoding has 4^6=4096 bit patterns, but validity constraints
can leave the same 729 reachable states; this is not evidence of a ternary win.
Three-valued semantics alone cannot establish a lower instruction, energy or
hardware cost. Existing authorization systems also retain richer results such
as NotApplicable and Indeterminate; [XACML 3.0](https://docs.oasis-open.org/xacml/3.0/xacml-3.0-core-spec-os-en.html)
is an explicit prior comparator, not an implementation to collapse here.

## 5. Guard, lifecycle and evidence semantics

Use six guards, retaining all reasons in fixed guard order:

| Guard | P | U | D |
| --- | --- | --- | --- |
| G1 binding/structure | Valid response and all independent pins/echoes match | Not used | Malformed/schema-invalid/substituted response or binding |
| G2 recommendation | endorse | review | reject |
| G3 observations | Every required, correctly bound observation has pair 10 | At least one 00 and no refutation/conflict | Any 01/11, bad digest/source/epoch/binding |
| G4 capability/policy | Matching independently granted tuple and active policy | Authority lookup unavailable | Absent grant, revoked policy, wrong operation/caller/policy |
| G5 freshness | Fresh request and current policy ref | Replay lookup unavailable | Spent request or stale policy ref |
| G6 resources | Live deadline, response in bounds, authorized budget can cover one sink admission | Not used | Exhausted budget, deadline passed, bounds exceeded |

An untrusted request with malformed structure or an observed model failure is
rejected before this six-guard phase. Absent policy is G4=D; unavailable policy
is G4=U. Unchecked guards remain U, never P. Recommendation can withhold progress
but cannot grant authority. Reasons are typed facts, not free-text justification.

Lifecycle: `Received → Preflight → ProposalObserved → Validated → Decided`.
Only P may proceed to `FinalRecheck → Committed`. Final recheck uses current
policy/replay/budget state with the single sink owner; if it changes, recompute
and stop or defer. D/U never invoke the sink. A fresh retry is a new request;
DEFER does not authorize an automatic retry or resource increase.

Terminal outcomes are distinct tagged alternatives:
`Rejected(reason) | Deferred(reason) | Committed(receipt) | EffectFailed(reason)`.
Reason codes are `INVALID_REQUEST, INVALID_RESPONSE, MODEL_FAILURE, BINDING,
MODEL_REJECT, MODEL_REVIEW, EVIDENCE_MISSING, EVIDENCE_REFUTED,
EVIDENCE_CONFLICT, EVIDENCE_INVALID, CAPABILITY, POLICY_REVOKED,
POLICY_UNAVAILABLE, REPLAY, REPLAY_UNAVAILABLE, STALE_POLICY,
BUDGET, DEADLINE, SINK_FAILURE`. Rejected(MODEL_FAILURE) must retain the observed Timeout/TransportFailure/Cancelled
subtag in its execution record. Multiple guard reasons retain the listed guard
order; within G3 use request evidence order. Preflight/transport failures retain
their typed phase instead of pretending an LLM returned DENY. No receipt labeled
Committed is produced merely because a decision was P.

The fixture sink atomically checks the final grant/replay/budget state, inserts
one in-memory row `(request_ref, artifact_ref, model_ref, policy_ref,
output_ref, verifier_ref, decision=P, outcome=Committed)` and marks the request
spent while consuming one unit. It produces a receipt ref over those exact
observed fields plus an owner sequence number and execution ref. The execution
record binds the ordered events, raw refs and final observation. A sink failure
has no committed row. This is an abstract single-owner transition, not crash
atomicity, distributed exactly-once behavior or an authenticated receipt format.

## 6. Safety proof obligations

Assuming the verifier and sole sink owner enforce this transition relation:

1. A committed row implies G1…G6 were P at final admission, with the matching
   live grant. Proof: the sole committing transition has exactly that precondition.
2. A missing, refuted or conflicting required observation cannot authorize a row.
   Its G3 is U/D and the meet cannot be P.
3. Modifying rationale cannot expand effect authority: it is not read by any
   guard or sink action. It can still change output identity.
4. Replacing the model cannot enlarge its grant: the final tuple check binds
   model_ref and capability ceiling independently of descriptor claims.
5. At most one committed row per request holds within an honest continuous
   owner session: freshness is tested and spent is set in one serialized step.
6. Model nondeterminism changes possible observations, not the allowed effect
   transition. Progress and recommendation correctness remain unproved.

These are deductions from a proposed machine, not model-checked or executable
results. Parser soundness, full mediation, store integrity and correspondence
between code and this model are experimental obligations, not assumptions that
can be silently declared verified.
