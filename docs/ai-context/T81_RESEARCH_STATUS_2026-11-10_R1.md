# T81 research and PR operational reconciliation — November 10, 2026, R1

Task: **T81-CONTINUITY-2026-11-10-R1**. Non-normative operational overlay; implementation authorized by the supplied task, independent review required before merge. **READY FOR INDEPENDENT REVIEW — DO NOT MERGE.**

Fetched `origin/main`: **`dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`**, the #763 merge. Primary collection checkpoint: **2026-11-11T00:26:00Z (November 10, 19:26 EST)**. Requests were sequential, not an atomic GitHub snapshot. Refresh tips and checks before action.

`t81dev/t81-foundation` is the single active repository. Other t81dev repositories remain historical sources, not parallel development workstreams. Preserve Track R (representation research), Track F (foundation and assurance engineering), Track M (AI research methodology), canonical H01–H08, and Hanoi/CWEB lineage. This maintenance increment reduces handoff ambiguity; it adds no subsystem or governance authority.

Read the [machine-readable inventory](receipts/2026-11-10-r1/pr-inventory.json), [retained GitHub observations](receipts/2026-11-10-r1/github-observations.json), and [validation report](receipts/2026-11-10-r1/VALIDATION.md). The inventory covers **all 69 open PRs (39 drafts, 30 non-drafts)** and 13 relevant merged/closed PRs. Counts precede publication of this task's own draft. GitHub lifecycle, returned reviews, unresolved threads, workflow results, dependency basis, exact base/head revisions, observation checkpoint and recommended actions are separate fields. Empty returned review/check arrays do not mean approval or successful CI. Connector review-thread pagination completeness is unavailable; the inventory does not certify absence of other historical findings.

## Active work and immediate actions

| Workstream | Observed lifecycle | Evidence boundary / next action |
| --- | --- | --- |
| SCG-08 #763 | **MERGED — CORRECTIVE REVIEW REQUIRED — GATE 1 ACCEPTANCE WITHHELD** | Four unresolved Codex findings; new correction commit exists on the old PR branch, but no subsequent corrective PR found. Publish a narrowly scoped corrective PR, repair remaining issues below, then review its exact head independently. |
| SCG-08 #760 | OPEN, non-draft | Original branch `dab59a63e4000f81799a6b2303bbfab26300d526`; implementation arrived through #763. GitHub reports not mergeable. Maintainer must reconcile its disposition; do not replay the old tree blindly. |
| SCG-08 #761 | OPEN, DRAFT | Head `5c69f11579b96686de1520aa45062b418c7c3bdd`. Phase 0 acceptance report publication pending. Mathematical PASS is source-reported; statistical readiness conditional. Not a Gate 1 acceptance or Gate 2 authorization. |
| TernaryOS #756 / #757 / #758 | MERGED | Fail-closed verification repair and firmware investigations integrated. Phase 4 reports 2/5 native qualification checks in all four configurations, not native qualification. Preserve firmware/payload distinction and failures; no QEMU execution repeated here. |
| Visualization #759 / #762 | MERGED | Dashboard and subsequent data-consumer/canonical-hypothesis corrections integrated. #759 retains four unresolved historical review threads; #762 records the corrective intent. No browser/coverage promotion follows from merging. |
| Foundation #747 | OPEN, DRAFT | Head `b18075ad6de93091f9aea30707ef5237d479c9d1`; Foundation CI failure reported at that head, and transient not-mergeable metadata. Independently review portability correction and current conflicts/checks first. |
| Policy #748 / #750 | OPEN, DRAFT; base #747 | CanonFS authorization and explicit CLI policy fixes. Retarget after parent review/integration; recheck exact scope, denial-before-effects evidence and hosted checks. No head workflow runs returned for these stacked PRs. |
| Policy #751 | OPEN, DRAFT; base #750 | Literal-argv wrapper hardening depends on #750 and #747. Review/recheck in dependency order. |
| H04 #752 | OPEN, DRAFT; base #747 | Retention status overlay pending. Preserve registered-Python-object limits, lack of physical reclamation evidence and Phase C freeze. |
| SCG-05 #732 | OPEN | Read-only generic Result feasibility, not implementation authorization. Refresh conflicts and independent review. |
| ARCH-002 #714 / ARCH-003 #716 | OPEN, DRAFT; #716 bases on #714 | Proposed protocol and bounded Linux accounting evidence; no correctness-core pass, accepted R2 amendment or execution authority inferred. |
| ARCH-003 #718 / #719 | OPEN | Related R2 deadline-authority proposals; each has three unresolved returned threads. Reconcile proposals and maintainer decision before activation. |
| CPA #707 / #709 | OPEN / OPEN DRAFT | Reported bounded conformance on PR/integration revisions remains distinct from main integration and full ISA qualification. Preserve normative discrepancies. |
| BASE-01 #697 / #698 / #699 | OPEN DRAFT / OPEN / OPEN DRAFT | Retention contracts, physical qualification report and budget amendment remain separate; #698 has three unresolved returned threads. Do not promote unavailable physical-host evidence into qualification. |
| H04 #685 and AIOPT #665–#674 | OPEN (individual draft states in inventory) | Linux enforcement/design/audit proposals remain under review. Preserve conditional model negatives, M2 protocol blockers and measurement freezes. |

Recent relevant closed-unmerged records include #745 (superseded charter), #527 (unreconciled historical audit), and #503 (source scenario integrated/extended separately). Their closure is not a merge or blanket acceptance. Charter #753 and continuity #754 are merged. The complete inventory retains their actual merge identifiers and dispositions.

## SCG-08 Gate 1: separate dimensions

| Dimension | Disposition at this checkpoint |
| --- | --- |
| Implementation integration | **VERIFIED** by fetched Git history and GitHub merged metadata: #763 → `dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`. |
| Implementation correctness | **BLOCKED** for acceptance: incomplete binary primitive/condition accounting on main; correction candidate has residual discrepancies. Arithmetic correctness was not executed in this task. |
| Evidence integrity | **BLOCKED**: main artifact says 30 trials, implementation report says 179. Candidate changes the number to 179 but does not establish its execution provenance. |
| Independent review | #763 has four unresolved returned Codex findings; review text names `afa0714c4eae7b97067ffc5fe9ab3466e301b642`, distinct from final PR head `93a3d1bc36e18395d6863d56a604506d108eac32`. Candidate reviewed here only by source inspection and schema-only execution; independent Gate 1 acceptance pending. |
| Scientific acceptance | **WITHHELD**. Merged software and theoretical carry frequencies establish no computational or hardware superiority. |
| Gate 1 authorization provenance | **MISSING EVIDENCE** in inspected repository: harness references Addendum-06, while the Phase 0 charter still says authorization not granted. The user's task describes separately authorized Gate 1 work, but its original authoritative decision/addendum was not located. Preserve the distinction; request the record rather than inventing or revoking approval. |
| Gate 2 authorization | **WITHHELD**. No authoritative grant located; no Gate 2 work performed. |

The four #763 findings are P1 binary primitive multiplication/addition undercounting, P2 missing binary conditions, P1 30-versus-179 trial inconsistency, and P2 incomplete schema type validation. Their complete bodies and thread identities are retained in the GitHub receipt.

A correction exists at **`3969002e9d684bbf67b01aab485648aa2cd8c0d0`**, on branch `feat/scg08-phase1-gate1-arithmetic-harness-11726708696323888098-10915230409052001903`. It is one commit beyond the fetched main baseline, changing seven experimental/validator files. The closed #763 PR metadata still names its merged head `93a3d1bc…`; the later branch tip is not thereby merged. No new corrective PR was found; **PR publication pending**, no invented number. No workflow runs were returned at the correction SHA.

Source inspection confirms helper multiply/add instrumentation, binary result-normalization conditions, evidence count changed to 179, added primitive-counter fields, stronger explicit validation, and CMake registration for negative schema tests. The earlier Jules report's “11 negative tests” is a source-reported claim; the inspected candidate contains **12**, and all 12 reject their fixtures in the schema-only execution retained here.

Remaining candidate issues to carry into independent review:

1. **Primitive/condition accounting remains incomplete (source inspection).** Same-sign binary addition evaluates `a + b + carry` but adds one ALU count, and its carry-result comparison is uncounted; the padding loop increments ALU count for assignments. The helper now records four additions in `mul64`, while the sparse exact-count test describes three helper additions and expects eight total additions; the call path has four helper plus four accumulation additions before any carry-loop addition. The sparse assertion's expected eight therefore requires correction. No arithmetic executable was run to reproduce this failure here.
2. **Schema type gap independently demonstrated.** Candidate validator accepts `operand_trits: null` and `ternary_limbs_op: "two"`; Draft202012Validator rejects both against the candidate schema. Passing its 12 negative fixtures does not establish full conformance. See [probe output](receipts/2026-11-10-r1/correction-schema-probe.log).
3. **Evidence revision/run linkage unresolved.** Candidate evidence sets both `baseline_sha` and `head_sha` to pre-correction `dbf107fda…`, despite claiming changed counters. Counters and 179 are static JSON, not demonstrably emitted by the same test execution. Source-reported smoke results and old Codex 179-trial execution are not a new local run. Retain exact run identity, operands, counters, command/environment and trial receipt before acceptance.
4. **Phase 0 statistical readiness remains conditional.** #761's reported 136,500-sequence allocation has not been reconciled with the preregistered `10^7`-step criterion. This task does not solve the protocol or authorize a campaign.

## Continuity and visualization preservation

This overlay supersedes only the operational status and next-action guidance in the earlier [November 10 snapshot](T81_RESEARCH_STATUS_2026-11-10.md); its original bytes remain unchanged. Entry-point updates are prepended pointers. `CURRENT_STATE.json` appends `2026-11-10-r1` under the existing schema; **`selected_view`, top-level historical metadata, authorization, earlier views and historical snapshots remain unchanged**.

The supported dashboard PR dataset now uses freshly observed lifecycle/check data for 24 mapped records and points to the complete 82-record receipt. Research-program SCG-08 entries distinguish merged implementation, unresolved correction, review and withheld acceptance. Historical `authoritative_baseline_commit` values remain unchanged; new status observations name their own source baseline. Coverage manifest, execution-path topology, diagrams, frontend, source arithmetic, RFCs and frozen artifacts are unchanged. Earlier dataset bytes remain retrievable at baseline `dbf107fda…`.

No new independent scientific validation is claimed. This Codex assistant authors the overlay and schema probes and has the user-supplied Jules/Codex history as context; it did not author the inspected arithmetic correction in this task. Separate role labels do not establish scientific independence. Local execution is limited to discovery, document/data checks and schema validation; no arithmetic smoke suite, benchmarks, Markov simulation, production build or Gate 2 execution.

**Recommended next research action:** publish the branch correction as a new focused draft PR with an artifact-bound run receipt, repair the residual accounting/type/provenance issues, and obtain independent exact-head Gate 1 review plus the missing authoritative contract/authorization record. Resolve Phase 0 statistical readiness before considering any separately authorized Gate 2 campaign. CI/policy stack review remains a separate Track F maintenance lane.
