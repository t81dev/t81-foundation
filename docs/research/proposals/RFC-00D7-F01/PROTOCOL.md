# RFC-00D7-F01 — Bounded contract-falsification protocol

Status: **PROPOSED / NON-NORMATIVE / NOT IMPLEMENTED / NOT EXECUTED**.

This document authorizes nothing. The current human instruction permits preparing this protocol and explicitly excludes implementing or executing its experiments. No production code, specification, research gate, PR #804, Commons index or historical finding is changed.

## 1. Identity, question and provenance

- Protocol identifier: `RFC-00D7-F01`, revision 1; six scoped experiment labels A–F.
- Candidate under test: PR #804 at **`ec22a7588282d37ad027cff61c807bcf159ac2e0`**.
- Comparison document: **`4fdba573cc3be94ceb69cbcb3b04b62083b4d252`**.
- Original repository base: **`40695b0ba5cea3e43563b6ed2443c987d2738089`**.
- Author: Codex assistant in the current session; model identity is not independently recorded and remains `null` in the proposal envelope. Gemini's RFC authorship is user-attributed. This author also performed the preceding reviews and has prior context; neither author nor oracles have independent human review yet.
- Canonical relationship: **H07**, representation-independent governed execution and a defined auditability property. A–F are bounded contract questions, not replacement canonical hypotheses. This work cannot establish comparative assurance, ternary advantage, OS isolation, real storage durability, or production readiness. H01–H08, Hanoi/CWEB evidence and all research/release holds remain unchanged.
- Research question: does the pinned draft specify satisfiable, replayable and internally consistent contracts for the six modeled areas, over the declared finite domains?
- Source anchors and byte identities are in `source-pins.json`; machine-readable Commons proposal metadata is in `proposal.json`. The RFC, actual RFC-00D3 schema, hash implementation and governance records must be read at their pinned revisions, not from moving branches.

The expected witnesses below are **predictions from prior source review**, not results of this protocol. Prior session countermodels must not be copied into a new results ledger as fresh execution. No code, synthetic inputs, enumeration results or experiment artifacts are supplied by this drafting task.

## 2. Common method and independent-oracle requirements

The unit under test is a small, declarative interpretation of the draft, not T81VM, Axion, CanonFS, DPE, a model-service ABI or a native backend. Models use bounded integers, booleans, short byte arrays and explicit event lists. No machine-learning model or executable bytecode is needed.

For each experiment, record separately:

1. The exact RFC sentence/test being challenged and any interpretation necessary to make it executable.
2. A **candidate admissibility predicate**: which histories the pinned clauses permit.
3. An independent reference oracle derived from arithmetic, linearizability, information sufficiency, allocation conservation, byte-format rules or the actual JSON schema.
4. A correct conventional control, with identical inputs and required outcomes, plus an intentionally defective negative control.

An unsafe mutant is not evidence against the RFC merely because it fails. A genuine witness must satisfy the declared RFC-admissibility predicate while contradicting the challenged claim. If the document does not select an interpretation, report that ambiguity instead of silently choosing a repair. Where a clause already forbids an outcome, an implementation violating it is only a negative control.

**Oracle independence means specification and implementation separation, not a role label.** A future candidate model and oracle must not share transition, normalization, accounting or durability routines. Arithmetic oracles should use prefix counting rather than candidate checkpoint arithmetic; event oracles should use explicit total-order relations rather than the candidate queue; allocation oracles should sum allocation identities rather than candidate category counters. Third-party schema validation supplies an additional independent implementation for F. All dependency versions, source relationships and any shared primitives must be disclosed. A reviewer other than the harness author must inspect oracle equations, literal vectors and admissibility rules before execution; no such inspection is claimed now.

No performance comparison is proposed. The conventional controls check equivalent semantics, not speed. Hardware allocation, conversion and setup costs cannot be inferred from modeled counters.

### Result vocabulary

- `FALSIFIED`: at least one admissible witness contradicts the precisely scoped claim; retain its complete prefix, assumptions and oracle output.
- `NO_COUNTEREXAMPLE_IN_DOMAIN`: all declared cases completed, both controls behaved correctly, and no witness occurred. This is not proof outside the finite domain.
- `UNDERDETERMINED`: the draft omits an ordering, payload, type or lifecycle rule needed to select a unique oracle comparison; retain competing interpretations.
- `INCONCLUSIVE_LIMIT`: a state/time/memory/artifact limit or enforcement failure prevented completion.
- `BLOCKED`: inputs, source pins, oracle review, dependency identity or execution authorization are missing.
- `INVALID_RUN`: a reference/control check or harness invariant failed. A failed negative control cannot become a favorable RFC result.

The observer reports control outcomes independently. There is no suite-wide “service conforms” boolean. No statistical significance or error-rate estimate is inferred from deterministic enumeration. List coverage and excluded domains explicitly.

### Ordering, seeds and limits

Core cases use exhaustive enumeration once in ascending tuple order; **no random generator is used**. Reordering checks apply only to the named witness/control corpus, at most 16 cases per area, whose literal membership must be pinned and reviewed before execution; they do not repeat the entire enumerated domain. They use fixed seeds `0`, `1`, `42`, `804` as a precisely defined rotation: for a canonical list of length n, visit positions `(i + seed mod n) mod n`, for `i=0..n-1`. This changes visit order, not membership or scheduler timing. Any cap hit under a rotation yields `INCONCLUSIVE_LIMIT`, not a pass. Request `seed` fields are data, not permission to sample away adversarial cases. A visited state is every materialized candidate/oracle comparison state, including intermediate observable transitions, control cases and repeated visits; counts must not be reduced by calling a state a setup step. These caps may prevent exhaustive completion and never license a bounded-negative verdict on an incomplete search.

| Experiment | Maximum visited states/cases, including controls and minimization | Maximum wall time | Maximum runner memory | Maximum artifacts |
|---|---:|---:|---:|---:|
| A | 4,096 | 5 s | 128 MiB | 4 MiB |
| B | 4,096 | 5 s | 128 MiB | 4 MiB |
| C | 8,192 | 10 s | 128 MiB | 4 MiB |
| D | 16,384 | 10 s | 128 MiB | 4 MiB |
| E | 16,384 | 10 s | 128 MiB | 4 MiB |
| F | 256 | 10 s | 128 MiB | 4 MiB |

Suite ceiling: **49,408** visited states/cases; **60 seconds wall**, **50 seconds CPU**, **one CPU**, **256 MiB aggregate memory including the budget supervisor**, and **25 MiB uncompressed artifacts** including suite metadata. These are ceilings, not completion-time predictions. Limits apply across all rotations and reruns together. No retries receive a new allowance.

The proposed execution platform is one qualified Linux hosted runner with an existing delegated resource-control facility. A trusted external budget supervisor may launch one trusted experiment process at a time; these are harness processes, not native backend workers. Hard per-process/group memory limits and external monotonic deadlines must be established before a run; checking memory only by occasional samples does not establish the hard limit. If enforcement cannot be established within existing authority, report `BLOCKED`; do not escalate privileges or substitute an unqualified platform. This protocol does not claim feasibility or cross-platform equivalence of its future enforcement mechanism.

Modeled byte budgets in D and simulated durable bytes in E are **distinct** from these real runner limits. Any output compression must not be used to evade the uncompressed artifact cap.

## 3. Experiment A — Commitment-to-halt K-step bound

**Hypothesis / claim.** The halt condition in RFC §4.3 and AT-MS-13 establishes `0 <= halt_step - commit_step <= K`, rather than merely `halt_step - latch_step <= K`. AT-MS-24 must describe the same interval. This is a logical-step claim, never a physical deadline.

**Reference oracle.** Independently count `AdvanceStep` events after the authoritative `RevokeCommit` and before `Halt`. A timing tuple satisfies the commitment claim iff that prefix difference is at most K. The control rejects any halt later than `commit_step + K`. It does not copy the candidate's checkpoint/latch equation.

**Minimal model.** One request, one epoch, a synthetic instruction counter, periodic poll events, `RevokeCommit`, `Observe`, `Halt`, and before/after ordering at an equal counter value. The as-written candidate permits the first observing poll followed by up to K more steps. No real VM or AI opcode is called.

**Domain / seeds.** `K in {1,2,4,256}`. For each K, commitment counter in deduplicated `{0,1,K-1,K,K+1,2K-1}`; also include commitment 100 when K=256. Enumerate commit-before/after-poll ties. The first observing poll is derived from that explicit event order; choose halt offsets from the latch in deduplicated `{0,1,K}`. Counter horizon is 1,024. Enumerate all legal tuples, plus the four common order rotations; remain within A's shared cap.

**Predicted falsifier.** `K=256; commit=100; latch=256; halt=512`. The draft's latch inequality permits this tuple, but the commitment oracle counts 412 steps, exceeding 256. This witnesses a failed implication between clauses, not observed T81VM behavior.

**Expected output / criteria.** For every tuple, emit commit/latch/halt, tie order, both distances and both predicate results. A valid tuple accepted by the draft inequality and rejected by the commitment oracle yields `FALSIFIED`. Immediate halt at the first poll is a positive control. A deliberately delayed extra-K mutant must be rejected by the commitment control. Missing definition of commitment's authoritative counter/ordering is separately `UNDERDETERMINED`.

**Limits / stop / provenance.** Apply A's table row. Retain the first witness in canonical order and its complete event prefix, then run mandatory controls; at most 128 additional minimization attempts, charged to the same cap. Stop A after a verified witness and controls; otherwise complete the domain before a bounded negative result. Link evidence to RFC §4.3, AT-MS-13/24 and source-pinned line ranges.

## 4. Experiment B — Publication/revocation linearization

**Hypotheses / claims.** B1: if applicable revocation commits before output publication, §4.3's fence prevents publication. B2: AT-MS-05's unconditional rejection for a concurrent Halt/revoke race follows from the defined ordering. B1 and B2 receive separate verdicts.

**Reference oracle.** A conventional single serial coordinator maintains `(revoked_epochs, revoked_capabilities, output_visible)`. `Publish` is one atomic check-and-visibility event. It is allowed exactly when no applicable revocation precedes its linearization point. This oracle treats revoke-before-publish and publish-before-revoke as different legal histories; a call beginning is not a commitment.

**Minimal model.** Two capabilities at epoch 5, one request and one output bit. Events are `RevokeInvoke`, `RevokeCommit`, `FenceAcquire`, `Check`, `PublishOrDeny`, `FenceRelease`, and response. Enumerate total orders preserving invocation/commit and fence program order. Cover primary and fallback publication and revocation of the target versus the unrelated peer. Run no threads or sockets.

**Domain / seeds.** At most seven events, two capabilities `{1,2}`, one epoch `{5}`, two attempt kinds, epoch-wide versus selective revocation, and initially valid versus already revoked capability. Canonical admissible order enumeration with the common rotation seeds; prune impossible mutual-exclusion histories explicitly, recording why. Ambiguous O1 precedence is not invented by the runner.

**Predicted falsifiers and controls.** An invoked revoke whose commitment follows atomic publication permits output in the oracle; AT-MS-05 does not state this ordering distinction. If that history is not excluded by a defined O1 rule, it contradicts the unconditional test expectation or establishes `UNDERDETERMINED`. Separately, `CheckAllow -> RevokeCommit -> Publish` must be rejected: it is a **broken-fence negative control**, not automatically a draft counterexample, because §4.3 already prohibits that publication. Revoking capability 1 must leave capability 2 publishable within the same epoch.

**Expected output / criteria.** Emit invocation, commitment and publication order, scope, target, visibility and expected outcome. `FALSIFIED` requires a draft-admissible bad publication or an admissible ordering inconsistent with AT-MS-05. If B1 survives the complete domain, report `NO_COUNTEREXAMPLE_IN_DOMAIN`, independently of an ambiguous B2. If revoke commits first, publication must be denied; if publication linearizes first, no retroactive leak claim is made. No real sandbox or authorization-key claim follows.

**Limits / stop / provenance.** Apply B's row, including all topological orders and control cases. Use the common witness/minimization rule. Retain the admissibility decision for each disputed tie and cite §4.2–4.3, TM-02/08 and AT-MS-04/05/08. Human adjudication of an ambiguous ordering must be appended, not silently coded as an assumption.

## 5. Experiment C — Log sufficiency and scheduler-independent identity

**Hypotheses / claims.** C1: the session-log projection in §4.4, plus a pinned initial state, contains enough information to reconstruct authorization state and future verdicts. C2: the per-request canonical representation excludes host ordering wherever §2 and AT-MS-17/18/21 claim independence. These are separate questions; integrity of an untrusted self-supplied hash is not authenticity.

**Reference oracle.** An independent complete command-history interpreter maintains capability state from explicit command subjects and payloads. It groups histories by the candidate log projection and asks whether identical projections can require different responses to the same subsequent probe. A complete-log control retains the missing subjects/content references. For C2, an independently specified projection compares canonical bytes after removing only the explicitly excluded metadata, and verifies expected normative-field sensitivity.

**Minimal model.** One epoch, two capabilities, two requests, and histories of up to four commands from `{Dispatch(r1,cap1), Dispatch(r2,cap2), RevokeCap(cap1), RevokeCap(cap2), Cancel(r1), Cancel(r2)}`. Invalid-before-creation commands are marked invalid inputs, not contract failures. Capability state is preloaded and identical. Probe a fixed future request after each history. Interpret `event_kind` as an operation tag, not a hidden encoding of command payload; if the author intends otherwise, report that interpretation gap rather than retrofitting the runner.

**Domain / seeds.** IDs `{1,2}`, control-operation request ID `99`, epoch `5`; no more than four commands. Request seed data `{0,42,UINT64_MAX}`. Telemetry order coordinates `{1,2}`. Perturb one host field at a time, then a nested `LIP-Queue` coordinate; perturb one extension byte as a sensitivity control. Use ASCII keys/string values and decimal integer strings only; bounded strings are at most 64 bytes and arrays at most eight entries. This restricted projection is not a general JCS implementation or Unicode-conformance test. Use all common rotations within C's cap.

**Predicted falsifier.** Two prefixes revoke different capability IDs but have the same `{commit_seq,event_kind,request_id,epoch_id,s_latch?}` projection. A subsequent identical `Infer(cap1)` must receive different oracle verdicts. No replay function of only that projection and the same initial state can select both correctly. If `request_id` or `event_kind` is meant to carry an undocumented subject encoding, the result is `UNDERDETERMINED`, not a fabricated successful replay.

**Expected output / criteria.** Emit projection collision buckets, full commands, resulting states and the distinguishing future query. A demonstrated lost distinction with differing future verdicts yields `FALSIFIED` for C1 under the recorded interpretation. Merely replaying recorded outcome labels does not count as recomputing verdicts. For C2, changing excluded telemetry must preserve canonical input bytes; changing normative extension data must alter those bytes. Nested dispatch coordinates must either have a specified normalization or be reported ambiguous. A byte-input difference is not reported as an observed digest difference without computing the pinned digest.

**Limits / stop / provenance.** Apply C's row; stream collision detection and retain bounded witness buckets, not an all-pairs comparison. Use the common stopping rule separately for C1/C2. Pin §2, §4.3–4.4, TM-07/08/12 and AT-MS-12/17/18/21/23. The complete-log control is a counterfactual control, not an approved RFC amendment or the existing DPE `commit_epoch` interface.

## 6. Experiment D — Live allocation and logical-step conservation

**Hypotheses / claims.** D1: the proposed reservation gate implies all simultaneously live allocations fit `service_max_bytes`. D2: primary, migration, fallback and evidence consume one non-replenishable logical budget, preserving the evidence reserve. D3: bounded queue admission also reserves emergency capacity with a defined release lifecycle. D3 is `UNDERDETERMINED` if no slot reuse/retention rule exists.

**Reference oracle.** A conventional allocation-identity ledger records `(allocation_id, owner, size, live)` and sums every live allocation exactly once. A separate step oracle sums event charges by attempt and purpose; it enforces `primary+migration+fallback <= L-R` and `primary+migration+fallback+evidence <= L`. Counters do not reset on fallback. Mathematical byte units are not actual runner RSS.

**Minimal model / domain.** Evaluate three finite subdomains independently, not their Cartesian product:

- Memory: service capacity `{2048,4096}`, emergency slots `{2,3}` of 512 bytes; one worker and queue limit one. Each of seven live categories—models, swap staging, migration, arenas, output, trace, and IPC/queue metadata—has size in `{0,512,2048}`. Reject startup configurations with a negative usable budget. Record allocation identities so category aliases cannot hide double-counting or omission.
- Steps: primary/migration/fallback charges in `{0,1,2,4}`, total limit in `{1,4,8}`, evidence reserve in `{0,1,2}` with `reserve<=limit`; include the named `(L=500,R=50,primary=420,fallback_demand=100)` case. Enumerate individual charge events for boundary checks, rather than treating a too-large requested batch as already consumed.
- Capacity: queue limits `{1,2}`, active count `{0,1,2}`, queued count `{0,1,2,3}`, and slot count `{1,2,3,4}`. Test accepted work, full-queue rejection, terminal record retention and reuse only under explicitly named lifecycle interpretations.

Use the common rotations; no random allocation sizes or actual large allocations. All subdomains, controls and minimization share D's cap.

**Predicted falsifiers.** With capacity 4096 and two emergency slots, usable memory is 3072. Models 512 plus reserved arena 2048 pass the named reservation, but an independently live 2048-byte output makes total live non-emergency memory 4608. This is a witness only if output is allowed outside the reserved arena; otherwise record the enforced within-arena invariant and test it. For steps, a model that restarts the full 500-step budget for fallback contradicts the conservation oracle; if the draft already forbids that reset, it is only a negative control.

**Expected output / criteria.** Emit allocation/release ledger, gate result, exact live sum, slot owners, attempt charges and remaining evidence credit. An admissible gate-approved excess yields `FALSIFIED`; missing definitions yield `UNDERDETERMINED`. The named step case permits only 30 more non-evidence steps before the 450-step cutoff; evidence may then use up to 50. A correct all-live reservation/conservation control must reject the corresponding excesses. No allocation threshold or formula is silently added to the candidate to obtain a pass.

**Limits / stop / provenance.** Apply D's row; stream states without retaining all ledgers in RAM. Stop on the first independently checked witness per D1/D2/D3, within the shared budget, then complete mandatory controls. Cite §3.1–3.3, §4.5, TM-05/10/13 and AT-MS-03/14/19. Coverage must state that full joint memory/step interactions were not exhaustively enumerated.

## 7. Experiment E — Emergency record byte consistency and simulated durability

**Hypotheses / claims.** E1: the §4.5 512-byte layout and post-sync flag update preserve checksum validity. E2: an acknowledged `durable_persisted=1` has a recoverable matching record under the **declared simulated storage model**. AT-MS-19/22/23 must use that same model. This does not test actual `fsync`, power-loss behavior or CanonFS durability.

**Reference oracle.** Independent literal field-offset/coverage tables check a 512-byte record, fixed zero padding and the checksum over bytes 0–479. The digest is SHA3-512 truncated to its first 32 bytes after the ASCII emergency-domain prefix, as in the pinned `t81::hash::hash_bytes` implementation. One oracle uses a separately checked literal byte vector and Python `hashlib`; it must not reuse the candidate record packer. A storage oracle keeps distinct `volatile_image` and `durable_image`: successful simulated sync copies the former to the latter; failed sync does not establish persistence; simulated restart discards volatile state.

**Minimal model / domain.** One or two 512-byte slots. The enumerated crash core uses request/epoch/generation `1`, logical step `256` and a fixed short ASCII reason. Identifier `2`, logical steps `{0,1}` and reasons up to 16 bytes are control variants only, not a Cartesian multiplier of every crash case. Diagnostic status bytes are test-only tags; no stable production enum assignment is inferred. Execute the as-written abstract action sequence `Construct -> Hash -> Write -> Sync -> SetDurableBit -> Crash/Recover`, with explicit branches for recomputing and rewriting that are labeled **candidate interpretations or counterfactual repairs**, not silently added requirements.

For a single write, enumerate every completed prefix length `0..512`, simulated sync success/failure and crash points before/after each operation. For an explicitly labeled two-phase control, use boundary prefix lengths `{0,1,7,8,10,11,12,479,480,481,511,512}` for each write rather than the full two-write Cartesian product. Include a no-write, fully successful write and domain-separation control. Selected bit flips at offsets `{0,8,10,16,48,56,479,480,511}` check coverage and checksum rejection. Common rotations apply; E's cap includes all branches.

**Predicted falsifier.** A valid flag-zero record is written and successfully synced. Changing byte 10's durability bit afterward without rehashing invalidates the in-memory checksum, while the simulated durable image retains flag zero. Rehashing only in memory still leaves the persisted record unchanged. This challenges the sufficiency of the specified sequence; it does not prove that no corrected multi-phase protocol could work.

**Expected output / criteria.** Emit exact before/after bytes for the minimized witness, digest inputs, checksum validity, simulated acknowledged durability and recovered image. An allowed acknowledged success whose recovered record is missing, mismatched or falsely marked yields `FALSIFIED` within this storage model. If no acknowledgement/rewrite point is defined, report `UNDERDETERMINED`. Checksum-preserving partial old/new images are classified by their valid decoded generation, not automatically called corruption. Do not assume every interrupted write produces a detectable torn record; retention/slot-generation ambiguity must be recorded.

**Limits / stop / provenance.** Apply E's row. Payloads and fake journal images remain in RAM; **no real journal, `fsync`, SIGKILL or filesystem fault injection** occurs. Artifact writes contain evidence only. Retain the first failure and at most 128 minimization attempts within the cap. Pin §4.4–4.5, TM-09/13 and AT-MS-19/22/23 plus the actual hash implementation. A positive control uses an explicitly defined immutable-record/commit protocol and matching acknowledgement; it does not ratify that repair.

## 8. Experiment F — Admission satisfiability under the actual RFC-00D3 schema

**Hypothesis / exact claim.** There exists a manifest that both conforms to the unchanged pinned RFC-00D3 schema and satisfies §4.1 Stage 2's required `manifest.model_canon_hash81` binding and state-schema condition. A schema-valid metadata manifest is not automatically a service admission manifest.

**Reference oracle.** Use the pinned actual Draft 2020-12 schema with `jsonschema==4.26.0` and an independently written structural key-set proof: for a root object with `additionalProperties:false`, every present property must be declared. Compare that allowed set with Stage 2's required root binding field. This set proof does not share a proposed admission validator. The unextended schema is the authority for the schema arm; it must not be patched inside the runner.

**Minimal model / domain.** A schema-valid minimal metadata object with one tensor named `w`, shape `[1]`, declared format `safetensors`, reader `safetensors`, and bounded count/string fields. For the baseline metadata recipe, use the schema's literal constants, `source_path="synthetic/w.bin"`, `imported_format_summary="synthetic"`, `tensor_count=1`, `parameters=1`, root/tensor `trits=0`, `file_size_bytes` equal to the selected synthetic byte-string length, and only `provenance.host_format_reader="safetensors"`; omit optional provenance properties. Wrong-type controls replace one required value at a time with `null`; missing-field controls omit one required property at a time. This recipe is a protocol definition, not a generated fixture or a claimed validation result. The declared format is metadata only: no real SafeTensors parser or real model is involved. Synthetic opaque weight byte strings have lengths `{1,2,16}` and bytes drawn from `{0x00,0x01,0xff}` using only the all-zero, all-one and alternating named patterns—not all byte-string combinations. They are labeled synthetic, never valid trained weights.

For each relevant case, compare the metadata object without binding fields, with `model_canon_hash81` only, with `state_schema_hash81` only, and with both, using matching and mismatching model digests. Include absent/wrong-type required base properties and one-byte manifest/weight mutations as negative controls. Request seed data is `{0,42}`; enumeration and common rotations are deterministic. No more than F's total case cap may be generated.

**Predicted falsifier.** The current schema's allowed root properties contain neither proposed binding property and reject extra properties. Without `model_canon_hash81`, the Stage 2 equality cannot be satisfied; with it, the schema rejects the manifest. Establish this structural contradiction together with a successful baseline metadata-schema control. Do not report “all models rejected” as evidence that secure admission works.

**Expected output / criteria.** Emit exact schema revision/hash, allowed/required property sets, validator error instance/schema paths and the Stage 2 predicate result. A proof that the combined required/allowed property constraints are inconsistent yields `FALSIFIED` for satisfiability. A schema-valid metadata-only object must pass the schema control. A separately labeled versioned-wrapper control may express a working binding, but its success cannot count as compliance with unchanged RFC-00D3. Existing `load_model_via_tloadhash` is not called and is not treated as a byte verifier.

**Limits / stop / provenance.** Apply F's row. Stop after a checked structural witness and mandatory positive/negative controls, or exhaust the finite cases for a bounded result. Pin §4.1, TM-01 and AT-MS-01, the full actual schema, and its `additionalProperties`/`properties` clauses. Hash identities establish the exact input bytes, not model authenticity or caller authority.

## 9. Evidence packet, stopping and reporting rules

Every future run must produce a bounded evidence packet containing:

- `authorization-record.json`: attributable human scope, protocol digest, permitted harness revision and resource ceilings; no secrets or signing keys.
- `inputs-manifest.json`: protocol/source/dependency identities, exact harness and oracle revision, environment, commands, seed/rotation definitions, and the approved interpretation table. Working-tree changes are identified explicitly; moving refs are not sufficient.
- One bounded case ledger per area with case ID, input digest, explored-state count, control outcome, admissibility, oracle outcome, result and stop reason. Prefer fixed-width records of at most 128 bytes plus a documented decoder; cap retained full witnesses at 512 KiB per area. Artifacts must remain independently readable and uncompressed caps must be enforced before appending.
- Complete minimal witness histories and byte arrays; ambiguity branches; positive/negative control records; allocation or storage ledgers sufficient for independent replay; observed limits and failures.
- `summary.json` and `REPORT.md`, with A–F verdicts, exact checked domains, exclusions, counterexamples, incomplete coverage and interpretation limits.
- `SHA256SUMS` for all packet artifacts except itself. A digest establishes content identity, not truth, authenticity, authorization or independent review.

Do not retain only hashes of missing raw witnesses. Do not log secrets or host account information. Do not generate more cases once the remaining artifact allowance cannot retain their required evidence.

**Stopping order:** validate authorization and pins; validate controls; examine cases. A bad oracle/control halts the affected area as `INVALID_RUN`; a shared primitive failure invalidates every dependent area. A genuine first witness stops that subhypothesis after controls and bounded minimization; remaining areas may continue within the global ceiling. Incomplete or ambiguous checks remain labeled accordingly. Any memory/time/state/artifact cap stops the affected process immediately, preserves the available evidence and terminates that allowance. No automatic larger-budget retry, altered domain or weakened assertion is permitted.

The first run may not be called a confirmatory or independent replication study. Oracle authorship and reviewer relationships must be disclosed. Preserve every failed, negative, inconclusive and superseded result separately. Fixing a harness defect requires a new revision and a fresh applicable authorization; retain the prior invalid-run record. No scientific gate, RFC status, release hold or canonical hypothesis disposition changes automatically.

## 10. Proposed execution authorization boundary

**Current authorization:** protocol preparation only. `human_authorization.status = not_requested` refers specifically to experiment implementation/execution; no implementation or execution is granted by this file or its validation.

After review, a human may separately authorize:

1. **Harness preparation:** a small off-core synthetic model, independent oracles and controls in an isolated experimental branch/directory; no service implementation or production dependency. Pin the final code, protocol, source inputs and dependency lock; obtain oracle/admissibility review. This step itself is not authorized by the current task.
2. **One bounded run:** precisely the approved A–F domains, interpretations, resource limits, seeds and artifact destination, with an accountable budget supervisor and no hidden repeated allowances. The execution authorization must identify the reviewed harness SHA and protocol SHA-256, not just this protocol ID.

Permitted run operations would be integer/byte-array modeling, schema validation and hashing in RAM, plus bounded writes to a newly designated scratch evidence directory. A trusted external supervisor is allowed only for enforcing this experiment's resource limits. No native backend workers, real model inference, networking, credentials, OS sandbox tests, kernel changes, real journal writes/flushes, process-kill tests, performance measurements, production integration or release actions are included.

PR #804 and all normative specifications remain untouched. Resource-control setup requiring privileges or unavailable facilities is outside the proposed run authority and must block execution. Any request to expand these boundaries requires a new human decision. Research Commons discovery, JSON schema validity, an agent's recommendation or an `authorized` field cannot substitute for that decision or activate Protocol 001.

## 11. Preparation-only validation

During drafting, source retrieval, source hashing, Commons discovery and proposal/document structure validation are permitted preparation activities; they must not enumerate A–F inputs, execute their oracles or create a results packet. Record those checks in `PREPARATION.md`. Successful preparation validation is not an experiment result and does not grant execution authority.
