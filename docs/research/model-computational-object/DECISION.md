# Decision: revise the claim; retain a small conformance question

Date: 2026-10-25, America/New_York. Status: proposed research disposition, not
adoption of a runtime API, OS primitive or new canonical hypothesis.

**Recommendation: REVISE.** Treat a model object as a versioned, constrained
service reference with an externally enforced contract and evidence envelope.
Proceed only to specifying/reviewing the finite synthetic corpus if that helps
test a concrete binding or diagnostic defect. Stop the current OS-primitive and
radix-superiority interpretations. This is not closure of Track R or H01–H08.

## Evidence and classification

| Finding | Status and scope |
| --- | --- |
| Interface/enforcement determinism can be separated from model computation | Formal architectural distinction under explicit event/state/TCB assumptions; not a theorem about an implemented opaque service |
| Three-state meet gives an all-required-guards rule | Mathematical derivation; equivalent to an ordinary binary-encoded enum under relabeling |
| Three states cannot losslessly retain four independent evidence configurations | Cardinality argument; preserve support/refutation bits and lifecycle/errors separately |
| A first-class wrapper is architecturally feasible | It can be expressed as a typed descriptor and ordinary service calls; no new computability result or kernel primitive follows |
| Comparable components and composites exist | Primary prior art covers registries, inference servers, effects, capabilities, monitors, actors and evidence; global novelty absence is not proven |
| T81 contains reusable boundary surfaces | Source inspection at 01c85019, with substantial implementation-specific limits. Existing reports are not newly reproduced tests here. |
| T81 uniquely improves model invocation or security | Not established. Existing descriptor/conventional comparisons are a warning against claiming uniqueness from packaging. |
| A local finite experiment is well posed | Proposed schemas, state machine, oracle, safety checks and falsifiers; exact 60-case raw corpus still needs to be authored and pinned |
| CPU, memory-bandwidth, energy or human-review benefit | Unmeasured. No speculative benchmark or line-count argument supplies evidence. |

The closest existing abstraction is a versioned typed service/actor behind a
reference monitor, with a registry and explicit evidence. The possible remaining
contribution is a small, reproducible **cross-provider conformance contract**,
not a new model of computation. It must survive the conventional comparator and
show a specific missing invariant or demonstrated reduction in validation burden.
See [PRIOR_ART.md](PRIOR_ART.md) and the [source inventory](RESEARCH_QUESTION.md).

## Placement comparison

| Placement | Required authority | Isolation and portability | Performance implications | Novel capability and burden |
| --- | --- | --- | --- | --- |
| A: ordinary application library | No privilege for inert parsing, synthetic proposals and an in-memory sink | Portable host-language code; same-process callers are trusted. Untrusted native model code needs a separately isolated worker. | Local calls; no inference speed measured. Parsing/validation remains necessary. | Can test this entire hypothesis. Burden: small schema/state/registry/adapter contract and oracle. |
| B: user-space model runtime | Explicit resource allocation and narrow broker-held capability; ordinary OS process permissions | Separate worker/process/container can contain model implementation; GPU drivers and IPC expand the TCB. Portability depends on backend. | IPC and copies may add cost; serving/batching may change it. Measure only after a real workload is approved. | Better operational isolation/resource ownership, already an established placement. Burden: runtime integration, cancellation, lifecycle and provider pinning. |
| C: privileged system service | Central multi-user grants, quotas, identity and audit authority | Existing OS IPC/process isolation can protect a broker; compromise has larger impact. Platform-specific policy work. | Service crossings and contention are possible; no evidence they are required here. | Could centralize cross-application mediation. No demonstrated need in the single-owner experiment. Burden: hardened daemon, authentication, revocation, storage and lifecycle. |
| D: new OS primitive | Kernel authority and a long-lived compatibility contract | Kernel or trusted-process TCB grows; portability and driver/backend coupling become major obligations. | A potential crossing reduction is hypothetical; LLM inference remains outside that argument. | No presently identified capability unavailable through existing process/capability services. Burden: kernel API, policy composition, verification and maintenance. |

Prefer **A for the synthetic experiment**. Existing OS process protection is a
prerequisite when running genuinely untrusted native model code, but that does
not require a model-specific OS abstraction. Move to B only if a later approved
provider experiment needs isolation and physical resource enforcement. C/D need
an actual multi-tenant or shared-resource requirement that A/B cannot satisfy.
The existence of [capability-kernel proofs](https://sel4.systems/Verification/proofs.html)
shows an established route for certain isolation claims, not a need to put an
LLM into the kernel or inherit those proofs automatically.

## What can and cannot be promised

Within an honest fully mediated owner, one can enforce bounded syntax,
independent capability checks, final policy/replay checks, no effect on unknown
or invalid outcomes, and records binding observed data to a verifier version.
These are conditional safety properties, not guarantees that the model's advice
is correct. Attacker-controlled model output must not become its own authority.

One cannot presently promise bit-exact hosted-model replay, hidden provider
identity, semantic interchangeability, factual correctness from hashes, physical
external effects from local receipts, rollback resistance from a local mutable
store, or hard provider-side CPU/energy limits from declared logical budgets.
Deterministic verification of a record is distinct from deterministic generation
of that record or of the model response.

## Next single action and stopping condition

Review the schemas, guard projection and oracle specification, then freeze the
60 exact raw cases and expected context/event records **before** writing an arm.
The planned 729 guard assignments, 27 associativity triples and 24 event orders
are finite checks, not results already achieved. The expected enum/meet
expressive equivalence is established by relabeling; no benchmark is needed to
rediscover it. An implementation is justified only to test binding failures,
error handling or verification effort that a paper equivalence does not resolve.

Stop/revise if no missing invariant or testable ergonomic question remains,
if the conventional service already provides all useful properties, if the
oracle shares the candidate's assumptions unchecked, or if scope requires real
system control. Preserve null and negative results. Do not replace the independent
H04 continuation with this optional governance experiment.
