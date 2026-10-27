# Governance integration acceptance — PRs #574–#577

**Status: NOT ACCEPTED / independent hosted verification outstanding.**
**Candidate head:** `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` (#577).
**Scope:** read-only evidence reconciliation. No validator, workflow, governance contract, branch protection, or runtime behavior is modified by this report.

## Provenance and limitations

This assessment reads the **existing author-generated receipts**, not a fresh execution of the 27 validators. The two source files are:
- [#576 after.json](../2026-10-governance-deadlock/after.json) (historical #576 head `41fa126cc82df6ed54b095d8dd67bfe066252066`);
- [#577 after-and-tests.json](../2026-10-workflow-supply-chain/after-and-tests.json) (author-run local results for the #577 correction, recorded before the final audit commit).

The author reports **16/27 pass, 11/27 fail** at #576 and **18/27 pass, 9/27 fail** at #577. The full JSON arrays contain *additional regression and informational commands*, so their unfiltered array totals (30 and 31) must **not** be represented as the 27-command governance inventory. The two improved commands are the unchanged action-reference and workflow-permissions validators. Neither regression test execution nor source review constitutes independent acceptance.

GitHub's head-associated workflow lookup for #577 returned only **QEMU Boot (x86_64): success**. No successful complete required quality gate or `Analyze (cpp)` check was established for this exact candidate. #574's own hosted main CI failed on target discovery and README determinism claims; the downstream #575/#576 repairs are documented, not yet independently hosted-accepted. No hosted workflow was dispatched by this audit.

## Observed local validator dispositions (retained #577 author receipts)

| Validator | Exit | Attribution / decision |
| --- | ---: | --- |
| Architecture target discovery | 0 | Repaired by #575; independently reproduce against exact cumulative head |
| TISC freeze integrity | 0 | 209-opcode check reported; not a full ISA semantic proof |
| AI experiment boundary | 0 | Bounded sandbox check reported |
| Architecture coherence | 0 | Repaired by #576; confirm against current CMake graph |
| Action pinning, tagged=0 unknown=0 | 0 | Repaired by #577; exact upstream SHAs still require trust review |
| Workflow explicit permissions, missing=0 | 0 | Repaired by #577; explicitness alone does not prove least privilege |
| Root structure | 1 | Governance authority conflict: missing package manifests and unexpected active project paths; **do not delete files or loosen allowlist** |
| README naming | 0 | Reported pass |
| Translation metadata | 1 | Four missing root README language-link tokens; resolve documented contract before editing |
| Translation staleness | 1 | Four translated READMEs stale by reported history thresholds; history-sensitive, not an environment excuse |
| Translation semantic alignment | 0 | Reported pass; distinct from freshness |
| Documentation structure | 1 | Five expected book directories absent; resolve canonical documentation layout |
| Active docs links | 0 | Reported pass |
| License policy | 0 | Reported pass |
| Artifact hygiene | 0 | Reported pass |
| Public API semantic-version lock | 1 | Config 1.9.1 vs lock 1.9.0 plus digest mismatch; **no auto-relock without API approval** |
| Spec/code alignment baseline | 1 | Obsolete `core/vm/vm.cpp` and `core/isa/encoding.cpp` mappings; verify authoritative replacement paths |
| Standard library surface | 0 | Reported pass |
| Standard library promotion snapshot | 0 | Reported pass |
| Status-label coherence | 1 | README lacks parser-required t81lang/t81vm/axion status rows; do not invent green status |
| Cognitive tier boundary | 0 | Reported pass |
| Overclaim guardrails | 0 | Reported pass |
| RFC lifecycle hygiene | 1 | Metadata parse failures and RFC-0055/0056 accepted-vs-draft disagreement; **maintainer authority required** |
| Aggregate docs governance hygiene | 1 | Composite failure of root/docs/translation/API/spec/status/RFC and obsolete README paths; do not double-count as independent root cause |
| Determinism claims | 0 | #576 bounded README claims to Determinism Surface Registry |
| Governance metrics | 0 | Generation reported; does not validate semantics |
| Target-name drift | 0 | Reported pass |

**Classification:** seven primary failing areas (root layout, translations, docs layout, API lock, spec/code mappings, status labels, RFC lifecycle), plus the aggregate docs-governance failure; translations include two separately failing checks. Some are stale structural assumptions, but **none is classified as harmless without authority and independent reproduction**. No failure is attributed to a runner outage on the available evidence.

## Integration verdict

- **#574:** failure propagation mechanism supported by retained negative/mutation evidence; hosted red checks demonstrate propagation, not a merge pass.
- **#575:** documented correction for CMake target discovery; acceptance blocked on independent cumulative-head execution.
- **#576:** documented architecture coherence and bounded README corrections; acceptance blocked on independent cumulative-head execution.
- **#577:** documented action pins and least-privilege permission change; only QEMU x86 hosted success observed for its exact head, not complete CI acceptance.

**No GO for protected-branch merge, policy relaxation, or governance activation.** Keep the four-PR dependency order, and preserve exact evidence identities. Do not merge a child before its parent or mistake stacked-branch CI for an up-to-date main merge-queue result.

## Required next independent verification

1. Check out the exact candidate SHA `b6a20706b3b37eebbfb5ab0293a88306a3dae29a` with full Git history. Independently execute the **27 commands recorded in the source inventories**, individually with captured command, exit, stdout, stderr, environment, commit SHA and auditor attribution. Preserve the source inventory rather than reconstructing commands from the shortened table above.
2. Confirm the expected 18/9 distribution *or record the exact deviation*. Check the two improvements with their unchanged zero-tolerance thresholds. Run all regression suites for #574–#577.
3. Obtain complete hosted Linux CI and CodeQL results against the cumulative candidate; distinguish success, failure, skipped, queued and untriggered jobs. Validate the required quality gate, not only individual green checks.
4. Independently disposition all nine remaining failures. Do not update API locks, RFC statuses, root allowlists or required checks without explicit maintainer authorization.
5. Request human review. Only then decide whether to merge in order or stage further, separately authorized corrective PRs.

**Stop condition:** any proposed correction would redefine governance authority, alter preserved historical evidence, weaken checks, or expand runtime/agent capabilities. This report is an integration assessment, not authorization.
