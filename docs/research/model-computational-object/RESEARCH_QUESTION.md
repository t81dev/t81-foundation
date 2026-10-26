# Investigation: model as a governed computational object

Date: 2026-10-25, America/New_York. Status: bounded research specification;
no new implementation, benchmark, ISA change or product architecture approval.
Source baseline: Foundation `01c85019319fd020922402ba356878e392cb2231`.
Separate branch: `research/model-computational-object`.

## Meaning and competing claims

`Model<Input, Output, Policy, Evidence>` denotes a **descriptor plus an invocation
protocol mediated by a trusted verifier and effect boundary**. The model itself
is an untrusted proposal generator. The notation neither introduces a language
type nor makes its contents authoritative. First-class means the host can pass,
identify, inspect, invoke and replace a reference through explicit operations;
it does not mean kernel residency, self-authorization or general computability
beyond an ordinary service.

Separate four identities: a mutable logical role/alias, an immutable local model
bundle, a particular execution record, and the exact output bytes. Equal output
does not prove equal execution; changing a model must change its immutable
bundle reference, while its role can remain the same. A hosted descriptor can
identify an endpoint and reported revision, but cannot independently establish
the provider's hidden weights, kernel or version. See [FORMAL_MODEL.md](FORMAL_MODEL.md).

| Candidate hypothesis (no canonical number assigned) | Competing explanation / falsifier |
| --- | --- |
| `candidate:mco-binding` — binding contract, model revision, authority and observations reduces unsafe substitution or replay mistakes | A conventional typed service plus registry and reference monitor enforces the same predicates; the proposed descriptor adds no demonstrated capability |
| `candidate:mco-three-way` — a three-element composition algebra improves reviewability of incomplete decisions | A binary-encoded enum implements the identical algebra; richer evidence/error state is still required |
| `candidate:mco-os-necessity` — a new OS primitive is needed | A library using existing process isolation and a narrowly held capability supplies the same boundary |

These are refinements adjacent to H07/H08 and a semantic question adjacent to
H05. They neither redefine those hypotheses nor add H09. A semantic trivalent
result is independent of native ternary representation, H01/H04 packing or H06
inference kernels. The [charter](../continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)
and [canonical H01–H08 register](../continuity/HYPOTHESIS_REGISTER_v1.json)
remain authoritative and unchanged. Track R stays open; Track F supplies testable
boundaries; Track M records which claims are derived, inspected or unverified.

## Source-grounded starting facts

The table records **source inspection**, not freshly executed tests. Existing
reports' test counts are historical claims here, even when their data are in Git.
No prior AI report is treated as an independent scientific replication.

| Surface and pinned source | Established by this inspection / limit |
| --- | --- |
| [Master charter](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md) and [register v1](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/docs/research/continuity/HYPOTHESIS_REGISTER_v1.json) | Ternary investigation is primary; governed execution is a separate supporting hypothesis. Register v1 is a dated snapshot, not proof of current implementation absence. |
| [Hanoi archive, fe003968](https://github.com/t81dev/t81-foundation/blob/fe00396831c8bb249fde06a04186fa02f6018caf/legacy/hanoivm/src/hanoivm_core/t81recursion.cweb) and [pinned lineage](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/docs/research/continuity/LINEAGE.md) | Recursive dispatch and depth bounds exist as source intent. The lineage pins the Hanoi import at 59205d76 and documents scheduler/promotion inconsistencies and arithmetic counterexamples; those probes were not rerun here. No integrated working Hanoi model-object system follows. |
| [language build targets](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/src/lang/CMakeLists.txt), [semantic analyzer](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/lang/frontend/semantic_analyzer.cpp), [feature registry](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/spec/t81lang_features.md) | Concrete frontend and tier/effect surfaces exist; they are not a proof of arbitrary LLM output safety or a new approved Model type. |
| [VM step path](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/vm/vm.cpp) (`step`, policy check before dispatch), [VM interface](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/include/t81/vm/vm.hpp) | A TISC interpreter with explicit bounded execution/policy paths exists. Do not extend its verified guarantees to arbitrary model serving or every host side effect. |
| [model manifest](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/docs/contracts/MODEL_MANIFEST_V1_CONTRACT.md) | Persisted intake/tensor inventory contract, not evidence of model inference equivalence or authorization to execute. |
| [persistent CanonFS driver](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/fs/persistent_driver.cpp) (`write_object`, `read_object_bytes`, `has_capability`) | Payload hashing and hook/permission checks precede covered operations. With no capability files, `has_capability` permits legacy operation; an object ref is not itself a capability token. |
| [block driver](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/fs/block_backed_driver.cpp) | Read/write consult the hook, but not the stored capability map; publish/revoke/repair have different paths. No uniform security guarantee follows from a shared driver type. |
| [Axion final authorization](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/kernel/axion/policy_engine.cpp) and [verdict enum](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/include/t81/axion/verdict.hpp) | Final authorization requires an active policy. Verdicts are Allow, Deny, Defer, Warn, Quarantine — already more than three states. |
| [ControlledAIBackend](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/vm/ai_backend/controlled_ai_backend.cpp) (`authorize`, `inference_with_external_ai`) | Missing active policy/non-Allow blocks dispatch; requiring consent blocks because a verified consent contract is unavailable. The external path generates a local response string, not an opaque hosted-model call. |
| [descriptor runner](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/examples/research/validated-descriptors/runner.cpp) and [descriptor report](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/examples/research/validated-descriptors/REPORT.md) | Two fixture-bounded adapters expose run/inspect; the report records equivalent rejection/discovery predicates from a conventional control. Tensor inspection is not inference, and these fixture results do not prove all abstractions useless. |
| [witness](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/experiments/durable_witness/witness.py) and [verifier](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/experiments/durable_witness/verify.py) | Transaction/HMAC receipt mechanisms exist. Verification shares the witness signing helper and test key defaults; inspect this TCB rather than assuming laboratory independence or public authenticity. A live-store check and rollback-resistant anchor are different obligations. |
| [determinism registry](https://github.com/t81dev/t81-foundation/blob/01c85019319fd020922402ba356878e392cb2231/docs/governance/DETERMINISM_SURFACE_REGISTRY.md) | Determinism claims are surface-specific; experimental network/distributed behavior does not inherit the interpreter's guarantees. |

Existing boundary tests include `tests/cpp/controlled_ai_authorization_test.cpp`,
`canonfs_policy_boundary_test.cpp` and `vm_predispatch_policy_deny_logging_test.cpp`.
Their source was inspected; their suites were not executed for this documentation task.

## H04 continuity boundary

[PR #572](https://github.com/t81dev/t81-foundation/pull/572) was checked open,
unmerged, at `a99fbe5ef8a89671e68b3963973f076855b101ec` during this investigation.
Its [current conclusion](https://github.com/t81dev/t81-foundation/blob/a99fbe5ef8a89671e68b3963973f076855b101ec/experiments/h04_r02_adaptive_representation/RESULTS.md)
is conditional representation-cost evidence with actual codec byte counts, not
CPU/device superiority or universal ternary futility. Its
[precision correction](https://github.com/t81dev/t81-foundation/blob/a99fbe5ef8a89671e68b3963973f076855b101ec/experiments/h04_r02_adaptive_representation/PRECISION_CORRECTION.md)
illustrates why agreement on rounded totals is not independent mathematical
validation. These are read-only references, not changes or new reproduction.
The fetched main still contains the older merged #569 packet; do not mistake
that older conclusion for the unmerged corrected research state.

## Result sought

The useful distinction is between nondeterministic proposal generation and
conditionally deterministic enforcement/verification. Novelty requires a new
observable invariant or reproducible reduction in integration/verification
burden beyond established composition; a common wrapper name is insufficient.
Read [PRIOR_ART.md](PRIOR_ART.md), then [FORMAL_MODEL.md](FORMAL_MODEL.md),
[THREAT_MODEL.md](THREAT_MODEL.md), [EXPERIMENT_PROPOSAL.md](EXPERIMENT_PROPOSAL.md)
and [DECISION.md](DECISION.md). [HANDOFF.md](HANDOFF.md) records provenance and limits.
