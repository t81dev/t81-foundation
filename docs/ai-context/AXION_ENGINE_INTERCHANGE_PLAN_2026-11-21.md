# Interchangeable AI engines for Axion

**Status:** NON-NORMATIVE design backlog. Implementation deferred. This note
does not authorize a prototype, a schema change, or a hold change.

**Date:** 2026-11-21. Rechecked 2026-11-23 against `main`
`5a005b2dea5e1eb262d4af45698f8338d16b137c`.
The 2026-11-21 baseline was
`c314f6f6fd9ed1a90414132344a5ed5a5dab3f6f`.
`git diff` from that baseline to the recheck pin was empty for the route and
classify examples, both composition tests, the stable-baseline and family
contracts, the chain catalog, ADR-0007, the HD01 memorandum and preparation
record, the F01 review plan, draft RFC-00D4, the 2026-11-18 status note, and
`tools/cli/ai/ai_cli_shared.cpp`.
[ACTIVE_RESEARCH_HANDOFF.md](ACTIVE_RESEARCH_HANDOFF.md) gained the 2026-11-22
live queue. [v2.1.0 notes](../releases/v2.1.0.md) gained a Python package audit
section; the preserved-holds paragraph is unchanged. This pass read those
files. It did not build or run them.

The immediate work is a paper review of the routing example. Later stages are
candidate work and need their own authorization. This plan does not replace
the 2026-11-22 live queue in
[ACTIVE_RESEARCH_HANDOFF.md](ACTIVE_RESEARCH_HANDOFF.md).

## Recorded holds

These dispositions are copied from the documents named here. This recheck did
not re-measure them.

| Hold                     | Recorded disposition                                                  | Where this recheck read it                                                                                                                                           |
| ------------------------ | --------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `AXION-AI-001` HD01      | OPEN. No authority model selected.                                    | [HD01 memorandum](../research/proposals/AXION-AI-001-HD01/MEMORANDUM.md)                                                                                             |
| `AXION-AI-001` HD02–HD06 | Named open human gates. No separate in-tree memorandum for HD02–HD06. | [ADR-0007](../architecture/adr/ADR-0007-axion-ai-001-governance-broker.md) and the [HD01 preparation record](../research/proposals/AXION-AI-001-HD01/PREPARATION.md) |
| `RFC-00D7-F01`           | M1–M6 STOP. `F01-HOST-BOLQ-v1` NOT ADOPTED.                           | [F01 review plan](../research/proposals/RFC-00D7-F01-BOLQ-MIGRATION/REVIEW_PLAN.md)                                                                                  |
| SCG-08 Gate 2            | Recorded NO-GO in the handoff and release notes.                      | [Active handoff](ACTIVE_RESEARCH_HANDOFF.md), [v2.1.0 notes](../releases/v2.1.0.md)                                                                                  |
| H01 / Investigation 10A  | Recorded BLOCKED.                                                     | Same handoff and release notes                                                                                                                                       |

There is no `spec/rfcs/RFC-00D7` file. The F01 protocol bytes are not in this
tree. The review plan records operative protocol SHA-256
`d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
This pass did not recompute that digest.

## Starting point and first milestone

In the [route example](../../examples/ai-and-inference/model-load-canonfs/run_route_fixed_path_selection.sh),
the CLI runs `t81 ai task route-fixed` with a policy file, a CanonFS root, and
`--mode strict_deterministic`. The script fetches the result from CanonFS,
reads `route`, and maps `A`, `B`, and `C` to `routes/a.target`,
`routes/b.target`, and `routes/c.target`. It writes that action file on the
host, stores it with `canonfs put-file`, writes the downstream
`t81.ai.task.route-fixed.path-selection-record.v1` record, and stores
`t81.ai.task.route-fixed.bundle.v1`. The host write and the CanonFS put are
separate steps. The script has no revocation or crash-recovery logic. Whether
that write can be revoked or recovered atomically is a design question.
[HD04](../architecture/adr/ADR-0007-axion-ai-001-governance-broker.md) still
leaves the local transactional primitive unselected.

The [composition test](../../tests/cpp/ai_task_route_fixed_composition_test.cpp)
compares result, provenance, record, action, and bundle references across a
repeated run and a second CanonFS root, and compares the retrieved artifact
text. That is what the test source checks. This pass did not execute it, and
did not execute the demo, so the route letter produced by the default input
`greet hello` is not reported here.

First milestone for a future userspace prototype: the same host mapping
consumes a valid route from either a deterministic engine or a sampler. A
reader can recover which proposal was used, what permission applied, and what
happened to the effect, including an unknown outcome. This milestone is
interchange at task boundaries. Engine-private token history and KV caches
stay behind the adapter. No such adapter is in the tree today.

## Proposed contract for one bounded decision

The tables below are proposed review material. They are not a current schema
or an accepted contract.

| Surface                      | Proposed minimum                                                                                                                                                                                          | Review question                                                                                 |
| ---------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------- |
| Request                      | Versioned task semantics and input references, output vocabulary, request identity, budget, and required replay guarantee. Distinguish a retry of one invocation from a new sample on the same input.     | Can the host reject an incompatible profile before disclosing input or dispatching work?        |
| Engine manifest              | Supported tasks, model/adapter and execution profile identity, limits, randomness controls, and evidenced replay guarantees. Mark unknown provider details explicitly.                                    | Can a local, remote, or hardware implementation qualify using the same contract?                |
| Result and invocation record | Typed value or failure linked to the request and actual engine invocation. Capture the sampling policy and seed or randomness information when available. Record which identity claims could be verified. | Can the consumer distinguish malformed output, timeout, cancellation, late output, and success? |
| Host decision and effect     | Deterministic interpretation of the typed value, current authorization for the concrete target, a durable operation identity, and an observed outcome.                                                    | Can an old result be inspected without granting a fresh effect or repeating one?                |

The deterministic reference is the current admitted family, not a sampled
engine. The [stable baseline contract](../reference/STABLE_BASELINE_CONTRACT.md)
requires `assess-fixed`, `route-fixed`, and `classify-fixed` to produce the
same canonical object identities for identical task, model, policy, and input.
It requires identical bytes for the result artifact, the provenance artifact
where validated, the downstream record, and the canonical bundle. Trace or
diagnostic text is outside that identity law. A sampled profile is not part of
that contract.

| Routing case        | Deterministic reference                                                                                           | Proposed sampled profile                                                                                                |
| ------------------- | ----------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| Valid result        | `A`, `B`, or `C` under the existing bounded task                                                                  | Same vocabulary and host meaning in a separately identified profile                                                     |
| Fresh invocation    | The baseline contract requires identical canonical object identities for identical task, model, policy, and input | A fresh sample may differ. Two identical results can still represent distinct invocations.                              |
| Recorded replay     | Recover the recorded choice without submitting an effect                                                          | Same behavior, including when the provider is unavailable                                                               |
| Exact recomputation | Assess against the pinned supported execution profile                                                             | Admit only with evidence for the actual model, runtime, sampler, and randomness controls. A seed alone is insufficient. |

Preserve the frozen bundle field set in the
[stable baseline contract](../reference/STABLE_BASELINE_CONTRACT.md) and its
[version boundary](../reference/AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md).
For the current `.v1` bundles those fields, in order, are `schema`,
`source_result_ref`, `source_provenance_ref`, `action_ref`, and `record_ref`.
They are the bundle fields, not the whole object model. The object model is
still result, provenance, downstream record, and bundle. Keep per-invocation
identities and denial or unknown evidence in separately versioned experimental
records. A replay must not invent an action reference. The
[admission contract](../reference/AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)
would have to be followed before any extension of the three-member family
(`assess-fixed`, `route-fixed`, `classify-fixed`).

## Delivery sequence and exit criteria

1. **Trace and reconcile on paper.** Map each routing step to its source,
   owner, durable evidence, policy check, and crash window. Compare the
   proposed records with the surfaces that exist now, and keep their limits:

   - result and provenance objects in the
     [chain catalog](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
   - inference checkpoint verification named in the
     [2026-11-18 status note](T81_RESEARCH_STATUS_2026-11-18_V2_MCP_AIRLOCK.md)
     (`t81 ai checkpoint-verify`, `--checkpoint-canonfs`). Source inspection
     found those commands in `tools/cli/ai/ai_cli_shared.cpp`. This pass did
     not run them. That surface checks hash-linked inference step chains. It
     is not an admitted engine-substitution checkpoint.
   - draft [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md)
     accountable decision bundles. The RFC header says Draft.
   - the [F01 review plan](../research/proposals/RFC-00D7-F01-BOLQ-MIGRATION/REVIEW_PLAN.md).
     This is not an RFC file under `spec/rfcs/`.
   - the [HD01 memorandum](../research/proposals/AXION-AI-001-HD01/MEMORANDUM.md)
     and the open HD02–HD06 gates in
     [ADR-0007](../architecture/adr/ADR-0007-axion-ai-001-governance-broker.md).
     HD03 and HD04 overlap effect authority and recovery, and they are not
     selected.

   Deliver a reuse/gap table and the two profile examples. Exit when every
   proposed field has a consumer and the reader can distinguish an existing
   guarantee from a requested one.

1. **Review failure semantics before coding.** Walk through the cases below
   with a reviewer. Record which component owns admission, ordering,
   cancellation, revocation, and reconciliation. Exit when each timeline has
   one defensible disposition or a named unresolved dependency. Escalate any
   change to authority, frozen semantics, or the model-service boundary.

1. **Future bounded userspace prototype.** Only after separate authorization,
   begin with an uninterrupted deterministic control, then a local
   deterministic adapter and a bounded sampler on synthetic inputs. Feed both
   through one routing consumer. Exercise `A`/`B`/`C`, malformed output,
   unsupported replay requirements, duplicate delivery, and the recovery cases
   below. Keep a pinned revision, fixture hashes, profile identities, raw
   records, and expected/observed outcomes. Stop on baseline drift, duplicate
   effects, denied mutation, or an unsupported replay claim.

1. **Try a second task.** Apply the same request and invocation lifecycle to
   `classify-fixed`. Keep labels and rule selection task-specific. The
   [chain catalog](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md) already
   documents `t81.ai.task.classify-fixed.rule-selection-record.v1` and the
   [rule-selection example](../../examples/ai-and-inference/model-load-canonfs/run_classify_fixed_rule_selection.sh).
   Exit when both tasks can share the lifecycle without route fields leaking
   into the generic interface. Revisit the abstraction if either needs special
   dispatcher behavior.

1. **Deferred substitution and comparison.** Start with a human-directed
   switch at a checkpoint, then consider opt-in automatic substitution under a
   scoped policy. Evaluate both against the uninterrupted control. Transfer
   the task objective and portable checkpoint, recheck input access and effect
   permission, and reconcile outstanding work before retry. For shadow
   comparison, designate the live invocation in advance. Authorize input
   disclosure to each engine separately. Store both proposals; only the
   designated invocation is eligible to cause an effect. A shadow response
   never becomes an automatic fallback. Evaluate disagreement with an
   independent task criterion before making quality claims. No quality claim
   is made here.

1. **Later kernel or hardware decision.** Keep adapters, sampling, transport,
   and model caches in userspace for the first prototype. Identify which
   scheduling, resource, ordering, or isolation property needs kernel support
   before proposing an ABI or ISA change. A future accelerator would have to
   satisfy the same task and replay profile and disclose its execution
   identity. Relevant review evidence includes both tasks, negative cases,
   recovery results, and a software-only comparison. None of that evidence
   exists yet.

## Recovery cases for the paper review

These rows are proposed dispositions for review. They are not current script
behavior and not an admitted Axion recovery contract.

| Timeline                                                             | Proposed disposition                                                                                                                                                                     |
| -------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Permission revoked after `B` is returned, before the effect is armed | Preserve the decision where audit policy permits. Deny the target write.                                                                                                                 |
| Request times out or is superseded, then `B` arrives                 | Match the response to its invocation. Record it as late where permitted. The stale invocation cannot arm a write.                                                                        |
| Crash after dispatch but before confirmation                         | On restart, reconcile the operation identity against durable target evidence. If the target cannot confirm completion or safely deduplicate, retain outcome unknown and suspend retries. |
| Two replies race, or a retry delivers `B` twice                      | Resolve eligibility and ordering at the host. At most one invocation may arm this logical effect. Where the target cannot supply atomicity, expose the remaining uncertainty.            |
| Saved sampled response is `B`; a fresh sample would be `C`           | Read-only replay yields `B` with zero model calls and zero target writes. A fresh sample has a separate invocation identity.                                                             |
| Primary and shadow disagree                                          | Record both and the preselected live role. A shadow with a valid value has no effect authority.                                                                                          |

## Evidence limits

Unsupported or not established by this recheck:

- The route and classify examples and composition tests were not executed.
  The default demo route letter is unknown here.
- Identical canonical identities are the baseline contract's requirement, as
  read from the contract text, plus what the route composition test source
  checks. They are not a new measurement.
- Sampled engines, engine manifests, invocation records, shadow execution,
  substitution, and kernel or hardware adapters are not implemented.
- Replay of a sampled result, exact recomputation from a seed, and atomic
  revocation at the host write are proposed. The route script does not do
  them.
- `t81 ai checkpoint-verify` was not run. Draft RFC-00D4 was not treated as an
  adopted engine receipt. HD04 checkpoints are not admitted.
- No `spec/rfcs/RFC-00D7` document was found. The F01 protocol file is absent
  from this tree; only the review plan's recorded digest was read.
- HD02–HD06 were not given new dispositions. SCG-08 Gate 2 and H01/10A were
  not re-run.
- The handoff's 2026-11-22 live queue still names `main` `120b8dc90` and
  RFC-00D1 denial-before-materialization as the next maintenance slice. This
  recheck pin is later and includes the merge of PR #848. This plan does not
  rewrite that queue and does not re-test the export path.

**Resume here:** review the source-to-contract routing trace in stage 1
against the recheck pin. Open decisions include trusted evidence for engine
identity, the exact revocation/arming order, durable linkage to target
outcomes, and who may approve substitution. HD03 and HD04 remain unresolved
dependencies for effect authority and recovery. A future implementation
proposal should name its scope, review these dependencies, and identify its
authorization before scheduling execution.
