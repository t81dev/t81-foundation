# Scope A implementation — preparation only

This is a concrete implementation, not another investigation. **No policy
activation, merge, branch-protection change, cleanup or scientific change.**

## Authority and baseline

The authenticated [maintainer comment](https://github.com/t81dev/t81-foundation/pull/579#issuecomment-6419311331)
is authored by @t81dev, confirms the sole active maintainer roster and approves
D1–D5 for preparation/testing. It explicitly reserves activation. The API snapshot
is [authorization-comment.json](authorization-comment.json); no final artifact
approval, signature or accepted RFC is fabricated.

Fetched refs and created isolated branch `corrective/root-governance-validator`
at exact #579 `71bb2c0e937dbcad73c9421e5465651b6e0d3ec3`. Both approved proposal
files were checked before implementation:

| File | SHA-256 |
| --- | --- |
| proposed-contract.json | `0548f2a2291f364db9d60196ffbe40403a611b08918cd939de0f7ba8350faab5` |
| proposed-contract.schema.json | `4e7abafa412343885316cb6729ebaa2f65f2a7c3a08c7ff1366f14e3a687a54d` |

The original contributor workspace and PRs #574–#579 are unchanged. The baseline
inventory was rerun in the clean #579 worktree, without repeating the historical
investigation. Implementation inventories were run from clean committed trees;
this audit was assembled afterward.

## Implemented components

- `docs/governance/root-layout-contract.json` and its schema record exactly 31
  required directories, 29 regular files and 17 preservation-only exceptions.
  Types/modes and preservation objects match the immutable approved proposal.
  `artifacts/` and `logs/` are required containers; obsolete root npm manifests
  are not required. There is no unknown-root wildcard.
- `scripts/governance/check_root_structure.py --structural` resolves one immutable
  commit/tree and reads schema, contract, root entries and CMake inputs from it.
  It rejects missing entries, unknown/generated additions, exact-mode drift,
  symlinks, gitlinks, substitutions, invalid JSON/schema, and preservation drift.
  Diagnostics are sorted. Uncommitted contract edits cannot redefine evaluation.
- The CMake check inventories literal reachable subdirectories and literal target
  source paths, including PROJECT_SOURCE_DIR and optional branches. Referenced
  root directories must be required, not just present or historically preserved.
  Computed subdirectory edges fail. This is deliberately not a full interpreter:
  include-loaded factories, source variables and macro invocations are not a
  universal dependency-discovery guarantee. The draft RFC states that limit.
- `--authorization --trusted-approval PATH` is a working external-input prototype.
  It binds an exact candidate commit to SHA-256 identities for contract, schema,
  this checker, the legacy gate and all tracked workflow/local-action files.
  Missing, malformed, candidate-local, incomplete or mismatched approvals fail.
- `tests/ci/check_root_structure_test.py` exercises real temporary Git repositories,
  exact preservation objects, index modes, committed mutations and external
  synthetic trust fixtures. It independently compares contract coverage to the
  immutable approved proposal instead of only accepting the implementation's list.
- RFC-0057, “Root Layout Authority, Preservation Exceptions, and Contract Revision
  Approval”, is **draft**, with a matching draft index row. No other specification
  or governance authority document is amended.

## Required CI remains on its pre-activation behavior

The no-argument root command delegates to `check_root_structure_legacy.py`, whose
bytes exactly match the #579 root checker. This explicit compatibility path is
necessary because replacing a command already called by required CI would
otherwise activate the new policy. New modes require explicit command flags;
there is no auto-activation through candidate metadata or environment variables.
No workflow or local-action file is changed. Do not delete this compatibility
boundary before a separate activation decision.

## Preservation guarantees

Every one of the 17 exceptions requires its exact path, mode, Git object and
continued presence. Directory object identity covers all descendant names,
modes and bytes. The owner is @t81dev, the status is preservation-only, and the
review trigger is any proposed change to the entry or root contract. Changed
files/subtrees, new descendants, missing or renamed entries and type replacement
fail. Neither the policies, witnesses, reports, demos, generated Makefile/log,
nor the equal original/exported TISC pair were altered or untracked.

[preservation-and-identities.json](preservation-and-identities.json) records the
independent object comparison, required counts, exact implementation digests and
byte-identical legacy/default workflow boundary. Digests identify implementation
artifacts for review; they are not evidence of final activation approval.

## Trust boundary and remaining activation dependency

Structural success **does not imply authorization**. The prototype requires an
explicit external operator input; its location outside the candidate checkout
prevents candidate-local discovery, but neither that location nor the string
`approver: t81dev` authenticates a human. The operator must independently
authenticate/protect the manifest, trusted evaluator copy, invocation and status
publication. Running candidate-controlled code is not an independent evaluator.

The passing authorization examples are synthetic test fixtures, not production
approvals. The real standalone command without a trusted approval fails closed,
as preserved in [standalone-modes.json](standalone-modes.json). No final approved
manifest is generated for this implementation. No signing key, credential,
external service, required status or branch-protection setting is introduced.

Before activation: accept the governance RFC through the existing process;
review/approve final contract, schema, checker and trust-sensitive workflow
identities; establish an independently controlled approval source and evaluator;
verify candidate changes cannot replace that source/evaluator or its required
result; obtain a separate authenticated activation decision. Any needed
administrator configuration/protection/credentials remain separately authorized
dependencies, not actions taken here. Existing protected-main status checks alone
do not establish this missing enforcement.

## Regression and inventory results

[regressions.json](regressions.json) records actual commands, evaluated commits,
exit codes and complete stdout/stderr:

| Program | Result |
| --- | --- |
| New root structure suite | **36 methods pass** |
| Failure propagation | **47 cases, zero failures** |
| Architecture-target discovery | **18 methods pass** |
| Governance deadlock | **5 methods pass** |
| Workflow supply chain | **6 methods pass** |
| TISC freeze integrity | **209 opcodes verified** |

Coverage includes every requested negative case: missing files/directories,
unknown entries, both substitutions, symlink/gitlink, generated output, omitted
CMake required classification, dirty worktree versus HEAD, preservation file and
subtree modifications/extension/absence/rename, contract/schema/validator/workflow
mutation, absent/invalid/local/stale approvals, malformed Git records and unknown
schema revisions. Negatives assert nonzero results (or the parser exception plus
nonzero CLI entry-point result for malformed Git), not just diagnostic text.

A portability-oriented fixture change initially lost executable index modes
inside historical subtrees; the validator correctly rejected them. The fixture
now restores every descendant's recorded index mode explicitly. The five failed
fixture outcomes are retained in [regressions-before-fixture-fix.json](regressions-before-fixture-fix.json),
and the repaired run plus final committed 36-method run are retained. No expected
result or production preservation check was weakened to make tests pass.

Complete [inventory.md](inventory.md) and raw JSON distinguish three executions:

| Execution | Passing | Failing |
| --- | ---: | ---: |
| Clean exact #579 baseline | 18 | 9 |
| Implementation, unchanged required-command interface | 18 | 9 |
| Implementation, direct root invocation explicitly using --structural | 19 | 8 |

All 27 commands were independently executed in each view, no skips/timeouts.
Only the direct root invocation differs in the standalone view; the aggregate
hygiene wrapper continues using the unchanged required-command behavior. Eight
standalone failures remain: translation metadata/staleness, docs structure,
API lock, spec/code mapping, status labels, RFC lifecycle and aggregate hygiene.
The new RFC creates no additional lifecycle diagnostic. No required gate success
is claimed, and no unrelated red command was repaired or suppressed.

Inventories and existing five regression programs name their clean implementation
commit; the final root suite was rerun after the fixture-only repair. Production
validator/contract/schema bytes were unchanged by that repair. Standalone modes
and preservation/digests were rechecked on the repaired commit.

The schema also passed independent Draft 2020-12 validation using jsonschema
4.26.0. The runtime checker uses a strict standard-library interpreter for the
schema's supported subset, rejecting unsupported vocabulary rather than silently
ignoring it. No package dependency is added to required CI.

## Reproduction

Run from a full-history checkout of the implementation commit:

```sh
python3 scripts/governance/check_root_structure.py --structural
python3 scripts/governance/check_root_structure.py --authorization
python3 tests/ci/check_root_structure_test.py
python3 scripts/ci/test_failure_propagation.py --output-dir /NEW/ABSENT/SCRATCH/PATH
python3 tests/ci/check_architecture_targets_test.py
python3 tests/ci/governance_deadlock_test.py
python3 tests/ci/workflow_supply_chain_test.py
python3 scripts/ci/check_tisc_freeze_integrity.py
```

The second command is expected to exit 1 without authenticated external input.
The failure-propagation output directory must not already exist. Exact inventory
argument lists are in `before.json`, `after-required.json` and
`after-standalone.json`; run each independently and capture both output streams.

The external prototype manifest shape is illustrated only by synthetic test
fixtures in `approve_fixture()`. Never generate a manifest from candidate files
and treat it as human approval. For operator-authorized evaluation, use a trusted
copy of the evaluator outside the candidate and an independently authenticated
manifest outside the checkout, passing `--repo-root`, `--revision` and
`--trusted-approval` explicitly.

Local execution is macOS ARM64. Git plumbing constructs symlink/gitlink and
executable-mode mutations without relying on Windows symlink permissions or
filesystem executable support; autocrlf is disabled in fixtures. No Linux or
Windows hosted execution, runtime rebuild or cross-platform certification is
claimed. The default gate and production runtime remain unchanged.
