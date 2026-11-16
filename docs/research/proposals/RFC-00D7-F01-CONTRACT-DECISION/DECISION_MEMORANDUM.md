# RFC-00D7-F01 host-assurance contract revision decision memorandum

**NON-NORMATIVE PROPOSAL — DECISION PENDING — NOT ADOPTED OR QUALIFIED.**

This memorandum proposes language for human consideration only. It replaces no
operative document, protocol, schema, source gate or acceptance record. Until an
explicit decision and separately approved migration occur, the existing STOP
and linux_relock_required=true remain. Publishing/validating this memorandum is
not adoption, qualification, execution authorization or normative promotion.

| Reference | Exact head | Evidence role |
|---|---|---|
| PR #809 | `0afbc339288b44cc7536af6c227f957257bb8ff6` | Source-accepted supervisor/bootstrap/authorization/import plumbing; not host qualification |
| PR #810 | `0a369b8c7ce00979c970389e02e0da8d89d6ab0a` | Accepted input-profile proposal; runtime identities missing |
| PR #811 | `d0edeb486993860d733abc04b2ce740b35442874` | Accepted candidate timeline/enforcement design; unimplemented/unverified |
| PR #812 | `8ced4f076c3ea18594175a8e0a72885f54bb3c42` | UNSUPPORTED assessment of the unchanged complete failure-path claim; not a reproduced host failure |

All references are pinned by Git blob and SHA-256 in source-pins.json. The current
F01 protocol digest remains
`d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
Codex authored these preceding artifacts too. This is governance/architecture
analysis, not independent scientific or human validation. No host, installation,
interpreter, guard, probe or A–F execution occurred. H07 supplies context; H01–H08,
Hanoi/CWEB history, frozen ISA, RFC status and research/release holds are unchanged.

## 1. Original contract and authority boundaries

Here "original obligation" means a pinned research/admission requirement that
must govern any eventual authorized activity, not an assertion that draft RFCs
are frozen normative standards. F01 is a non-normative research protocol with
operative permission/limit clauses; #811 is a proposed provisioning contract.
RFC-00D7 remains a candidate; source acceptance never promotes it. Frozen ISA
semantics and RFC-00D3 schema are outside this decision and remain untouched.

Exact authority locations and required content:

| ID | Pinned location | Original obligation / assumption |
|---|---|---|
| O01 | F01 PROTOCOL.md, Ordering/seeds/limits and §8 | A–F sequential; one128MiB runner;49,408 visited-state ceiling including controls/rotations; F256 cap and FA01 planned254; no budget reset/retry |
| O02 | F01 PROTOCOL.md, platform paragraph in §2 | Establish real memory controls and external monotonic deadlines before a run; inability to establish enforcement blocks execution; no privilege escalation or platform substitution |
| O03 | F01 PROTOCOL.md §9 stopping order | On a real cap/enforcement failure stop the affected allowance, retain evidence, never enlarge budget or weaken assertions; immediate-stop wording has a physical-progress implication requiring qualification |
| O04 | #811 PLAN.md candidate timeline and resources.json | P0–P4<=44s wall/43s CPU including startup; FINAL10s/5s; planned quiescence54s/48s; final write cutoffs53s/47s or earlier entry-relative cutoffs |
| O05 | #811 PLAN.md independent guard paragraph | G survives C death/blocking; all custom G/C/R work in D; shared64MiB controller leaf; terminate all custom work within1s wall/1s CPU of the final cutoff despite admitted failures |
| O06 | #811 PLAN.md manager timeline | Same CLOCK_MONOTONIC origin T0 before custom work, active-enter Ta with0<=Ta-T0<=1;55s runtime expiry at T0+55–56, required<=2s completion/reaping after expiry and latest58s |
| O07 | #809 supervise.py/topology/gates; #810 profiles | Exact source/clean tree, sealed authorization bytes, isolated venv paths, reviewed actual dependencies, host/boot and attributable human records before release; unsupported/missing features block |
| O08 | #811 evidence/finalization; F01 PROTOCOL.md §9 | Stable targeting, helper termination, population/owned-child reaping evidence; bounded checkpoint/terminal records and actual persistence scope; missing evidence cannot establish success |
| O09 | evidence.schema.json $defs.qualification | Strict existing receipt includes limits_demonstrated:true, dependency_relock_reviewed:true, exact identities and human RFC §4.1 review; the boolean alone cannot express a narrowed assurance scope |
| O10 | F01 PROTOCOL.md §10 and #811 H0–H6 | Acquisition/build/provisioning/re-lock/qualification/A–F permissions are separate; a record/hash/shape check cannot authenticate a human decision |

Read [the pinned protocol](https://github.com/t81dev/t81-foundation/blob/0afbc339288b44cc7536af6c227f957257bb8ff6/docs/research/proposals/RFC-00D7-F01/PROTOCOL.md),
[the pinned provisioning contract](https://github.com/t81dev/t81-foundation/blob/d0edeb486993860d733abc04b2ce740b35442874/docs/research/proposals/RFC-00D7-F01-HOST-PROVISIONING/PLAN.md),
and [the existing receipt definition](https://github.com/t81dev/t81-foundation/blob/0afbc339288b44cc7536af6c227f957257bb8ff6/experiments/rfc_00d7_f01/evidence.schema.json).
These paths and sections identify replacement targets; nothing here edits them.

Preserved numerical limits are60s wall,50s cumulative CPU,256MiB aggregate,
128MiB helper/runner,64MiB total controller/guard leaf,zero swap,one logical CPU
and25MiB uncompressed evidence. The provisioning storage proposal separately
allows128MiB read-only inputs+512MiB payload+256MiB staging+25MiB evidence=921MiB
within1GiB. No such storage allowance expands A–F's25MiB artifact allowance.

Provisioning is offline interpreter/venv/five-wheel materialization and inventory,
with optional separately approved metadata-only diagnostics. Its P0–P4/FINAL
schedule is not the A–F schedule. A–F retains area walls5/5/10/10/10/10s, area
state/artifact caps, logical arithmetic/event/proof domains and simulated E
journal semantics. #809 currently uses its own58s/48s observation guards and
59s/49s finalization guards; #811's53/47/54/48 must not silently overwrite those.
No qualification/provisioning fault probe, native load or real journal operation
becomes an A–F operation through this proposal.

The original failure concerns include shared-leaf/D OOM/reclaim, starvation/
throttling, blocked I/O, C/G death, lost authority and incomplete evidence.
Suspend/VM-pause allowances are not approved. The target kernel, manager, host,
provider and guard remain unspecified/unobserved. Exact source bytes are known;
physical enforcement and all fault-path latency bounds are not established.

## 2. Feasibility findings and the decision being requested

The [pinned #812 assessment](https://github.com/t81dev/t81-foundation/blob/8ced4f076c3ea18594175a8e0a72885f54bb3c42/docs/research/proposals/RFC-00D7-F01-GUARD-FEASIBILITY/ASSESSMENT.md)
finds no admitted mechanism that establishes the complete unchanged claim.
A second process in S is not a separate memory/scheduling failure domain. OOM
policy, pidfds, cgroup.kill, subreaping and PID1 stop policy are useful mechanisms;
none alone supplies successful completed1s/2s quiescence under every named fault.
A timer request, a signal, process exit and successful reaping are separate facts.

CPU accounting observes cumulative use; quota constrains periodic bandwidth.
A tighter quota plus a proved lifetime can support a CPU envelope, but assuming
the disputed lifetime to prove that envelope is circular. A finite maximum
observed delay does not establish every future delay. Moving a new custom watcher
outside D would change attribution even if its unit is called host-managed.
These are source/argument findings, not measured runtime failures or a theorem
that no stronger host can ever satisfy a stronger execution contract.

The proposed change is therefore **material assurance weakening**, not a claim
that the original guarantee was proved, already meant only observation, or can
be repaired by renaming configuration as enforcement. Numbers would remain,
but some prevention claims become scoped qualification criteria and measured
outcomes. If the resource owner requires unconditional physical non-exceedance,
this model cannot satisfy that requirement: retain STOP or investigate a stronger
host. Human acceptance of residual overrun risk is a separate explicit decision;
it neither increases an allocation nor makes an exceeded bound compliant.

## 3. Proposed assurance model and exact language — NON-NORMATIVE

Candidate model identifier: F01-HOST-BOLQ-v1. This is a proposed label, not a
registered/accepted contract or schema. The following text is an exact proposed
replacement/addition for the named obligation targets only if a later decision
approves a versioned amendment. All clauses are **NON-NORMATIVE / NOT ADOPTED**.

### L01 — Authority and scope (targets O02/O05/O06/O10)

> NON-NORMATIVE PROPOSED TEXT: Host qualification establishes only the measured
> behavior of the pinned host, configuration, source, dependency set, workload
> envelope and predeclared finite fault scenarios. It does not establish a
> universal response, termination, reaping or persistence guarantee. A different
> source/host/boot/kernel/manager/build/control configuration invalidates reuse
> unless a separately reviewed applicability decision expressly permits it.
> Adoption, source preparation, provisioning, lock activation, qualification and
> one A–F execution require distinct applicable human decisions. No activity is
> authorized by this model identifier, a PASS label or a JSON boolean.

### L02 — Mechanisms, admission and outcomes (targets O02/O05/O07)

> NON-NORMATIVE PROPOSED TEXT: Three evidence classes shall be reported separately.
> Mechanism evidence records the configured/read-back Linux resource, affinity,
> signaling, containment and wait controls and their exact implementation scope.
> Admission evidence records required identities, permissions, isolation, source
> and human approvals before releasing work. Outcome evidence records actual
> counters, times, exits, reaping and persistence from the declared cases. Kernel
> mechanism presence is not proof of a deadline or universal physical bound.
> Unsupported/missing admission prerequisites prohibit release. No control is
> described as tested until its target-host assertions and raw evidence exist.

### L03 — Preserved resources and overrun meaning (targets O01/O03/O04)

> NON-NORMATIVE PROPOSED TEXT: All existing numerical ceilings, phase limits,
> attribution, storage/evidence reservations and one-attempt/no-retry rules remain
> unchanged. Controls shall be configured and armed before work, with all custom
> costs charged from the original origin. The numbers remain stop requirements
> and acceptance bounds. Deterministic model-state limits, pre-write byte
> reservations, source/authorization identity and pre-side-effect checks retain
> their exact existing requirements; this revision does not convert them into
> after-the-fact observation. For physical host failure-path behavior only,
> this model does not assert that ordinary host scheduling
> will prevent every exceedance under every failure. A known exceedance is FAILED
> for the affected qualification/resource criterion, never prevented-by-detection,
> compliant completion or permission for additional work. Stop further admission
> and request termination immediately when the stop condition is observed; record
> observation-to-request, request-to-exit and exit-to-reaping separately. A known
> unsafe loss of enforcement triggers the same stop requirement before a cap is
> reached. No new phase, budget baseline, retry, larger allowance or compensating
> later average is permitted. Residual physical overrun risk must be explicitly
> accepted before any future execution authorization; if prohibited, retain STOP.

### L04 — Clock, CPU and observed latency criteria (targets O04/O05/O06)

> NON-NORMATIVE PROPOSED TEXT: Record CLOCK_MONOTONIC T0 before first custom work,
> actual active-enter Ta, all phase/FINAL absolute deadlines, and absolute fresh-D
> CPU accounting including guard, startup, stopping, reaping and finalization.
> Preserve the #811 candidate arithmetic: work44s/43s, FINAL10s/5s, quiescence54s/
> 48s, cutoff53s/47s, Ta-T0<=1s and runtime55s from Ta. The1s inner wall/CPU and2s
> manager-expiry-to-quiescence intervals are observed qualification criteria for
> each specified scenario, not guaranteed maxima for all admitted host failures.
> Keep the60s/50s outer acceptance bounds. Evaluate timing/counter uncertainty
> conservatively: PASS requires the supported upper measurement bound within
> the criterion; an interval straddling it is INCOMPLETE for that comparison.
> A supported lower bound above it is FAILED. A point estimate or intended polling
> period alone cannot establish PASS. Do not reset origins or backdate detection.

### L05 — Guard/finalizer loss and terminal evidence (targets O05/O08)

> NON-NORMATIVE PROPOSED TEXT: Guard health, shared-failure dependencies, remaining
> signaling authority and reaper ownership shall be explicit scope items. No
> survival assumption follows solely from a separate process or shared-leaf
> placement. Controller-only finalization proceeds after confirmed helper stop
> within the original FINAL allowance. A missing survivor, stalled operation,
> lost authority or unconfirmed quiescence cannot be reported as successful stop.
> Following unit termination, no replacement finalizer or new allowance may
> manufacture a receipt. Preserve confirmed prior checkpoints, partial tails and
> any ordinary existing manager records with their actual collection scope.
> Missing durability evidence is not successful persistence. Local write/fsync/
> readback evidence supports only its declared filesystem/failure scope, not an
> untested power-loss guarantee or the modeled Area E journal claim.

### L06 — Qualification classification (targets O08/O09)

> NON-NORMATIVE PROPOSED TEXT: Each admission check, fault scenario, operation
> outcome and evidence-completeness dimension shall remain distinguishable.
> PASS, FAILED, INCOMPLETE and NOT VERIFIED have the definitions in §4 below.
> Any conclusive violation is sticky FAILED for its criterion even if other
> evidence is missing. No aggregate PASS may omit a mandatory failed, incomplete
> or unverified criterion. A predeclared successful abort scenario does not make
> an interrupted provisioning or A–F operation successfully complete. Qualification
> PASS is a scoped empirical result requiring human acceptance, never a new A–F
> verdict, general service-conformance claim or execution authorization.

### L07 — Prospective coverage and STOP (targets O02/O05/O10)

> NON-NORMATIVE PROPOSED TEXT: Before tests, pin scenarios, assertions, thresholds,
> uncertainty rules, authority, evidence budgets and exclusions. Keep OOM,
> starvation, blocked operations, controller/guard death and authority-loss risks
> visible even when a particular fault cannot safely be exercised. Such coverage
> is NOT VERIFIED, not silently excluded or passed. A required scenario that
> cannot be contained, budgeted, observed or justified blocks the requested scope.
> No outside-D custom observer, privilege escalation, altered topology or extra
> CPU is authorized by this assurance model. Changes to those assumptions need
> their own human decision, not a scope note applied after a failure.

### L08 — Versioned receipt and claim restriction (targets O09/O10)

> NON-NORMATIVE PROPOSED TEXT: Any adopted bounded-observation receipt shall name
> its assurance-model version, exact source/host/control/dependency identities,
> scenario coverage, raw evidence digest, measurement uncertainty, per-criterion
> results, exclusions, actual limit observations and independent human acceptance.
> Claims shall say only that the listed criteria passed in the stated tested
> scope. They shall not say universal deadlines or failure prevention were proved.
> Legacy limits_demonstrated:true shall not be reinterpreted as acceptance of this
> different model or supplied without the evidence its original meaning required.
> A separately reviewed versioned receipt/gate migration is required; older
> receipts are retained and are not automatically upgraded or transferable.

These clauses together change the assurance claim. Cherry-picking L01's weaker
claim while retaining an unqualified legacy receipt or deleting L03's failure
semantics is not the proposed model. No clause changes A–F model algorithms,
logical K-step bounds, candidate/oracle independence, FA01 proof/fixtures/rotations,
F254/256 or F288-v1's deferred/unexecuted status.

## 4. Failure classification and rollup

These labels apply to host qualification/admission evidence, not replacements
for F01's FALSIFIED/NO_COUNTEREXAMPLE_IN_DOMAIN/UNDERDETERMINED/
INCONCLUSIVE_LIMIT/BLOCKED/INVALID_RUN scientific taxonomy.

| Label | Proposed meaning | Forbidden inference |
|---|---|---|
| PASS | Every mandatory assertion for the declared criterion/scenario has complete admissible evidence and meets the uncertainty-aware thresholds in that exact scope | Guaranteed future behavior, fully complete installation merely from cleanup success, or A–F permission |
| FAILED | Conclusive criterion violation: known overrun, unsafe authority/containment loss, wrong admission identity, observed late survivor, failed persistence check or false success claim | Hiding the violation inside missing evidence, retrying with a clean allowance or attributing it to modeled RFC behavior |
| INCOMPLETE | A criterion was attempted but required timing/counter/exit/reaping/durability evidence is missing, interrupted or too uncertain to decide, with no established violation for that criterion | Zero latency, successful persistence, lack of a counterexample, or no residual processes |
| NOT VERIFIED | Criterion/scenario was not attempted, not authorized, outside the approved applicability scope or blocked before it could be evaluated | Waived requirement or passing coverage |

Record criterion status and operation/evidence completeness separately. A known
violation stays FAILED even when completeness is false. An attempted operation
whose required terminal data are absent is INCOMPLETE unless a known violation
makes it FAILED. An admission check may be FAILED while the prohibited operation
remains NOT VERIFIED because it never began. Rollup order is FAILED first, then
INCOMPLETE, then mandatory NOT VERIFIED; only all-required PASS supports a scoped
qualification PASS. Optional/unrequested claims remain explicitly NOT VERIFIED.

| Event | Proposed disposition |
|---|---|
| Unexpected C death | Interrupted operation INCOMPLETE; FAILED where an overrun/unsafe survivor is known; retain actual manager/checkpoint evidence |
| Predeclared C-death test | The abort-response scenario may PASS only with all predeclared stop/reap/timing evidence; operation remains interrupted and cannot be called a completed installation |
| G death or loss of heartbeat | Stop admission/attempt remaining authorized termination; missing response evidence INCOMPLETE; known missed bound or admitted unsafe continuation FAILED |
| Predeclared G-death test | May PASS its response assertion only if the declared remaining path actually meets criteria with complete evidence; no claim G survived or full common-mode resilience |
| Manager termination | Normal operation is not successful completion; retain cause/checkpoints. A deliberate fallback test may PASS its limited assertion only with independent admissible completion/reaping evidence |
| Blocked operation | Label actual wait/cancellation/exit scope; missing exit evidence INCOMPLETE, proved missed deadline FAILED; a pipe-stall success says nothing about all uninterruptible waits |
| Missing reaping evidence | INCOMPLETE if attempted; NOT VERIFIED if not checked; observed survivor/zombie beyond its criterion FAILED; signal success/population-only evidence cannot substitute |
| Missing durability receipt | INCOMPLETE for asserted persistence; a confirmed write/fsync/readback failure is FAILED for that check; a verified earlier checkpoint does not repair the missing terminal receipt |
| Known cap exceedance plus missing terminal record | FAILED resource criterion, evidence incomplete; never downgrade to INCOMPLETE to conceal the known violation |

An expected fault is not itself failure of its predeclared abort test. Conversely,
expected absence of a final scientific receipt cannot support a persistence claim.
That distinction must be fixed before tests, not used later to rescue a result.
An A–F host-limit/enforcement interruption retains affected INCONCLUSIVE_LIMIT/
invalidity treatment under the eventual reviewed protocol; it cannot support a
complete bounded-negative result. Preserve any raw witnesses with their actual
validity/coverage limits for review, never relabel host failure as RFC falsification.

## 5. Resource accounting and measurement scope

No numerical change is proposed. Keep every cap in §1 and all custom C/G/R,
install/hash/diagnostic/load/fault/cleanup costs inside D, from T0/fresh-D zero.
The standard existing OS-manager role remains the already declared infrastructure
boundary; a new F01-specific host watcher outside D is still custom work. An
observed model does not excuse missing controller memory/CPU or an extra CPU.

Preserve53/47 cutoffs and54/48 planned quiescence as provisioning criteria, plus
60/50 outer acceptance bounds. Detection/request latency, target exit latency,
reaping latency, evidence seal and final control-process exit are all measured
separately; missed margins are failures, not additional finalization time.
cpuset/affinity, quotas, memory/swap limits and per-process RLIMIT controls still
must be configured/read back; report their policy scope and actual counters.
Do not call a policy value proof of universal physical non-exceedance, especially
when kernel transients, lost progress or missing peak/final samples remain unknown.

Qualification, provisioning and A–F are separate approved campaigns, each with
its explicit applicable envelope; no new per-case allowance is inferred here.
No automatic reset/retry, compensating average, extra host or excluded observer.
A55s fallback case need not fit alongside all other tests in one60s campaign;
a future separately reviewed schedule must account for it or remain blocked.
The applicable49,408 state/observation ceiling is never increased; qualification
counters are not fabricated A–F visited-state evidence.

Measured host times are not modeled logical steps or candidate canonical trace
identity. Do not normalize away a late observation to make a timing case pass.
Host-scope qualification neither changes the deterministic A–F algorithms nor
proves RFC logical-step, replay or journal claims. Final counters/exit evidence
must have an admitted observation owner and retained cgroup/manager scope; if
that cannot be established without an uncharged observer, mark the criterion
INCOMPLETE/NOT VERIFIED and block it rather than inventing the final total.

Retain raw/partial/checkpoint/log/receipt/hash bytes within25MiB, including the
reserved manager-record export. No compression or delayed collection conceals
a larger retained packet. Read-only H3 review/export is a distinct authorized
scope, never a resumed operation or a post-kill finalization allowance. If the
resource owner cannot accept the residual possibility of physical overshoot,
this model is inadmissible irrespective of passing samples or unchanged numbers.

## 6. Fail-closed admission and STOP rules

Before release require exact immutable source/profile/protocol/model identities,
clean worktree/export, reviewed real interpreter/stdlib/venv/native/package locks,
actual host/boot/controller/manager/rights data, preconfigured controls, readable
clocks/counters, known containment/reaper ownership, bounded evidence capacity,
prospective scenario/uncertainty rules and the applicable authentic human decision.
A scoped qualification record cannot waive any of those checks.

STOP if any mandatory admission prerequisite is false/missing, any requested
scope lacks evidence, any known overrun or unsafe loss of control occurs, a
survivor cannot be ruled out, or evidence cannot support its claims. Do not keep
admitting work merely to obtain enough samples after a failure. Issue only
remaining authorized stop actions inside the original allowance; no late writer,
repair, cap increase or new privilege is implied. Preserve negative/inconclusive
records and quarantine payload; no lock activation or promotion on partial PASS.

If the allowance is exhausted with survivors, only a previously authorized
OS emergency-stop path may remain; no new custom cleanup/writer gets a budget.
Any actual custom consumption after a bound remains charged and reported as a
violation, not free emergency time or a compliant revised allocation. Detecting
and reporting that event does not satisfy a prevention claim. The risk of this
case must be part of the human decision before authorization, not excused later.

No present scope is qualified, no model is approved, and no receipt exists.
A meaningful future receipt needs independently reviewed assertions and actual
raw results; source shape tests and the same author's summaries are not enough.

## 7. Migration impact and original-to-proposed traceability

| Original obligation | Proposed treatment | Retained invariant / migration consequence |
|---|---|---|
| O01 finite A–F domains, caps and no retry | Preserve unchanged | No new inputs, algorithms, states, proof facts or rotations; no numerical cap change |
| O02 pre-run controls and qualified platform | Split mechanism readback, admission and scoped outcomes via L01/L02/L07 | Still fail closed; research protocol's host-assurance wording needs an explicit future amendment before A–F can rely on the new model |
| O03 immediate physical stop implication | L03 distinguishes immediate stop request on observation from measured completion | Known overrun FAILED; not prevented-by-detection; future §9 wording required, not silently inferred |
| O04 provisioning timing | Retain numbers as L04 observed acceptance criteria/stop targets | No transfer of P0–P4 thresholds to A–F; all startup/FINAL cost retained |
| O05 universal shared-leaf guard survival/1s completion | Replace only the guarantee with declared finite fault coverage and measured outcomes | No custom work outside D or new leaf/CPU; mandatory unverified faults block requested scope; common-mode risk remains visible |
| O06 manager2s completion bound | L04 measured comparison in exact cases | Timer origin/settings still admitted; not evidence of guaranteed scheduler/kill/reap progress |
| O07 source/auth/import/dependency identity | Preserve source-accepted mechanisms | Sealed snapshot and strict venv checks remain; real Linux identities still missing; no active-lock clearance |
| O08 stop/reap/persistence | L05/L06 separate observations and missing evidence | No fabricated terminal receipt; observed known failures sticky; no post-kill finalizer |
| O09 limits_demonstrated:true | L08 requires a new explicit model/scope receipt version | Do not reinterpret the old constant or auto-upgrade receipts; exact schema/gate migration needs later approval |
| O10 distinct human authorities/holds | Preserve and add explicit model/risk decision | Proposal publication/validation adopts nothing; source review, model adoption and execution remain separate |

Per-PR consequences if, and only if, a later decision chooses the revision:

- #809: keep topology, authorization-byte snapshot, bootstrap/import checks,
  algorithms and limit mechanisms as source-accepted evidence. That status still
  means reviewed source bytes, not Linux qualification or universal prevention.
  Before admitting this new model, the qualification gate/schema must receive
  a separately reviewed version/model/receipt migration; static checks and
  identities must follow any such source delta.
  Do not reuse an old harness SHA for changed code. Current code stays untouched.
- #810: retain verified wheel/source hashes, architecture distinctions and proposed
  venv layout. INPUT PROFILE ACCEPTABLE FOR PROVISIONING REVIEW remains exactly
  that. Missing built interpreter/installed stdlib/native/runtime evidence remains
  missing; a new model does not approve a provider, ABI or historical patch level.
- #811: future docs would replace universal survival/latency premises with L01–L08,
  predeclared scenarios, measurement uncertainty and the §4 rollup. Keep timelines,
  budgets, all-custom-work attribution and unfilled authority records. Its prior
  DESIGN ACCEPTED statement needs explicit qualification as the earlier conditional
  design, not a passing host result or approval of this changed assurance claim.
- #812: preserve UNSUPPORTED for the original complete contract and its raw source
  reasoning. Append a separately approved supersession/applicability record if a
  different contract is adopted; do not overwrite it as FEASIBLE, negate the old
  finding or turn paper counterpaths into empirical test results.

The existing $defs.qualification cannot accept extra model/scope fields and uses
limits_demonstrated:true. A companion evidence hash alone does not resolve that
semantic ambiguity. Propose a future versioned host-qualification receipt and
explicit model-aware admission/gate review. Preserve existing A–F result schemas
and labels; do not inject PASS into their summary. If any envelope schema must
change, give it a new explicit version and migration, not an in-place weakening.
No such schema or gate is supplied or edited by this memorandum.

The F01 research protocol host-assurance/platform and stopping clauses (§2/§9/
§10) would need a separately reviewed amendment if A–F will use this model, even
when mathematical/model semantics are unchanged. A future amendment needs new
exact protocol bytes/hash, model ID and coherent downstream fa01-plan/source/
identity pins and static expectations. Keep the old protocol digest as historical
identity, never overwrite it with a claim that the bytes are equivalent. Neither
frozen ISA nor RFC-00D3/#804 normative semantics require change for this proposal.

## 8. Alternatives, human decisions and smallest next PR

| Alternative | Consequence | Decision/review burden |
|---|---|---|
| A — Retain current STOP | No weaker assurance claim or new execution risk adopted; current unsupported full contract still blocks host work | May accept this memo as analysis only; no implementation to chase an unproved guarantee |
| B — Adopt bounded-observed qualification prospectively | Enables a possible scoped empirical program; cannot promise physical cap prevention or survival through untested faults; failures still disqualify | Explicit assurance/risk decision, exact scoped receipt semantics, independent amendment/source/host review and separate permissions; no current qualification |
| C — Redesign around stronger host execution | Investigate proved resource/lifetime/authority/termination mechanisms or constrained kernel/I/O model | New feasibility/architecture/accounting proposal; RT/hardware/kernel labels alone prove nothing; not assumed available and no new resources authorized |

Recommendation for consideration: review B as a possible successor research
contract, but retain A as the operative disposition until an explicit decision
and all prerequisites are satisfied. This is a recommendation to deliberate,
not approval to adopt B or a finding that B's cases will pass. If unconditional
physical caps or all-fault quiescence remain mandatory, choose A or investigate
C rather than disguising a different claim under the old words.

Unresolved risks: common-mode S/D failure, guard liveness and authority, unknown
I/O/reaping delay, sampling gaps/clock validity, unobserved native loader behavior,
configuration drift, finite fault coverage, incomplete durability, source-derived
oracle dependence and inability to contain an overrun despite detecting it.
No numerical increase or outside-D exception is proposed to address these risks.

Explicit human decisions required, with no record populated here:

1. Select A/B/C and identify the decision maker/authority and immutable memo/model
   bytes. Acknowledge that B changes assurance meaning while leaving numbers.
2. If B is considered, decide acceptable residual physical overrun and fault risk,
   mandatory scenarios/exclusions, uncertainty/acceptance rules and claim wording.
   If the owner rejects that risk, STOP; samples cannot override the decision.
3. Independently approve a versioned research-contract amendment and future
   host-receipt/gate migration design. Preserve #812 history and attribution.
4. Separately authorize narrowly scoped source preparation, provider/host actions,
   lock changes, qualification and at most one A–F run, each only when its exact
   required artifacts/evidence exist. Maintain mandatory human RFC §4.1 review.
5. Any normative RFC/ISA/governance/release promotion or merge remains a separate
   decision outside this memorandum; no Commons field activates Protocol001.

Smallest next PR, only after a human selects B for amendment preparation: a
**documentation-only candidate research-contract amendment** confined to the F01
host-assurance/stopping/authorization paragraphs, #811 applicability text and a
proposed versioned host-receipt field dictionary. Place candidate bytes in a new
review path, retain old artifacts, and supply new candidate hashes plus this
traceability matrix. Do not implement a guard, edit active schemas/gates/locks,
change A–F domains or run anything in that PR. Actual adoption and any matching
source/schema implementation require later distinct approval and new identities.
If no option is selected, the smallest next action is independent review of this
memo while preserving STOP. Nothing is adopted by the present PR.
