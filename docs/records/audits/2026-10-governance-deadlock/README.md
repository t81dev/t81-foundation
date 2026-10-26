# Protected-branch integration deadlock investigation

Investigation date: 2026-10-26 (America/New_York). Scope: CI reliability and
newcomer clarity; no production architecture, ISA, hypothesis, result, or
research artifact changes. This report records failures rather than authorizing
exceptions to governance.

## Baseline and protection

- main: `01c85019319fd020922402ba356878e392cb2231`.
- #574, open: `10df367102151b087ee64b1fbb737c471f4c87fb`.
- #575, open draft: `b06f8520d02fd68301a5ae06777beb03b73c8eb5`.
- #575 base is `corrective/ci-failure-propagation`; its merge-base with #574
  equals the exact #574 head. This corrective branch starts at exact #575 head.
- Neither existing PR is retargeted, rewritten, merged, or otherwise modified.
- Effective main protection requires `quality gate / required` and
  `Analyze (cpp)`, with strict up-to-date checking. No rule changes or merge
  retries are part of this investigation. Prior #574 normal merge was rejected.

The `quality-gate` job in `.github/workflows/ci.yml` explicitly requires success
from all thirteen jobs: `spec-and-docs`, `determinism-slice`,
`determinism-repeatability`, `build-and-test`, `t3k-cross-arch-bit-identity`,
`t81lang-cross-arch-bit-identity`, `cxx-standard-compat`, `static-analysis`,
`sanitizers`, `fuzzing`, `formal-verification`, `benchmarks`, and
`tritwise-determinism`. Each failed or skipped dependency causes exit 1.
The separately running `governance-metrics` job is not one of these thirteen
dependencies; its failure is still a repository failure, not a passed gate.

At #575's existing Ubuntu run 40267221776, architecture coherence failed with
FileNotFoundError for `core/vm/vm.cpp`. Later spec/docs steps were skipped.
Separately, `python3 scripts/governance/check_determinism_claims.py` failed for
README.md; metrics generation was skipped. Build/test, reproducibility, analysis,
sanitizer, fuzzing, benchmark and formal jobs behind spec/docs were skipped.
They have not been proven successful by this investigation. #574 CodeQL
analysis and aggregate results were successful in the preceding investigation.

## Independently executed inventory

The complete commands, exit codes, stdout and stderr are in [before.json](before.json)
and [after.json](after.json). Before is the exact #575 checkout, prior to edits.
After is the bounded working patch before its commit. Each command executes
independently (no fail-fast shell chain), from repository root, with a 90-second
timeout; no command timed out. Local environment: macOS ARM64, Python 3.14.3,
full repository Git history. Translation-history results may differ in shallow
hosted checkouts; they are not waved through here.

The 27 mandatory-lane/governance-wrapper commands were all executed: before
14 passed / 13 failed; after 16 passed / 11 failed. Three additional informational
architecture commands were executed after the patch, returning zero. That does
not mean all GitHub Actions checks were executed locally.

| Command (under scripts/) | Before exit | After exit | Independent diagnostic / status |
| --- | ---: | ---: | --- |
| ci/check_architecture_targets.py | 0 | 0 | 11 targets, 19 reachable CMake files |
| ci/check_tisc_freeze_integrity.py | 0 | 0 | 209 opcodes; bitwise/reserved integrity intact |
| ci/check_ai_experiment_boundary.py | 0 | 0 | Sandbox boundary passes |
| ci/check_architecture_coherence.py | 1 | 0 | Three moved VM paths; delegated canonical policy source |
| ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0 | 1 | 1 | 7 tagged references; 101/108 pinned, zero unknown |
| ci/audit_workflow_permissions.py --max-missing 0 | 1 | 1 | qemu-boot-x86.yml lacks explicit permissions |
| governance/check_root_structure.py | 1 | 1 | Missing package manifests; unexpected root entries (full list retained) |
| governance/check_readme_naming.py | 0 | 0 | Pass |
| governance/check_translation_metadata.py | 1 | 1 | Four missing language-link tokens in README |
| governance/check_translation_staleness.py | 1 | 1 | Four stale README translations; 220.1 days / 69 canonical commits at baseline |
| governance/check_translation_semantic_alignment.py | 0 | 0 | Pass |
| governance/check_docs_structure.py | 1 | 1 | Five expected book/book-* directories absent |
| governance/check_active_docs_link_hygiene.py | 0 | 0 | Pass |
| governance/check_license_policy.py | 0 | 0 | Pass |
| governance/check_repo_artifact_hygiene.py | 0 | 0 | Pass |
| governance/check_public_api_semver.py | 1 | 1 | Config 1.9.1 vs lock 1.9.0; public API digest mismatch |
| governance/check_spec_code_alignment_baseline.py | 1 | 1 | Stale core/vm/vm.cpp and core/isa/encoding.cpp mappings |
| governance/check_stdlib_surface_baseline.py | 0 | 0 | Pass |
| governance/check_stdlib_promotion_snapshot.py | 0 | 0 | Pass |
| governance/check_status_label_coherence.py | 1 | 1 | Missing parsed README rows for t81lang, t81vm, axion |
| governance/check_cognitive_tier_boundary.py | 0 | 0 | Pass |
| governance/check_overclaim_guardrails.py | 0 | 0 | Pass |
| governance/check_rfc_lifecycle_hygiene.py | 1 | 1 | 11 metadata parse failures; RFC-0055/0056 accepted vs draft mismatches |
| governance/check_docs_governance_hygiene.py | 1 | 1 | Missing old-layout READMEs and aggregates the above failures |
| governance/check_determinism_claims.py | 1 | 0 | Root README omitted registry-bounded claim statement |
| ci/governance_metrics.py | 0 | 0 | Independently generated metrics; no claim of metric semantic correctness |
| governance/check_target_name_drift.py | 0 | 0 | Extra check invoked by governance wrapper |

Additional informational commands: `python3 scripts/architecture/check_dependency_firewall.py`,
`bash scripts/architecture/check_legacy_paths.sh`, and
`bash scripts/restructure/verify_restructure_clean.sh` each returned 0. The two
shell scans also emitted missing-search-root errors (obsolete core/runtime/tooling
directories). Their zero return is recorded as observed, not evidence of a clean
scan. Their handling of rg errors warrants separate investigation; untouched.

The GitHub link-check action and live DeepWiki HTTP checks are not represented
as independently executed local validators. Neither are package installs,
builds, model-dependent gates, or matrix tests. Hosted evidence must be read
with success/failure/skipped distinctions after pushing this branch. This report
does not claim a green or fully executed protected-branch baseline.

## Architecture coherence: authority and root cause

The original gate was introduced in
`099286d2126724c38888993e159ad9d69351c502` for RFC-0026 phase-1 dispatch,
canonical kernel policy enforcement, cross-architecture dependencies and WLOAD
allow/deny evidence. `ab9be0a5327f3a4a95a23abcae3bb2ae82cb85be` added tensor
provenance, shared decode and regression-target presence invariants. These are
structural checks supporting accepted specifications, not semantic VM proofs.

The missing files are not missing implementations:

- `cce005f5ef38b525f1b175f9a03f791356b31701` records **100% identical renames**
  `core/vm/vm.cpp` -> `vm/vm.cpp`, `core/vm/tensor_helpers.cpp` ->
  `vm/tensor_helpers.cpp`, and `core/vm/policy_trace_bridge.cpp` ->
  `vm/policy_trace_bridge.cpp`.
- Current `src/vm/CMakeLists.txt` explicitly compiles those three current paths
  into `t81_vm`; root -> src -> vm add_subdirectory edges establish reachability.
- `c36ccaff98cee8e36f246b147e2dab94dde1cd60` deliberately refactored CMake
  without changing build behavior. The canonical `kernel/axion/policy_engine.cpp`
  declaration is now in reachable `src/axion/CMakeLists.txt`, under `t81_axion`.
  After correcting only the VM paths in an experimental read, the old root-only
  CMake assumption would still miss that declaration.

The patch updates these three exact reads and includes the delegated Axion
CMake declaration alongside root CMake. Every existing assertion remains.
No filename fallback, recursive orphan search, source alias, or implementation
change is introduced. Classification: **validator defect correction**.

Contract traceability: accepted `spec/rfcs/RFC-0026-ai-native-inference-opcodes.md`
and normative `spec/tisc-spec.md`
/ `spec/t81vm-spec.md` define opcode/policy semantics; existing
`tests/cpp/vm_tensor_provenance_trace_test.cpp`,
`tests/cpp/vm_ai_phase1_shared_helper_parity_test.cpp`, and
`tests/cpp/tensor_native_decode_test.cpp` exercise the corresponding behavior.
The lexical coherence gate merely checks the named connective surfaces; this
assignment does not certify runtime conformance. Literal matches can still be
fooled by comments; arbitrary CMake semantics/reachability are not interpreted.
Those inherited limits are not relaxed or advertised as solved here.

## Determinism boundary: authority and root cause

`62530f2e2b012448b329e20fcd8487ea13bb1313` implemented the R-01 mitigation
and added the explicit README registry-boundary statement. R-01 is recorded in
`docs/status/ACTIVE_RISKS.md` and `docs/process/roadmaps-plans/ROADMAP.md`.
The gate requires summaries making deterministic/determinism claims to name
or link the registry. The baseline README claims deterministic execution,
bit-exact VM execution and DCP compliance, but has neither the phrase nor link.
The link was removed in the README reconstruction
`d343b18da46323fced7928e652a00cd16068317b`; subsequent hypothesis reconciliation
`84972dd6` did not restore it.

Authority is established independently of the string matcher:
`docs/governance/SPEC_AUTHORITY_MODEL.md` defers to normative specifications;
accepted RFC-0048 defines DCP, governed non-DCP, experimental and out-of-scope
surfaces. `spec/supplemental/deterministic-core-profile-v1.md` and registry
sections 2–4 require explicit verification and exclude timing/network/hardware
FPU guarantees and unverified backends. Policy governance does not promote a
surface into DCP. This requirement remains substantively valid.

Classification: **documentation correction** (a genuine omitted boundary,
not evidence that all determinism claims are false). Restore the linked boundary
near the introduction and explain inputs/configuration/numeric/platform limits,
reproduction versus bit identity, model/backend exclusions, and the distinction
between authorization/provenance and correctness or architectural proof.
No validator exemption or threshold change. All H01–H08 and negative results
remain verbatim. Other README path/status accuracy issues are explicitly deferred.

## Focused regressions and reproduction

Run from repository root:

```sh
python3 tests/ci/governance_deadlock_test.py
python3 tests/ci/check_architecture_targets_test.py
python3 scripts/ci/check_architecture_coherence.py
python3 scripts/governance/check_determinism_claims.py
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/check_tisc_freeze_integrity.py
python3 scripts/ci/test_failure_propagation.py --output-dir /tmp/t81-deadlock-controls
```

New suite: five test methods pass, with 20 independent invariant-removal/substitution
mutations and five independently written summary cases. Real source snapshots
are isolated from the checkout. Current layout without core/ passes; moving
the real source to an obsolete copy fails. Dispatch, placeholder-handler,
canonical policy source, cross-arch dependencies, WLOAD allow/deny, provenance
and test-target violations all fail with the expected diagnostic. Removing the
restored README boundary fails; bounded and no-claim fixtures pass. This tests
lexical enforcement, not factual sufficiency of the prose or VM semantics.

Existing architecture-target regressions: 18/18 pass. Failure-propagation
controls: 47 cases / zero failures. ISA freeze: 209 opcodes verified; no frozen
definition touched. Source behavior has not changed; no production build or
cross-platform runtime verification is claimed. pathlib/subprocess/tempfile
tests avoid platform-specific paths and shell dependencies; hosted Ubuntu
Python 3.12 validation supplements local macOS ARM64, with Windows unexecuted.

To rerun the complete inventory without fail-fast, load each `command` array
from before.json and invoke subprocess.run independently from repository root,
capturing returncode/stdout/stderr with timeout=90. after.json adds three
informational commands. Do not convert nonzero exits to successful checks.

## Minimal sequence and unresolved integration blockers

The two scoped corrections are justified and sufficient only for their two
failures. Supporting test changes and logged CI registration are **test
corrections**; there is no source defect correction or architecture expansion.

Next actual spec/docs blocker after coherence is **action pinning**, followed
by explicit workflow permissions and the governance command group. The eleven
remaining failing commands must each receive contract/history investigation:
some appear to have stale layout metadata, some missing documentation, and API
lock / RFC-status differences require explicit authority decisions. They are
**unresolved contract questions**, not permission to regenerate locks, relabel
RFCs, delete artifacts, or relax policy. Wrapper failures partly duplicate their
children but also contain independent missing README/task-status evidence.

Smallest defensible dependency order:

1. Independently review these two corrections on the exact stacked diff.
2. Investigate pinning/permissions, then remaining governance categories,
   fixing only justified contracts in separately authorized bounded work.
3. Execute every previously skipped required job on the resulting cumulative
   tree; investigate any new failures before proposing integration.
4. A stack cannot by itself unblock #574: GitHub evaluates #574's own head,
   which lacks descendant corrections. Once all needed fixes are reviewed,
   obtain maintainer authorization for a normally protected cumulative
   integration PR against main, or for bringing reviewed prerequisite fixes
   onto #574 and re-reviewing its changed head. Neither is done here. Existing
   SHAs cannot both remain unchanged and acquire descendant fixes.
5. Integrate only through successful required checks and normal review rules;
   reconcile redundant stacked PRs only with explicit later authorization.

No merge retry, bypass, branch-protection change, existing-PR modification,
automatic expansion to the other failures, or new scientific conclusion.
