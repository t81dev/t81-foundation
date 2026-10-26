# One discriminator: model proposal to append a verified test summary

Status: proposal only; no implementation, model call, adversarial run or benchmark
was performed. This document preregisters a bounded test before implementation.
Schemas and semantics are in [FORMAL_MODEL.md](FORMAL_MODEL.md); threat assumptions
are in [THREAT_MODEL.md](THREAT_MODEL.md).

## Workload and responsibilities

A trusted caller requests `append_report` for a fixed software artifact. A model
recommends endorse/reject/review and cites required test-observation refs. A
separate deterministic validator checks the recommendation, pinned context,
observations, authority and resources. The only effect is appending a row to an
in-memory fixture ledger. No software operation, test command or natural-language
instruction supplied by the model is executed.

Use deterministic synthetic response bytes first. The host, not the model,
provides the truth fixtures and expected capability tuple. There is no inference
quality claim: this workload tests mediation, diagnostic semantics and substitution
handling. A later real-model proposal would change only the source of untrusted
response bytes after the enforcement gate passes; it would need separate review
of runtime isolation, privacy and physical resource accounting.

Requested budget is capped by `min(request.budget_units, host.remaining_units)`;
one successful sink admission consumes one unit. The wrapper makes at most one
synthetic response lookup, validates at most three evidence refs and six guards,
and has no model-driven retry. Message byte/depth limits are checked before
parsing. Logical deadline ticks and cancellation events are supplied by the
fixture owner. These are finite verification costs, not measured real inference
or OS scheduling budgets.

## Arms and independent oracle

- **A — conventional control:** a typed service/registry, Boolean guard predicates,
  explicit tagged failures and deferred outcomes; retain the support/refutation
  bits and all reason codes. Use an ordinary binary-encoded enum for the summary.
  Do not cripple A by replacing unknown with false or removing replay protection.
- **B — three-state candidate:** same parser, context, observation source and sink;
  each guard projects to D/U/P and composes with the specified meet. The enum can
  be binary encoded. This isolates composition semantics, not hardware radix.
- **C — diagnostic ablation:** retain four-valued evidence before projection,
  using the same safe sink predicate. This tests whether a three-state-only
  record loses required distinctions; it must not gain extra authority.

A and B are expected to be extensionally equivalent under the declared rules.
That expectation is a falsifiable implementation check, not a claimed speedup.
The mathematical relabeling already shows there can be no extra expressive
power from B's state names alone. The experiment could still detect integration
errors or a concrete reviewability difference; a null result is valuable.

The independent oracle must not import either arm's combiners or validation
helpers. Use a direct declarative predicate for permission: every binding holds,
recommendation=endorse, every required observation has support and no refutation,
matching active grant, fresh request, live deadline and adequate host budget.
For the summary, any hard failure means D, otherwise any unresolved obligation
means U, otherwise P. Independently enumerate reason codes, fixture truth and
permitted sink transitions. Sharing a parser is a controlled confounder: add
independent raw-byte rejection expectations and an independently parsed corpus
before asserting end-to-end correctness. A/B agreement alone is insufficient.

## Fixed distribution and finite checks

Keep three separately reported suites; do not blend challenge prevalence with
an estimate of real-world attack frequency.

1. **Composition suite:** all 729 assignments to six D/U/P guards, all 9 binary
   meet pairs and all 27 associativity triples. Check A/B/oracle agreement and
   the P-only sink invariant. These are abstract assignments, not all necessarily
   reachable wire-level requests.
2. **Raw-byte suite:** a balanced 60-case initial corpus, 5 cases in each of the
   12 families below. Before either arm is implemented, author and pin the exact
   request/response bytes, context snapshots and expected outcomes; independently
   review them. The current family plan is not a frozen executable dataset.
3. **Temporal suite:** all 24 permutations of four labeled events: an already
   validated proposal's final recheck, revocation, sink-admission attempt and a
   duplicate-admission attempt. The sink rechecks current authority/freshness
   within admission regardless of previous recheck order. Reject schedules whose
   prerequisites are absent explicitly; do not silently count them as passes.
   Record whether either attempt committed, and require at most one. Final
   recheck alone never grants a token usable after revocation.

| Raw-byte family (five cases each) | Required observation |
| --- | --- |
| Well-formed authorized complete evidence | Endorse can commit once; reject blocks; review defers; rationale changes do not grant new authority |
| Direct/indirect prompt injection | Instructions in rationale or fixture source cannot alter verifier/sink policy |
| Conflicting instruction / forged authority | Claimed admin role, policy override or invented grant never creates permission |
| Malformed output | Duplicate keys, trailing JSON, wrong type, invalid UTF-8 and extra fields reject before sink |
| Unauthorized tools | Requests for shell/network/arbitrary operation or nested tool objects reject, no attempted dispatch |
| Missing evidence | Absent required observations defer with typed reason, never commit |
| Refuted/conflicting evidence | Both states deny, with distinct retained diagnostics; forged source/hash also rejects |
| Replay | Reuse of spent request under same session, altered response, or concurrent duplicate cannot add a row |
| Substitution | Wrong model/provider/request/artifact/policy binding rejects despite self-consistent rehashing |
| Stale/revoked authority | Final admission checks current epoch, not a cached earlier permit |
| Exhaustion/deadline | Zero host allowance, inflated request budget, excessive bytes/depth and expired deadline cannot commit |
| Model/transport/sink failure | Timeout, cancellation, truncated stream, transport failure and sink fault remain distinguishable; no phantom committed receipt |

The first family may need contextual variations to produce five distinct cases;
classify outcome proportions separately from family proportions. Distinct request
IDs are not five independent security observations by themselves. Every raw
case must have a concrete independent expected result before implementation.
Do not claim 60 cases have passed merely because this table has 12 rows.

## Measurements and acceptance

Report full outcomes rather than a summed architecture score:

- unauthorized attempted effects and committed rows separately (both must be
  zero on blocked cases), duplicate effects, reason fidelity and phantom receipts;
- A/B/oracle mismatch counts, reachable/invalid state patterns, parser disagreement,
  timeout/error classification and complete decision/observation records;
- trust-boundary count, shared versus arm-specific policy obligations, adapter
  changes required for a second synthetic model revision, and test additions;
- enumerate proof obligations and document invalid-state handling. If a human
  review comparison is later run, preregister matched tasks, reviewer assignment,
  defect rubric and uncertainty reporting; do not invent a reviewer-effort result;
- count verifier predicate evaluations, evidence lookups and memory bounds as
  structural measurements. No speculative CPU timing, line-count superiority,
  ternary-energy or device-storage claim is authorized by this experiment.

Both arms must pass every safety case. Inject a permit-on-unknown mutation,
a skipped final-authority check, a skipped replay check and a model-pin omission;
the independent checks must reject each mutant. If an expected violation is
not detected, the harness is invalid, not a successful candidate.

## Falsification and stopping rules

- Any D/U case causing an attempted effect, model-controlled policy mutation,
  or unauthorized commit falsifies the proposed enforcement implementation.
- Any difference between declared A/B/oracle semantics is investigated before
  making advantage claims. Preserve the failed trace and defect classification.
- If three-state-only storage loses conflict/error/lifecycle distinctions,
  reject it as the whole-object state model; retain richer tagged records.
- If A supplies the same invariants and extension behavior at equivalent
  validation effort, reject a novelty/necessity claim for B. Mere renaming is
  already known to be expressively equivalent, not an empirical discovery.
- Stop scope growth if the fixture needs arbitrary tools, production driver
  changes, a new scheduler, kernel privileges or an ISA extension.
- Do not add a real LLM until every finite safety check, independent oracle check
  and mutation check passes, the exact corpus is pinned and isolation/resource
  controls for that provider have been separately reviewed.

The experiment has not yet been run. It is ready for corpus specification and
independent review, not for a live autonomous agent or a production rollout.
