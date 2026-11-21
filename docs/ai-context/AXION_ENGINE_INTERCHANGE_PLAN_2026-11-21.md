# Interchangeable AI engines for Axion

**November 21, 2026 · Non-normative design backlog · Implementation deferred.** The maintainer requested a plan and an AI-context update. The immediate handoff is a paper review of the routing example. Later stages describe candidate work for separate authorization. HD01–HD06 are open and RFC-00D7-F01 M1–M6 remain at STOP. SCG-08 Gate 2 and H01/10A retain their existing dispositions.

## Starting point and first milestone

Baseline revision `c314f6f6fd9ed1a90414132344a5ed5a5dab3f6f`. In the [route example](../../examples/ai-and-inference/model-load-canonfs/run_route_fixed_path_selection.sh), the CLI runs the bounded task under policy. The script fetches its result from CanonFS, reads the route, maps B to `routes/b.target`, writes the action artifact, then stores the downstream record and canonical bundle. The [composition test](../../tests/cpp/ai_task_route_fixed_composition_test.cpp) compares bundle references across repeated runs and different CanonFS roots. The script performs the host write. Atomic revocation and crash recovery at that write are design questions.

First milestone for a future userspace prototype: the same host mapping consumes a valid route from either a deterministic engine or a sampler. A reader can recover which proposal was used, what permission applied and what happened to the effect, including an unknown outcome. This milestone concerns interchange at task boundaries. Engine-private token history and KV caches stay behind the adapter.

## Proposed contract for one bounded decision

| Surface | Proposed minimum | Review question |
|---|---|---|
| Request | Versioned task semantics and input references, output vocabulary, request identity, budget and required replay guarantee. Distinguish a retry of one invocation from a new sample on the same input. | Can the host reject an incompatible profile before disclosing input or dispatching work? |
| Engine manifest | Supported tasks, model/adapter and execution profile identity, limits, randomness controls and evidenced replay guarantees. Mark unknown provider details explicitly. | Can a local, remote or hardware implementation qualify using the same contract? |
| Result and invocation record | Typed value or failure linked to the request and actual engine invocation. Capture the sampling policy and seed or randomness information when available. Record which identity claims could be verified. | Can the consumer distinguish malformed output, timeout, cancellation, late output and success? |
| Host decision and effect | Deterministic interpretation of the typed value, current authorization for the concrete target, a durable operation identity, and an observed outcome. | Can an old result be inspected without granting a fresh effect or repeating one? |

| Routing case | Deterministic reference | Proposed sampled profile |
|---|---|---|
| Valid result | A, B or C under the existing bounded task | Same vocabulary and host meaning in a separately identified profile |
| Fresh invocation | The admitted baseline promises identical canonical objects for identical task/model/policy/input | A fresh sample may differ. Two identical results can still represent distinct invocations. |
| Recorded replay | Recover the recorded choice without submitting an effect | Same behavior, including when the provider is unavailable |
| Exact recomputation | Assess against the pinned supported execution profile | Admit only with evidence for the actual model, runtime, sampler and randomness controls. A seed alone is insufficient. |

Preserve the [frozen object contract](../reference/STABLE_BASELINE_CONTRACT.md) and its [version boundary](../reference/AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md). The five canonical fields are `schema`, `source_result_ref`, `source_provenance_ref`, `action_ref` and `record_ref`. Keep per-invocation identities and denial/unknown evidence in separately versioned experimental records. A replay must never invent an action reference. Explicit admission and compatibility review would precede any extension of the three-member family.

## Delivery sequence and exit criteria

1. **Trace and reconcile on paper.** Map each routing step to its source, owner, durable evidence, policy check and crash window. Compare the proposed records with existing result/provenance objects, checkpoint and receipt surfaces, draft RFC-00D7 and [HD01](../research/proposals/AXION-AI-001-HD01/MEMORANDUM.md). Deliver a reuse/gap table and the two profile examples. Exit when every proposed field has a consumer and the reader can distinguish existing guarantees from requested ones.
2. **Review failure semantics before coding.** Walk through the cases below with a reviewer. Record which component owns admission, ordering, cancellation, revocation, and reconciliation. Exit when each timeline has one defensible disposition or a named unresolved dependency. Escalate any change to authority, frozen semantics or the model-service boundary.
3. **Future bounded userspace prototype.** Once separately authorized, begin with an uninterrupted deterministic control, then a local deterministic adapter and a bounded sampler on synthetic inputs. Feed both through one routing consumer. Exercise A/B/C, malformed output, unsupported replay requirements, duplicate delivery and the recovery cases below. Keep a pinned revision, fixture hashes, profile identities, raw records and expected/observed outcomes. Stop on baseline drift, duplicate effects, denied mutation or an unsupported replay claim.
4. **Try a second task.** Apply the same request and invocation lifecycle to `classify-fixed`; keep labels and rule selection task-specific. Compare with the [chain catalog](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md). Exit when both tasks can share the lifecycle without route fields leaking into the generic interface. Revisit the abstraction if either needs special dispatcher behavior.
5. **Deferred substitution and comparison.** Start with a human-directed switch at a checkpoint, then consider opt-in automatic substitution under a scoped policy. Evaluate both against the uninterrupted control. Transfer the task objective and portable checkpoint, recheck input access and effect permission, and reconcile outstanding work before retry. For shadow comparison, designate the live invocation in advance. Authorize input disclosure to each engine separately. Store both proposals; only the designated invocation is eligible to cause an effect. A shadow response never becomes an automatic fallback. Evaluate disagreement with an independent task criterion before making quality claims.
6. **Later kernel or hardware decision.** Keep adapters, sampling, transport, and model caches in userspace for the first prototype. Identify which scheduling, resource, ordering or isolation property demonstrably needs kernel support before proposing an ABI or ISA change. A future accelerator must satisfy the same task and replay profile and disclose its execution identity. Relevant review evidence includes both tasks, negative cases, recovery results, and a software-only comparison.

## Recovery cases for the paper review

| Timeline | Expected disposition for the proposed design |
|---|---|
| Permission revoked after B is returned, before the effect is armed | Preserve the decision where audit policy permits. Deny the target write. |
| Request times out or is superseded, then B arrives | Match the response to its invocation. Record it as late where permitted. The stale invocation cannot arm a write. |
| Crash after dispatch but before confirmation | On restart, reconcile the operation identity against durable target evidence. If the target cannot confirm completion or safely deduplicate, retain outcome unknown and suspend retries. |
| Two replies race, or a retry delivers B twice | Resolve eligibility and ordering at the host. At most one invocation may arm this logical effect. Where the target cannot supply atomicity, expose the remaining uncertainty. |
| Saved sampled response is B; a fresh sample would be C | Read-only replay yields B with zero model calls and zero target writes. A fresh sample has a separate invocation identity. |
| Primary and shadow disagree | Record both and the preselected live role. A shadow with a valid value has no effect authority. |

**Resume here:** review the source-to-contract routing trace in stage 1 against current repository revisions. Open decisions include trusted evidence for engine identity, the exact revocation/arming order, durable linkage to target outcomes, and who may approve substitution. A future implementation proposal should name its scope, review these dependencies, and identify its authorization before scheduling execution.
