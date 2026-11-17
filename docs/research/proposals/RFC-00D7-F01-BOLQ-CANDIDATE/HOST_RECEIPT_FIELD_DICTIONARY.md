# Proposed versioned host-qualification receipt field dictionary

**NON-NORMATIVE CANDIDATE — NOT ADOPTED — NOT A SCHEMA OR RECEIPT.**

Proposed record_type: t81.f01.host-qualification.bolq.v1.
Proposed schema_version:1. Proposed assurance_model:F01-HOST-BOLQ-v1.
These identifiers are proposals, not registrations or implemented parser routes.
No example instance, populated approval, executable validator or active schema
is supplied. All "required" rules below describe a possible future version only.

This type is distinct from the existing evidence.v2 $defs.qualification. It
must not be passed to today's strict parser/gate or serialized as a legacy
receipt. limits_demonstrated:true is intentionally not a field in this proposed
type: do not reinterpret, alias, auto-upgrade or backfill it. Legacy records
retain their original meaning/history. Any future schema/parser/gate deployment
needs explicit migration approval and new exact source identities first.

## Value and size conventions

DigestRef = SHA-256 of available raw bytes plus safe relative artifact path,
byte length, artifact role and access/retention scope. A hash without the raw
mandatory evidence available to authorized review is not complete evidence.
CommitRef = repository, exact40-hex commit, path and blob/content SHA-256 as
applicable. No moving refs/latest selectors. Identity values and timestamps are
observations with sources, not authenticated merely by being JSON.

Outcome = PASS | FAILED | INCOMPLETE | NOT VERIFIED under A4. MeasurementRef =
raw samples DigestRef, unit/clock/counter source, interval endpoints, resolution,
sampling gaps and prospectively fixed error method. DecisionRef = attributable
human decision record, exact reviewed object digests, date/scope and externally
verified provenance; a digest, username or boolean alone is not approval.

Proposed core receipt remains<=8192 UTF-8 bytes, including whitespace, inside
the existing25MiB uncompressed evidence cap. Larger coverage/raw/counter/decision
manifests are referenced artifacts within that same cap, not inline unlimited
arrays or new storage allowances. Future implementation must demonstrate actual
fit; otherwise STOP for design review, not silently enlarge the current gate
input limit. Unknown/duplicate fields and ambiguous versions must fail closed
in any later parser; no permissive fallback to the legacy type is proposed.

## Proposed top-level dictionary

| Field | Type / requirement | Intended meaning and prohibition |
|---|---|---|
| record_type | exact proposed string, required | Distinct receipt family; no legacy alias |
| schema_version | integer1, required | Must agree with record_type; unknown version is not accepted |
| assurance_model | exact F01-HOST-BOLQ-v1, required | Names scoped observed assurance, never universal timing |
| activity_under_test | provisioning_host_envelope or AF_host_envelope, required | Separates host qualification scopes; neither means A–F scientific conformance |
| receipt_id / campaign_id | bounded unique strings, required | Stable correlation; not authority and not a budget-reset key |
| source_identity | bounded object of CommitRefs/digests, required | Complete adopted model/protocol/candidate/harness/guard/controller/diagnostic/installer versions; unknown missing identity blocks use |
| host_identity | observed manifest DigestRef, required | Exact host/boot/architecture/kernel/build/manager/interpreter/dependency/stdlib/venv/native identity and observer scope; no cross-boot/architecture transfer by default |
| control_identity | readback/implementation manifest DigestRef, required | Unit/limits/delegation/cgroup inode/CPU mask/scheduler/rights/OOM/storage policies, effective values and source/binary identities; configuration is not a latency result |
| scenario_coverage | prospective plan and actual coverage manifest refs, required | Scenario IDs/inputs/fault model, planned assertion set, applicability, missing/attempted counts and full outcomes; no retroactive omission |
| raw_evidence | raw packet/hash inventory DigestRef, required | Timings/counters/status/errors/checkpoints/manager exports and source associations, retained within25MiB; hashes are not substitute observations |
| measurement_uncertainty | prospective method plus actual uncertainty/gap manifest refs, required | Sources, sampling, resolution/intervals/error bounds and observer cost; no post-result widening |
| per_criterion_outcomes | outcome manifest DigestRef and compact counts, required | One entry per planned mandatory criterion, expected vs actual, threshold, interval, evidence ref and reason; outcome names cannot replace original stop cause |
| exclusions | prospectively approved manifest ref, required even if empty | Explicit requested-scope boundary and NOT VERIFIED claims; an exclusion is not a passing test or automatic risk acceptance |
| resource_observations | bound-value/readback and actual sample manifest refs, required | Fresh-D absolute CPU, clocks, peaks, bytes, phase/FINAL counters, post-bound use if observed, all custom costs; no reset, hidden CPU or late free writer |
| operation_outcome | Outcome plus completeness/scope object, required | Distinguishes scenario abort success from completed installation/experiment; interrupted operation cannot be labelled complete |
| qualification_outcome | Outcome, required | Rollup over declared mandatory assertions; not human acceptance or execution right |
| reaping_and_quiescence | scoped status/evidence ref, required | Separate signal/request, exit, wait status, adopted child inventory and population observations; unknown is not zero latency |
| durability_and_finalization | scoped status/evidence ref, required | Confirmed writes/flush/readback, last checkpoint, partial tail, missing seal and actual finalizer lifetime; no fabricated post-kill receipt |
| human_decision_provenance | bounded DecisionRef manifest, required | Model adoption/risk/scope, source review, host/provider/dependency/provisioning/qualification approvals as applicable; separates draft permission from adoption |
| applicability_and_drift | scope and invalidation manifest ref, required | Actual identities/workload/load/fault window; any permitted reuse/drift requires explicit reviewed decision rather than implicit freshness |
| negative_and_superseded_records | retained record refs, required even if empty | Append corrections/supersession without erasing failed/incomplete/UNSUPPORTED evidence |
| record_integrity | canonical encoding/hash procedure identifier and evidence binding, required | Defines what is hashed, avoiding a recursive self-hash; content identity alone does not establish truth/authenticity |

There is no execution_authorized:true or qualified-by-boolean shortcut. A future
one-run authorization is a separate applicable human record and gate decision.
A complete PASS receipt alone cannot grant execution. Mandatory existing human
RFC §4.1 source-passage and oracle/admissibility review cannot be replaced by this
receipt, and the new model does not authenticate any human record by itself.

## Required referenced-manifest content

Host/control identity must preserve exact boot, kernel configuration, manager
unit/state paths, provider binary/build/patch, sysconfig/frozen/stdlib/lib-dynload,
venv config and symlink chains, installed package files, native candidate versus
actually loaded dependencies and observed rights. Never claim a native library
was loaded from a static SONAME/ELF guess. No secrets or signing keys are retained;
private provenance needs an approved access scope that still permits review of
mandatory raw evidence, not an opaque unverifiable hash.

Scenario coverage binds the plan's content hash and prospective decision before
testing, exact cases/assertions, named OOM/reclaim/starvation/blocking/C/G death/
rights/containment faults, intended load and observation window. For every missing
or unsafe case, record NOT VERIFIED and the requested scope it blocks. Fixed seeds
or repeated samples do not become a universal fault model. Sample count is not
evidence that the maximum possible delay is bounded.

Per-criterion entries preserve an outcome AND evidence_complete flag. Any known
violation stays FAILED when evidence_complete=false. No aggregate PASS can omit
a mandatory FAILED/INCOMPLETE/NOT VERIFIED entry. Preserve raw point estimates,
justified intervals and threshold decisions. An expected death/fallback case may
PASS its specific assertion while operation_outcome remains INCOMPLETE. The
criterion and operation are different objects, not competing labels to choose
whichever looks favorable. No PASS is inserted into A–F summary taxonomy.

Resource/clock observations include T0, Ta, actual phase and FINAL entries,
absolute cutoffs, sample begin/end/trigger/request/signal/exit/wait/persistence
measurements and uncertainty. Use the proper provisioning or A–F envelope;
provisioning53/47/54/48 values cannot silently change #809. Final CPU/quiescence
facts need a known observation owner and retained counters/manager scope. If
unknown, do not manufacture a terminal total after a controller/manager kill.

Durability entries name the exact local filesystem/failure scope; successful
fsync/readback does not prove untested power-loss behavior or modeled E semantics.
Manager export is ordinary existing evidence with collection timestamps, not a
replacement finalizer or an uncharged watcher. All collection/retention must
respect its applicable budget and authority, without rewriting an interrupted
operation as complete.

Human provenance must distinguish: drafting approval (already given for this
packet), actual model/contract/risk adoption (not given), preparation/host/lock
permissions, target qualification acceptance and one A–F authorization. Scope
changes, exclusions and correction decisions need their own immutable object
references and attribution; no field is prepopulated as accepted in this proposal.

## Future migration conditions — not performed here

The existing strict v2 receipt requires limits_demonstrated:true and different
fields. This candidate cannot satisfy or redefine that type. Before any use,
a separately approved change must implement explicit model-aware version routing,
full field/size/evidence validation, actual human-provenance checks and applicability
rules, while retaining all source/auth/dependency/isolation gates. Publish a new
schema/gate/harness identity and coordinated protocol/fa01/source pins. No code,
active schema, parser, receipt instance or migration is provided by this dictionary.
