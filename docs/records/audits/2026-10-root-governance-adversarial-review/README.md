# PR #580 adversarial review and corrections

Reviewed head: `9c354da7ff02c71ae9a8d7d5c105c8131fccf6c4`.
Corrections remain on the existing PR #580 branch. This is implementation review,
not a new authorization investigation. Scope A preparation authority remains the
authenticated comment linked by the implementation audit. No activation, merge,
protection change, API-lock update, artifact disposition or scientific change.

## Confirmed defects and fixes

| Demonstrated false pass | Narrow correction | Regression |
| --- | --- | --- |
| Git replacement refs could hide a committed unknown root and replace bytes returned for an unapproved checker object, passing both structure and artifact digest checks | Every preparation-mode Git read disables replacement objects; candidate OIDs now resolve to their actual objects | `test_git_replace_cannot_hide_unknown_root`, `test_git_replace_cannot_forge_authorization_digest` |
| `--repo-root` could point at a valid nested shadow layout and ignore an invalid outer root | Require the actual Git checkout top level; force full-tree, literal-pathspec reads | `test_nested_repo_root_cannot_hide_outer_violation` |
| `ls-tree` normalized raw mode 100664 to 100644; inferred object kinds also allowed a blob to masquerade as a directory, a tree as a file, and a missing required blob | Parse raw root modes and independently batch-check root object existence/kind | Four raw-mode/object regressions |
| A root tree with out-of-order entries passed | Enforce canonical Git root ordering using directory-aware byte comparison | `test_unsorted_raw_root_tree_rejected` |
| Python JSON accepted the nonstandard NaN constant in schema metadata | Reject NaN/Infinity/-Infinity at JSON decoding, including schema and trust data | Schema regression plus all-three-constant control |
| A semicolon-separated CMake source list used the first required root to hide a later preserved root; CMAKE_CURRENT_SOURCE_DIR paths were skipped | Inventory literal list elements independently and recognize PROJECT_SOURCE_DIR, CMAKE_SOURCE_DIR and CMAKE_CURRENT_SOURCE_DIR prefixes | Two literal-source regressions |

The corresponding rejection tests were introduced and failed **before each
associated correction**. They demonstrated actual exit-0 results, not speculative
risks or diagnostic-only mismatches. Initial reproduction files record the first
three defects against unchanged reviewed checker logic. Subsequent recordings
were taken incrementally, with earlier fixes present but the corresponding
vulnerable component unchanged. Raw malformed trees were created only in isolated
temporary repositories, never in the project's object database.

Reproduction artifacts: `reproduced-defects.json`, `cmake-prefix-before.json`,
`authorization-replacement-before.json`, `nested-root-before.json`,
`raw-tree-defects-before.json`, and `root-order-before.json`.
The corresponding `corrected-defects.json`, `raw-tree-defects-after.json` and
`root-order-after.json` prove targeted rejection after correction. The complete
final suite is recorded separately below.

## Structural adversarial coverage

All **53 root-validator test methods pass**, including the original 36 unchanged
methods and 17 additions. Existing symlink/gitlink, missing entry, preservation,
duplicate JSON, contract/schema/validator/legacy/workflow mutation and unsupported
schema tests remain exercised. New controls include whitespace, newline, tab,
Unicode, glob-like and option-like unknown filenames, a non-UTF-8 Git name, local
action mutation, and activation-boundary checks. Negatives assert rejection,
not merely text in a successful result.

`regressions.json` is the final committed run at
`fb15c4e1` (full SHA recorded in each row). `all-root-tests.json` is the earlier
52-method passing run, before adding the demonstrated unsorted-root regression;
it is retained as intermediate evidence, not the final result.

## Independent preservation and activation checks

`identities-and-preservation.json` independently compares all **17 paths, modes
and Git objects** against the immutable #579 proposal, using replacement-disabled
Git reads. All match. Contract/schema content is unchanged. Directory OIDs pin
complete historical subtrees; no original/exported TISC file or other protected
artifact has been modified, deleted, relocated, deduplicated or untracked.

The legacy checker remains byte-identical to the original #579 validator.
A regression compares no-argument invocation's complete exit/stdout/stderr to
that legacy checker, even with fabricated environment activation flags. Required
workflow and local-action files are unchanged from #579. The preparation modes
still require explicit CLI arguments; no contract metadata or environment switch
activates them. This proves the boundary in this revision, not immunity to a
future explicit workflow/code change by an authorized maintainer.

## CMake coverage — verified limits, not a complete interpreter

`cmake-limits.json` and the reproducible `cmake_boundary_probes.py` demonstrate
three intentionally unsupported routes: a local included file, an invoked macro,
and an indirect source variable. Each genuinely adds a preserved market-demos
source to a target: independent CMake configure succeeds and the recorded
REVIEW_SOURCES property contains the source. The bounded structural inventory
returns 0; authorization without trusted input returns 1.

These are limits already outside the declared literal reachable-subdirectory/
target-source profile, not claims that the corrected validator detects them.
The literal list and predefined-prefix false passes **within** that profile were
fixed. A structural pass certifies root layout and the explicitly supported
literal checks; it cannot certify complete CMake dependency coverage. RFC-0057
now makes the demonstrated gap explicit. Broader coverage or rejection of dynamic
constructs must be decided before claiming complete build-system enforcement;
this review does not implement a general CMake interpreter.

## Contract self-authorization and independent enforcement

Existing and new regressions use a trusted evaluator outside each candidate
fixture. Altered contract, schema, checker, legacy checker, workflow and local
action data cannot match unchanged external digests; changed commits also fail
the independently supplied candidate identity. Candidate-local manifests fail.
Replacement refs no longer let candidate object content masquerade as approved
bytes. Workflow/action additions and removals fail coverage checks.

`trust-limits.json` and `trust_boundary_probes.py` explicitly demonstrate:

| Attempt | Actual result |
| --- | --- |
| Remove a required file and loosen its candidate contract consistently | Structure passes; this is not revision authorization |
| Use the original external approval for that altered candidate | Authorization fails |
| Supply a forged approval inside the candidate | Authorization fails |
| Give the evaluator a forged external manifest after violating the operator-trust assumption | Prototype accepts the supplied data |
| Run a modified candidate evaluator that simply prints success | That evaluator returns 0 |
| Use the trusted evaluator and original external input against the altered candidate evaluator | Authorization fails |

The last two success cases are **not** authenticated approvals or production
security guarantees. Location and an approver string cannot authenticate an
external file. The trusted evaluator, manifest authentication, invocation,
runtime environment and result publication must be controlled independently.
The prototype cannot establish those conditions itself, and is not a required
CI check. No production approval manifest, signing key, service or administrator
configuration is introduced. The current root-layout contract is unchanged by
these probes; loosening occurs only in disposable test repositories.

## Full verification

Final results in `regressions.json`:

- Root suite: **53 methods pass**, including all original 36.
- Failure propagation: **47 controls, zero failures**.
- Architecture-target discovery: **18 methods pass**.
- Governance deadlock: **5 methods pass**.
- Workflow supply chain: **6 methods pass**.
- TISC freeze integrity: **209 opcodes verified**.

All 27 governance commands were rerun independently on the clean correction
commit, with full stdout/stderr and actual exits:

| View | Pass | Fail |
| --- | ---: | ---: |
| Unchanged required-command interface | 18 | 9 |
| Direct root command explicitly using --structural | 19 | 8 |

Raw inventories are `inventory-required.json` and `inventory-standalone.json`.
The aggregate wrapper still invokes the unchanged default gate. No skipped job,
timeout or absent hosted check is counted as a pass. Verification was local
macOS ARM64; no new hosted-platform or runtime-build success is asserted.

## RFC-0057 readiness

The RFC remains **draft** and is technically suitable for separate maintainer
review of the bounded preparation implementation. It does not amend §4.F or
claim acceptance, deployment or independence that does not exist. It now states
the raw-Git safeguards, demonstrated CMake limits and forged-input/evaluator
boundary explicitly. Maintainers must settle the adequacy of that bounded
coverage and the independent production enforcement mechanism before activation.
No freeze or scientific governance rule is relaxed.

## Reproduction

Run the final `tests/ci/check_root_structure_test.py` for all regressions. To
characterize the known limits without changing production files:

```sh
python3 docs/records/audits/2026-10-root-governance-adversarial-review/cmake_boundary_probes.py
python3 docs/records/audits/2026-10-root-governance-adversarial-review/trust_boundary_probes.py
```

Both scripts create disposable Git fixtures and print JSON. CMake probes
configure those fixtures in an external scratch build directory, without building
or executing research code. They reproduce boundary limitations, not successful
production authorization. The exact six regression commands and 27-command
inventories are preserved in the JSON evidence; the failure-propagation output
directory must be a new absent scratch path when rerunning.
