# Root layout contract reconciliation

**INVESTIGATION ONLY — NO GOVERNANCE CORRECTION. Outcome B: maintainer decision
required.** Investigated 2026-10-26 (America/New_York).

This follow-up supplies a reviewable typed contract and precise decision record,
without changing the validator, activating preservation exceptions, or moving,
deleting, modifying or untracking research artifacts. The assignment explicitly
permits this outcome when authority remains insufficient. This work stays in the
existing CI/newcomer-clarity lane; no subsystem or scientific claim is added.

## Exact provenance

Fetched remote refs and independently inspected PRs #574–#578. All remained open
and unmerged at the supplied heads; [pr-heads.json](pr-heads.json) records the
exact stack. Created isolated branch `corrective/root-layout-contract` at exact
#578 head **`f4ffa85bf0bcaf028b51462113bf19c2eebb0091`**. The original workspace
was not changed. Clean baseline and root tree modes/IDs are in
[baseline.json](baseline.json); the full inventory has separate clean pre/post
status evidence. Audit files were written after that clean inventory.

### Introduction discrepancy resolved from Git

The validator was first introduced by
**`529a9486240a525a62b3393a94b0b842d77b8096`** (2026-02-26), not
`01c85019319fd020922402ba356878e392cb2231`.

Evidence in [lineage-and-authority.json](lineage-and-authority.json):

1. `git log --all --diff-filter=A` finds the February introduction; the followed
   file history also starts there.
2. `git cat-file -e 529a9486^:scripts/governance/check_root_structure.py` fails
   because the file does not exist in that parent; the actual diff adds it.
3. Blame attributes the original exact-set logic and HEAD enumeration to that
   February commit. Later entries trace to their respective revisions.
4. Ancestry checks confirm `529a9486` precedes `01c8501931`, which precedes #578.
5. `git diff 01c8501931^ 01c8501931 -- scripts/governance/check_root_structure.py`
   is empty. Both sides have identical blob ID
   `f66410ef17f18d9ec5282923ac5dc04b182916bb`.
6. `01c8501931` is the October #569 research commit, changing root CMake and
   H04-R02 experiment files. It is a baseline containing the validator, not its
   introduction. The independent review's introduction attribution is therefore
   unsupported by this full-history repository; no scientific conclusion is
   affected by correcting that attribution.

Material revisions: `c087907d` added `experiments/`; `d969bb4f` removed the
CanonFS local store requirement; `326abf8a` added documentation/build/support
entries; `af4f07db` added the capabilities report; `3ec0fea8` replaced old layered
paths and removed `src/` and `experiments/`. The latter remains the last ancestor
change to the validator. Complete patches and blame are captured, not inferred
from audit prose.

## Authority analysis and stopping boundary

The specification authority hierarchy governs interpretation of system behavior;
it does not supply a complete current root layout. The active authority model
§4.F explicitly requires unanimous maintainer approval for **new root files**.
The exact text was introduced in `525757f0` and still exists. It does not
explicitly say whether root directories or restoring existing directories to a
validator are included. §6 and the architecture overview retain obsolete source
paths. Historical `PROJECT_STRUCTURE.md` is descriptive rather than an override.

Thus, it would be inaccurate to assert either that §4.F unambiguously prohibits
these three directory restorations or that it positively authorizes them.
Active dependencies demonstrate necessity for builds, not retrospective
governance consent. CODEOWNERS naming `@t81dev` is ownership evidence, not an
approval record or a definition of unanimous maintainer quorum. The read-only
GitHub review query for #578 returned no submitted reviews at inspection;
the independent review supplied in the assignment is accepted as context, not
misrepresented as a hosted review or implementation approval.

The assignment distinguishes technical evidence from authorization and directs
the agent to publish a proposal if authority remains insufficient. No documented
scope decision or contract-specific approval was found in the searched governance,
architecture and specification corpus/history. This is a scoped finding, not a
claim to have disproved the existence of private maintainer communications.
Consequently the permitted-root contract is not changed, including the otherwise
well-supported three-directory and npm corrections. The blocker is authority to
activate a reconciled policy, not inability to write a typed validator.

[DECISIONS.md](DECISIONS.md) separates the exact policy language from this
conservative interpretation and specifies the concrete approval evidence needed.
No new approval flow or protection setting is installed.

## Independently verified architectural minimum

- Root `CMakeLists.txt:27` unconditionally adds `src/`; `src/CMakeLists.txt`
  delegates the live core, VM, FFI, CLI and Direct API targets. This is a current
  build input, not simply an abandoned migration directory.
- `src/direct_api/CMakeLists.txt` builds
  `development_tests/test_real_direct_api.cpp`. That dependency does not authorize
  the generated executables and log sharing its directory.
- Root CMake references architecture/provenance experiments and optional radix,
  packed matvec and H04-R02 subdirectories. `experiments/` is needed for supported
  opt-in research configurations and repository completeness, not every default
  runtime execution. Research inclusion does not confer DCP guarantees.
- `cb54c175` explicitly deleted the root npm manifests on April 4. The former
  private root manifest had a docs-search script without dependencies; the
  lockfile had no external package inventory. Current documentation CI performs
  npm installation in `docs/site/search`, then runs Node directly. The extension
  manifest remains under `tools/vscode-t81/`. Captured workflow/script/CMake/
  Docker/Python-packaging searches found no active root-manifest consumer.

These facts independently support a narrow future correction; they do not
activate it. No placeholder root manifests were created.

## Proposed artifacts, all inactive

- [proposed-contract.json](proposed-contract.json): explicit names, observed Git
  types/modes/objects, evidence and authority status for all 77 root entries.
  It proposes 60 required entries and leaves 17 unresolved, including the already
  allowlisted generated `Makefile`. No approved optional or grandfathered
  exception is populated. Every candidate preservation grant is `NOT_GRANTED`.
- [proposed-contract.schema.json](proposed-contract.schema.json): deterministic
  strict shape for the **inactive proposal**, including required files versus
  directories, empty approved lists, exact names and disallowed symlink modes.
  It is not an authorization mechanism. Semantic checks and trusted approval
  remain separate.
- [CONTRACT_DESIGN.md](CONTRACT_DESIGN.md): committed-tree evaluation, strict
  types/modes, preservation pins, CMake consistency, optional working-tree
  diagnostics, and why untrusted candidate-local digests cannot authorize policy.
- [ENTRY_MATRIX.md](ENTRY_MATRIX.md): every root entry's technical role and
  authority, plus exact disposition questions and recommendations for all 17
  unresolved entries. Required does not mean every descendant is production.
- [DECISIONS.md](DECISIONS.md): D1 directory-scope/approval, D2 inherited inputs
  and npm removal, D3 individual preservation grants, D4 generated material and
  witnesses, D5 independent contract/checker trust. No approval fields are filled.
- [TEST_PLAN.md](TEST_PLAN.md): all twelve requested acceptance scenarios plus
  contract tampering, schema errors, optional typing and Git failures. This is
  explicitly **not** a passing executable test suite.

Preservation candidates pin exact baseline Git modes and objects. If approved
later, they permit exact continued preservation only, not replacement content,
new descendants, renaming, deletion or permanent authorization. Unresolved
entries remain failures. Matching original/exported TISC hashes do not authorize
deduplication. All listed research material and the root Makefile remain intact.

No `tests/ci/check_root_structure_test.py` or validator implementation is added:
Phase 6 is conditional on authorized implementation. No proposed policy is
executed as a production gate. Static schema/coverage/retention verification is
recorded separately and must not be mistaken for implementation regression proof.

## Verification

[inventory.md](inventory.md) compares every command with #578;
[inventory.json](inventory.json) preserves command arguments, exit codes and
diagnostics. All 27 independently reproduce **18 passing / 9 failing** from clean
full-history #578. There is no 19/8 improvement and no successful required gate.

All five requested regression programs pass with the required CLI arguments:
47 failure-propagation cases / zero failures, 18 architecture-target tests, five
governance methods, six supply-chain methods, and 209-opcode freeze integrity.
[regressions.json](regressions.json) retains the requested bare failure-propagation
invocation's exit 2 and successful retry with required `--output-dir`; no script
was changed to accommodate the command. Local environment is macOS ARM64; no
new validator or cross-platform build success is claimed.

The audit schema, JSON parsing, baseline coverage, type/object observations,
empty activation lists and preservation identity are checked in
[proposal-verification.json](proposal-verification.json). These checks establish
internal consistency of the proposed review artifact, not policy authority.
Reproduce with `python3 verify_proposal.py` from this audit directory in an
environment with `jsonschema==4.26.0`; [verify_proposal.py](verify_proposal.py)
is a read-only audit-data check, not a proposed root validator. Its displayed
SHA-256 values identify review artifacts and are expressly not trust anchors.

## Cumulative integration implications

The root check and aggregate governance wrapper remain red; all nine failures
remain independently recorded. Even an authorized future root-only correction
must rerun the full inventory rather than assume exactly 19/8: the wrapper may
continue failing for other reasons, and Git-history-sensitive checks can differ
in shallow CI. No threshold or failure propagation is weakened.

This new draft must target `corrective/root-layout-authority` at exact #578,
preserving #574–#578 heads. Publishing a descendant cannot retroactively satisfy
unchanged ancestor checks. Later cumulative integration still requires normal
review, successful required dependencies and CodeQL on an authorized candidate;
skipped or absent checks are not success. No merge, retarget, rebase, API-lock
regeneration, branch-protection change or scientific-result amendment is part
of this proposal. Stop after publishing the investigation-only draft.
