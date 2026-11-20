# AXION-AI-001 Phase 2 — HD01 Model-Binding Authority Research

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [AXION-AI-001 Phase 2 — HD01 Model-Binding Authority Research](#axion-ai-001-phase-2-—-hd01-model-binding-authority-research)
  - [Purpose, authorization and evidence boundary](#purpose-authorization-and-evidence-boundary)
  - [1. Baseline observations and research question](#1-baseline-observations-and-research-question)
  - [2. Separate authority objects and role boundaries](#2-separate-authority-objects-and-role-boundaries)
  - [3. Three competing authority models — no winner selected](#3-three-competing-authority-models-—-no-winner-selected)
    - [Limited external precedents](#limited-external-precedents)
  - [4. Exact identity and verification profile candidates](#4-exact-identity-and-verification-profile-candidates)
  - [5. Custodian, verifier and worker responsibilities](#5-custodian-verifier-and-worker-responsibilities)
  - [6. Revocation, replay protection, substitution and recovery](#6-revocation-replay-protection-substitution-and-recovery)
  - [7. Adversarial counterexamples and proposed conformance evidence](#7-adversarial-counterexamples-and-proposed-conformance-evidence)
  - [8. Dependencies and unresolved human decisions](#8-dependencies-and-unresolved-human-decisions)
  - [Evidence and preparation references](#evidence-and-preparation-references)

<!-- T81-TOC:END -->


Status: NON-NORMATIVE research memorandum — authority model NOT SELECTED / NOT ADOPTED
Date: 2026-11-17
Version: 0.1.0
Maturity: Experimental
Baseline: `cb9216f42f63e95e58709a71e08d38139c71e48b`

## Purpose, authorization and evidence boundary

Compare authority arrangements for the external model-binding envelope proposed
by [ADR-0007][ADR]. Human authorization permits this documentation research and a
draft PR for independent review only. Design B is preferred for research; its
merge in PR #826 did not adopt the architecture or choose HD01 authority.
**No authority model is selected by this memorandum. HD01 remains OPEN.**

Codex authored this memorandum and the preceding ADR proposal. This is preparation
for independent review, not a claim of independent Gemini validation. The ADR and
its [validation record][ADR-VALIDATION] were read before authoring. Their historical
passing documentation checks are not new execution evidence. No tests, signature
verification cases, model inference, benchmarks, host work or conformance scenarios
were executed for this memorandum. No active schema, gate or lock changes.

This is Track F / canonical H07 supporting research with H08 provenance limits.
It preserves H01–H08, historical Hanoi/CWEB lineage, negative findings and unrelated
research/release holds. Phase 2 is the user's research-work label, not permission
to advance an operative architecture, research gate or experiment.

Classification: IMPLEMENTED means source exists; STATICALLY VERIFIED means the
specific source/text was inspected; DYNAMICALLY VERIFIED means execution supports
the claim (none here); PROPOSED means a future contract requiring review;
UNSUPPORTED means the available evidence does not establish a guarantee.
**All authority models, record fields, procedures and conformance criteria below
are PROPOSED unless explicitly classified otherwise.**

F01 remains **BOLQ NOT ADOPTED; residual risks NOT ACCEPTED; M1–M6 STOP;
`linux_relock_required=true`**. Its operative protocol SHA-256 remains
`d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
No F01 resource, qualification, receipt or execution semantics change.

## 1. Baseline observations and research question

| Classification | Evidence and limit |
|---|---|
| STATICALLY VERIFIED | ADR sections 2–4 propose isolated admission, a separate binding envelope and complete request identity. HD01 explicitly leaves authority, canonicalization and signature/provenance policy unresolved. [ADR] |
| STATICALLY VERIFIED | RFC-00D3 `t81.model-manifest.v1` closes its root and provenance keys. New approval/delegation fields cannot be injected into that unchanged same object. Its optional source digest is not complete approval evidence. [D3] |
| IMPLEMENTED / STATICALLY VERIFIED | The CI provenance source includes canonical JSON and HMAC-SHA256 signing/verification helpers; its keyring declares active/next key metadata. This is not an adopted HD01 approval service or evidence of independently controlled approvers. No secret key material or environment values were read and the gate was not run. [PROVENANCE], [KEYRING] |
| IMPLEMENTED / STATICALLY VERIFIED | Experimental Axion OS capability records include kind, scope and delegation provenance. This does not define model-publisher trust, signed binding delegation or permission to map those records into HD01 grants. [KERNEL] |
| IMPLEMENTED / STATICALLY VERIFIED | Hosted Axion defaults allow; the native llama adapter loads before hashing; CanonFS pre-checks do not themselves establish revocation-atomic transactions. These ADR constraints remain, rather than being repaired here. [AXION], [LLAMA], [CANONFS] |
| UNSUPPORTED | Repository integration, content hashes, signatures, provider names or static source existence alone establish model safety, human approval, consent, execution identity or universal distributed revocation. |

Research question: under which explicit human-controlled trust roots, delegation
limits and freshness assumptions could a broker accept a binding while rejecting
substitution, stale authority and privilege escalation? Compare equivalent
artifact/admission assurance; do not infer that a more elaborate authority graph
improves security or cost without conformance and measurement evidence.

## 2. Separate authority objects and role boundaries

Keep these objects distinct; none substitutes for the next:

1. **Artifact binding:** identifies exact manifest and complete artifact closure.
2. **Binding approval:** an authorized principal approves a particular binding
   for a particular audience, purpose, policy scope and conditions.
3. **Admission decision:** the broker checks current approval/freshness, immutable
   snapshots, actual adapter identity, context fit and resource reservation.
4. **Invocation authorization:** a session-specific policy decision permits an
   exact request; binding approval is not an invocation capability.
5. **Effect authorization/commit:** ADR sections 5–6 govern each local effect or
   external handoff. Model approval cannot mint tool or kernel authority.

| Role | Possible authority, only if explicitly granted | Excluded authority |
|---|---|---|
| Human governance root | Select authority model, initial trust anchors, roles, thresholds, rotation/recovery and policy scope through attributable decisions | No implicit adoption from this document, a Git merge or an `approved` field |
| Binding approver | Approve/reject exact binding bytes for its bounded purpose/audience | Change policy, approve unspecified assets, grant execution, bypass resource/context checks |
| Delegator | Attenuate a granted approval role within allowed namespace, audience, purpose, lifetime and depth | Broaden scope, extend expiry, change thresholds, create an implicit wildcard or redelegate without permission |
| Revocation authority | Revoke identified grants/bindings/keys within its explicit scope | Revoke unrelated principals or rewrite historical evidence merely by publishing a negative record |
| Policy authority | Establish policy content, namespace and epoch through existing applicable governance | Treat model-publisher signatures as policy changes |
| Custodian / verifier | Supply authenticated snapshot/identity/schema observations | Approve its own observations unless separately and explicitly assigned an approval role |
| Broker | Enforce the adopted profile and current session authority; record admission | Invent a trust root, silently choose a model, interpret missing configuration as Allow |
| Untrusted worker/provider | Supply output and explicitly labeled identity claims | Issue trust anchors, self-approve manifests, dictate policy epoch or supply authoritative audit state |

An authenticated identity is not permission. A valid signature is not proof that
the signer had this role at the decision point. A human decision, an organization,
a key, a service and a process are different identities. Thresholds over keys do
not prove independent human or organizational control.

Delegation records need exact parent/child identities, signed scope, audience,
purpose, validity/freshness conditions, depth/redelegation constraints and a unique
grant/version identity. A verifier evaluates the entire bounded chain against the
locally trusted root and current revocations. Unknown scope intersection, cycles,
excess depth, ambiguous competing grants or unsupported semantics fail closed.
All graph traversal and metadata sizes require separately approved bounds.

## 3. Three competing authority models — no winner selected

| Candidate | Approval and delegation arrangement | Trust, availability and operational trade-offs | Principal falsifier |
|---|---|---|---|
| A — Single accountable release authority | One human-designated authority approves each exact binding; no downstream delegation. Root/recovery roles remain explicit. | Short trust path and simple attribution; concentrated compromise/availability risk, review bottleneck and key-custody burden. Offline approvals still need a freshness policy. | An untrusted or out-of-scope signature admits a binding, or loss of the one authority silently enables a fallback signer. |
| B — Scoped delegated authorities | Human-controlled root authorizes bounded publisher/reviewer roles; each approval proves a complete attenuating chain. | Team autonomy and compartmentalized scopes; more complex scope intersection, root/role rotation, cascading revocation and rollback protection. Root compromise remains critical. | A child broadens parent scope, revoked ancestry remains live or one namespace's grant authorizes another. |
| C — Threshold approval by distinct principals | A configured quorum of separately authorized principals approves the same payload; delegation is disabled or requires separately specified quorum-preserving rules. | Reduces dependence on one signing key under actual independent custody assumptions; increases coordination and outage risk. Multiple keys controlled by one actor are not independent review. | Duplicate/aliased keys count twice, signatures cover different payloads or a reduced threshold is accepted without root authorization. |

Evaluate all three against the same asset closure, immutable-use, replay,
revocation, session-admission and evidence requirements. A model does not earn
an availability exception that silently removes another model's assurance.
Online approval/freshness services and offline signed snapshots are deployment
variants for each candidate, not permission to skip freshness or change roots.

Review criteria to weight explicitly: acceptable compromise assumptions; required
organizational separation; policy/namespace granularity; disconnected operation;
revocation visibility; recovery authority; audit burden; implementation complexity;
and measured operational cost if later authorized. No weights, threshold, root
key, deployment, signature algorithm or authority model are adopted here.

### Limited external precedents

STATICALLY VERIFIED documentary precedent: TUF specification 1.0.37 describes
separate metadata roles, delegated targets, thresholds, versioning and freshness
checks for software updates. Those patterns inform the comparison; they do not
constitute T81 model approval, safe inference or immediate distributed revocation.
[TUF]

STATICALLY VERIFIED documentary precedent: DSSE protocol 1.0.2 authenticates
payload type together with serialized payload bytes. It does not choose trusted
signers or an application authorization policy. This supports separating signed
identity from authority; DSSE is not automatically selected for HD01. [DSSE]

These are externally maintained documents consulted on 2026-11-17. Their linked
latest/master pages are mutable; their observed versions and limits are retained
in PREPARATION.md. Normative adoption would need immutable external revision pins.

## 4. Exact identity and verification profile candidates

Separate an immutable **binding identity**, an **approval identity**, and a live
**admission identity**. Signatures on the same binding may change without changing
the artifact bytes; new approval/authority versions still change admission state.

The external envelope remains separate from unchanged RFC-00D3. Record-field
requirements below are research proposals, not JSON Schema edits or an ABI change.

| Identity | Exact evidence to bind and compare |
|---|---|
| Model | Raw legacy manifest digest and pinned v1 schema identity; complete role-labeled asset list with algorithm-qualified digest and byte length for each weight shard, configuration, quantization/adapter weight and auxiliary asset. Root identity covers ordering and membership, not just tensor summaries or path names. |
| Tokenizer | Vocabulary, merges/model, normalizer/pre-tokenizer, special/added tokens, configuration and tokenizer implementation identity. Equal name or vocabulary count is insufficient. |
| Inference adapter | Exact executable/component digest, versioned ABI, admitted dependency/plugin closure, executable configuration, backend/provider profile and independently admissible loaded-identity receipt. Source commit or package version alone is insufficient. Driver/runtime/hardware scope comes from HD05, not a fabricated complete closure. |
| Template and rendering | Exact chat/system templates, renderer/version, role handling, tool-schema bundle/wrappers, attachment/multimodal processor identities and effective configuration. Generated prompt/rendering evidence binds the exact task revision. |
| Policy epoch | Authoritative policy namespace, policy digest, monotone epoch/sequence and authority-snapshot identity, linked to boot generation/broker incarnation and session. A bare numeric epoch or worker-supplied policy hash is insufficient. |
| Approval | Exact binding ID, issuer/principal, role/delegation path, purpose, audience/tenant, policy-epoch scope, approval/version ID, validity/freshness conditions, authority model/profile ID and signed-payload encoding ID. |
| Live admission | Approval and snapshot identities, authenticated custodian/verifier receipts, actual worker/adapter generation, complete ADR request identity, context-admission reference and unchanged cumulative budget reservation. |

Candidate verification order, common to all authority models:

1. Load a human-authorized trust/profile configuration and current authority
   snapshot; missing or unsupported configuration means STOP, not a default model.
2. Parse bounded envelope/approval metadata in isolated trusted verification
   components. Reject duplicates, unknown critical fields, malformed identifiers,
   unsupported algorithms/encodings and ambiguous role/path membership.
3. Verify signatures over exact payload bytes under the configured domain,
   payload type, version and encoding. Do not reserialize arbitrary JSON and
   assume it is the signed message. Bind intended audience, purpose and policy
   scope inside the authenticated payload, not an unsigned header.
4. Resolve signer identity and its complete approval authority at the relevant
   authoritative state; verify delegation attenuation/quorum and revocation.
5. Compare every admitted asset digest/length/role with immutable custodian
   snapshots, including tokenizer/template/adapter closure. No post-check path reopen.
6. Obtain an authenticated actual-use receipt binding the snapshot handles,
   verifier version, nonce/challenge, broker incarnation and worker generation.
   A worker echo alone does not attest which bytes executed.
7. Match the authoritative current policy epoch and every live session constraint;
   finish context and resource admission before activation. Record exact inputs,
   authority snapshot, decisions and limitations in session-local audit state.

Canonicalization alternatives remain open: sign an explicitly versioned canonical
representation with independently agreed golden encodings, or sign exact immutable
payload bytes with strict interpretation rules and a distinct content identity.
Neither permits ambiguous JSON interpretation. Exact wire framing, Unicode,
number representation, digest/signature suites and compatibility vectors need a
future reviewed profile. Without that profile, verification cannot become operative.

For this research's strict epoch-bound profile, a policy change requires explicit
new approval/reauthorization; no silently compatible policy family is inferred.
Whether a separately authorized family constraint is ever acceptable remains a
human decision, not an automatic authority-model choice.

Remote model/tokenizer/adapter identity cannot be established by local envelope
checks, TLS endpoint identity or provider model names alone. Claimed/unknown
fields remain explicit and depend on HD02. Unknown local closure/actual-use proof
blocks exact-identity admission; a hash does not cure malicious-host deception.

## 5. Custodian, verifier and worker responsibilities

The custodian acquires bounded private snapshots, hashes raw bytes, protects
immutability for the entire use lifetime and authenticates observations. Read-only
file descriptors, symlink resolution or before/after hashes alone do not establish
protection against another writer. Include memory-mapped files, lazily loaded
shards, libraries and generated caches in the reviewed use profile. An unsupported
immutability mechanism blocks admission; it does not reduce the assurance claim.

The separate verifier validates legacy schema, envelope interpretation, signer
authority and receipt bindings under pinned profiles. It does not gain model
approval merely by producing a verifier receipt. Combining roles is an explicit
candidate-model decision and changes the compromise assumptions.

All untrusted model parsing, tokenization, template processing and native inference
remain outside the broker address space. The untrusted worker receives only
admitted snapshots and bounded requests, not trust-root mutation, signing material,
ambient effect credentials or authoritative epoch/audit storage. Worker outputs,
counts and identity reports are claims requiring the relevant admission evidence.
The broker owns current policy, grant validity, ordering and session audit.

A delegated publisher cannot force a custodian to fetch arbitrary locations or
execute a tokenizer plugin. Fetching, parsing and measurement each need their own
bounded authority; approval of the resulting artifact is a different operation.
No existing CI provenance secret or kernel capability is repurposed for HD01.

## 6. Revocation, replay protection, substitution and recovery

Distinguish revocation of a binding, a particular approval, an issuing/delegating
key, a root/profile version and a live session capability. Each revocation names
its subject, scope, issuer authority, sequence and effective commitment. Reject
unauthorized revocation as well as unauthorized approval. Descendant authority
becomes unusable when required ancestry is revoked; unrelated grants remain valid
unless a separately authoritative policy/root transition says otherwise.

At each admission/invocation/effect boundary, use the required current authority
state and ADR sequencer. Cached positive approval is not an irrevocable grant.
Revocation ordered before activation/arming blocks that action; arming before
revocation retains ADR in-flight uncertainty and possible remote completion.
Logical fencing is separate from termination and leaves unproven releases charged.

Replay protection needs all of: exact audience/purpose, non-reused boot/broker/
worker generations, request ID and attempt, signature-domain/version separation,
trusted monotone authority checkpoints and a duplicate-consumption ledger. No
single nonce substitutes for rollback-resistant state. Long-lived binding approval
may be reusable only under explicit scope; invocation/effect grants are separately
session-bound and single-use where the ADR requires it.

Freshness alternatives to decide prospectively: authenticated online status at
defined boundaries; bounded-age signed snapshots with a trusted clock; or explicit
disconnected profiles using a trustworthy monotone checkpoint and documented
staleness limits. Unsupported time/checkpoint/freshness evidence means STOP.
No claim of immediate global revocation during network partition is supported.
Availability and permissible stale exposure require human decisions, not a cache
fallback. An online service or replicated store needs explicit authoritative
ordering; conflicting checkpoints do not become valid through majority counting
unless a separately reviewed protocol establishes that property.

Recovery fences admission before restoring root/profile versions, revocations,
approval-consumption state, budget reservations and ADR effect records. An older
validly signed snapshot is not necessarily current. A lost/torn checkpoint or
unknown consumption state cannot reset replay protection. Retain known evidence,
classify unidentified exposure as incomplete and STOP new authority use until
independently admissible reconciliation. Recovery/root-key replacement requires
the human-approved recovery authority and migration evidence; never trust a new
root because it signs itself or appears in a repository update.

Historical governance replay uses the recorded authority state and trust profile
as of each decision, without models, effects, new grants or live status queries.
It must distinguish past authorization from present admissibility. Later key
compromise or conflicting provenance is appended as uncertainty/correction,
not erased or treated as evidence that all past signatures were trustworthy.

ADR torn-marker rules are unchanged: durable intent alone does not prove handoff
absence; ambiguous possible dispatch recovers UNCERTAIN. Never classify such an
effect NOT_DISPATCHED/FAILED/DENIED solely from absent metadata. Confirmed handoff
does not confirm remote outcome. No exactly-once claim or automatic uncertain retry.

## 7. Adversarial counterexamples and proposed conformance evidence

**All cases are PROPOSED / NOT EXECUTED.** Independent checking means separately
implemented reference interpretation and explicit authorship/common assumptions,
not two same-author helpers or a model-authored PASS summary. HD06 must authorize
a future conformance proposal and any later implementation/execution separately.

| Case / ADR mapping | Counterexample | Falsifier and proposed independently checkable evidence |
|---|---|---|
| B01 / I01 | Missing root or unknown model silently picks candidate A. | Any activation falsifies. Retain exact empty/unknown configuration and a decision trace showing STOP/no fallback. |
| B02 / I04 | Inject signer fields into the v1 manifest or treat envelope approval as schema validity. | Any accepted v1 extension or conflated conclusion falsifies. Preserve original schema bytes, separate object identities and independent interpretations. |
| B03 / I03 | Same manifest name, replaced weight shard or quantization configuration. | Consuming a different digest/role/length falsifies. Retain admitted and actual-use asset closure with immutable-snapshot evidence. |
| B04 / I03, I09 | Keep weights; replace tokenizer, tool template or attachment processor. | Reusing old identity/context admission falsifies. Retain every changed component, destination render/count record and expected rejection or new approval/admission. |
| B05 / I02, I05 | Correct source commit but substituted adapter library/plugin, or forged worker use receipt. | Activation under unproved loaded identity falsifies exact-identity mode. Preserve dependency inventory, authenticated peer/challenge/generation and receipt trust assumptions. |
| B06 / I05 | Replay another tenant's approval or alter unsigned purpose/policy metadata. | Cross-audience/purpose admission falsifies. Preserve exact signed bytes and field-by-field expected interpretation. |
| B07 / I05 | Delegate broadens namespace, expiry or depth, cycles to itself, or loses a required parent. | Accepted excess authority falsifies. Independent graph oracle must reject the exact out-of-scope chain; retain bounded full graph and authority snapshot. |
| B08 / I05 | Threshold counts aliases of one principal or signatures over different bindings. | Satisfying quorum without the configured distinct principals on identical payload falsifies. Retain root principal mapping, signature payload identities and threshold decision. |
| B09 / I05, I06 | Reuse a valid approval after issuer/ancestor revocation, or revoke an unrelated grant. | Wrong targeted/current validity falsifies. Preserve both orderings, targeted scope and unrelated-grant control history. |
| B10 / I05, I13 | Restore an older valid root snapshot or consume a grant again after restart. | Activation with rolled-back authority or duplicated consumption falsifies. Retain checkpoint/recovery provenance; missing freshness must STOP rather than reset. |
| B11 / I05, I09 | Same epoch number in another policy namespace, or changed policy/template with stale context record. | Accepted digest/epoch/context mismatch falsifies. Preserve complete request tuple and independent expected field comparisons. |
| B12 / I06–I08 | Revoke after HANDOFF_ARMED; cancel acknowledgement is treated as no remote effect. | False absence/prevention conclusion falsifies. Preserve authoritative order and separate dispatch progress/effect outcome; unresolved outcome is UNCERTAIN. |
| B13 / I10, I12 | Old worker survives revocation; replacement resets remaining budget. | New old-generation commitment or uncharged overlap falsifies. Preserve fence, reservations and actual release evidence; absence keeps accounting restrictive. |
| B14 / I11 | Historical replay fetches current status, reruns inference or issues a grant. | Any external model/effect/authority use or hidden input falsifies. Proposed replay has those authorities disabled and consumes only retained records. |
| B15 / I09 | Destination context shrinks; overhead pushes input one token over; unauthorized compaction drops uncertain effects. | Any activation without fit or authorized retained transformation falsifies. Preserve exact-fit/one-over render/count evidence, ContextBudgetExceeded, parent/child provenance and unchanged policy/effect/budget references. |

The future evidence packet would retain: exact proposal/source/profile identities;
raw signed bytes and public trust metadata (never secret keys); role/delegation/
revocation and checkpoint histories; custodian/verifier/actual-use evidence;
canonical task/context references; independent expected decisions and raw observed
events; resource accounting; uncertainty and every negative/incomplete result.
None exists as an executed HD01 result here. Case domains, enumerations, limits,
independent oracle and platform scope need separate preregistration and approval.

For each candidate, the future proposal also needs positive controls with a
correctly scoped approval and complete current authority evidence, alongside
each rejected substitution. Approval-verification success still does not activate
a worker or authorize effects: all separate admission and human gates apply.
Encoding/signature vectors need independent expected payload identities, including
ambiguous or mismatched encodings, before a verification profile can be accepted.

## 8. Dependencies and unresolved human decisions

| ADR gate | Dependency / boundary retained |
|---|---|
| HD02 | Decide admissible remote model/tokenizer claims, confirmation contracts, attestation trust and conservative context bounds. HD01 signatures cannot authenticate unseen remote weights by themselves. |
| HD03 | Decide which effect classes tolerate dispatch-time authorization and possible post-revocation completion. Binding revocation does not erase handed-off effects. |
| HD04 | Establish durable authority/consumption checkpoints and local transaction/recovery/no-attempt evidence. Metadata signature validity alone is not rollback resistance or atomicity. |
| HD05 | Establish actual host trust/isolation, snapshot and loaded-identity evidence, clocks, native closure, persistence, numerical budgets and compaction policy. No host is qualified here. |
| HD06 | Decide whether to authorize independent conformance preparation and, separately, implementation/execution. This memorandum authorizes none. |

Human decisions still required within HD01:

1. Select candidate A, B, C or a separately reviewed alternative and its explicit
   compromise/availability assumptions; establish accountable governance principals.
2. Define root installation, key custody, approval/delegation/revocation roles,
   actual independence requirements, emergency recovery and root rotation.
3. Choose canonicalization/signed-byte framing, digest/signature suites, audience,
   purpose and policy-epoch scope, strict parser rules and profile versioning.
4. Choose freshness, trusted time/checkpoints, offline/partition behavior and
   permitted stale exposure; no numeric allowance is invented by this memorandum.
5. Define evidence sufficiency for immutable custody, actual adapter use and
   provenance assertions, including compromised verifier/host limitations.
6. Reconcile HD02–HD06, then decide whether an independent bounded conformance
   proposal may be prepared. Approval of these documents is not execution approval.

**Disposition: RESEARCH OPTIONS FOR INDEPENDENT REVIEW — NO AUTHORITY MODEL
SELECTED OR ADOPTED.** Next action is evidence-linked review of alternatives,
assumptions, falsifiers and missing decisions. Do not populate trust roots,
thresholds, keys, policy epochs or approvals on behalf of a human. All F01 STOP
conditions and the ADR's operative/non-operative distinctions remain unchanged.

## Evidence and preparation references

All repository links below pin the exact baseline. Source facts are distinct from
future contracts; no existing source is promoted into HD01 authority. See
[PREPARATION.md](PREPARATION.md) for hashes, scope, command evidence, external-source
versions and omitted execution. Preserve both this proposal and prior evidence.

[ADR]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md
[ADR-VALIDATION]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/docs/records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md
[D3]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/spec/rfcs/RFC-00D3-model-manifest-schema.json
[PROVENANCE]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/scripts/ci/check_ai_model_provenance_gate.py
[KEYRING]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/scripts/ci/ai_model_provenance_keyring.json
[KERNEL]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/ternaryos/kernel/kernel_abi.hpp
[AXION]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/kernel/axion/axion_api.cpp
[LLAMA]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/tools/model/llama_cpp_adapter.cpp
[CANONFS]: https://github.com/t81dev/t81-foundation/blob/cb9216f42f63e95e58709a71e08d38139c71e48b/fs/block_backed_driver.cpp
[TUF]: https://theupdateframework.github.io/specification/latest/
[DSSE]: https://github.com/dsse-lf/dsse/blob/master/protocol.md
