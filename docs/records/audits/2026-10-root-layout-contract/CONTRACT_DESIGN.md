# Proposed typed contract semantics

Status: **PROPOSED_NOT_AUTHORIZED**. Neither the JSON nor this design is loaded by
CI, the current validator, or any production program. It grants no exception.

## Representation and authority

`proposed-contract.json` separates required directories, required files,
approved optional entries, explicitly grandfathered historical entries,
prohibited generated artifacts, and unresolved entries. Each observed entry has
its exact baseline Git mode/object and evidence/authority notes. These observations
are not the contract's authorization. `activation_authority` is null, both approved
optional and grandfathered lists are empty, and all 17 retention candidates are
`NOT_GRANTED`. The schema deliberately describes only a **proposal**; activation
requires a separately reviewed policy revision, not flipping a local boolean.

The 60 candidate required entries carry forward inherited non-generated root
expectations, remove the two deleted npm requirements, add the three active
directories, and remove generated `Makefile` from required status into unresolved
retention. None of these proposed classifications is activated. Most inherited
requirements have only historical operational precedent; D2 asks reviewers to
confirm or narrow them rather than asserting normative authority for every name.
`experiments/` is required for repository completeness and supported opt-in
research builds, not as a universal default-runtime dependency.

Every root entry is separately named. Unknown entries fail. No pattern permits
new root names. The generated-artifact list names exact examples; an output not
listed there still fails as unknown. Listed generated entries already present at
baseline (`Makefile` and `build.log`) stay physically preserved but unresolved;
their presence would block an activated contract until a specific authorized
exception/disposition exists. Similarly, pinning a historical directory is not
an exception for arbitrary new descendants.

## Committed-tree evaluation

Proposed evaluation must use a single resolved commit/tree throughout, defaulting
to HEAD, and read **both contract bytes and root entries from that tree**. Using
a working-tree contract with a committed root would let local edits change CI
meaning. Resolve once, enumerate using `git ls-tree -z`, and parse Git records
without stripping path whitespace. Reject malformed/duplicate records and Git
errors. Sort diagnostics by exact path for deterministic output.

Required directory means tree mode `040000`, object type `tree`. Required file
means regular blob mode `100644` or `100755` as explicitly approved for the entry;
the observed mode in the proposal is the candidate expected mode. Executable-bit
changes require explicit review where exact mode is specified. Reject mode
`120000` (symlink), mode `160000` (gitlink replacing a root directory), any other
unexpected mode/type, and file/directory substitutions. Do not follow symlinks.
This concerns root entry identity; recursive source-symlink policy is separate.

For required entries, `baseline_git.oid` is historical evidence only: regular
source changes must not require new root-layout authorization or updating that
observation. Only mode/type and required presence are constrained there. Exact
content-object enforcement applies to separately authorized historical
preservation exceptions, whose purpose is to hold unresolved bytes unchanged.

Optional entries may be absent, but require approved exact type/mode if present.
Grandfathered entries, if later authorized, require presence and exact pinned
mode/object ID. Tree IDs pin every descendant. They are reported as preservation
exceptions, not ordinary approved entries. Any byte/mode/path change requires a
new disposition; absent historical entries also fail until separately authorized.
Unresolved entries always produce `disposition required: PATH`, even though no
filesystem mutation occurs. Generated entries without an authorized preservation
exception produce `prohibited generated artifact: PATH`. A reviewed exception
must name the exact denial it overrides and cannot supersede unknown-entry or
type/symlink rejection. Duplicate paths across required/optional/grandfathered/
unresolved categories fail schema semantics. Prohibition may overlap unresolved
because it records an additional denial, not permission.

## CMake consistency

Directory allowlisting alone must not become evidence of build consistency.
Use the committed CMake graph and the existing architecture-target discovery
semantics as the starting point for a narrow consistency check. Verify root
`add_subdirectory(src)`, its delegated Direct API target's test source, and each
supported opt-in research path against required directory declarations. Reject
missing required classifications even if the path happens to exist. Evidence
references in JSON must not substitute for actually reading the committed CMake
inputs. Computed/unresolved graph paths must diagnose an unsupported construct
rather than guess or silently skip. A future implementation should reuse existing
parsing only if source-file edges are handled correctly; target-name discovery
alone is insufficient. No new general CMake interpreter is proposed here.

This is a specified acceptance criterion, not an implemented check. Reducing it
to a one-time three-name assertion would not detect subsequent build-system
drift. Default runtime, opt-in research and repository-completeness requirements
must remain distinguishable in diagnostics.

## Contract changes cannot authorize themselves

Syntactically valid JSON, existing Git inclusion, or a digest embedded in the
same candidate branch does not prove maintainer approval. Before root evaluation,
the future CI check must verify the exact contract **and checker** against an
independently trusted approved revision/digest or attestation (D5). Missing trust,
changed checker/contract, or mismatched evidence fails with `unapproved contract
revision`. Unknown schema versions, duplicate JSON keys, malformed paths, unknown
fields, duplicate names, false approval records and unauthorized exception
activation must also fail. The trusted expected value cannot be redefined by the
candidate contract, candidate workflow, or a candidate-local approval file.

How maintainers establish and protect that independent trust is a required
decision. Existing branch protection is not altered here, and this proposal does
not claim that in-repository code alone can prevent an attacker who can also
replace the code that evaluates it. Local diagnostics without a trust source
must explicitly lack authorization status and cannot claim the CI gate passed.

## Optional working-tree diagnostics

An explicit future `--working-tree` mode could compare physical root entries to
the **approved committed** contract, using `lstat` to reject symlinks and separately
reporting untracked additions/missing paths. Its result must say `working-tree
diagnostic only` and never replace the committed result. Git's administrative
`.git` file/directory is the sole structural administrative exclusion, not a
general hidden-file exception. No working-tree mode is added in this PR.

## Verification boundary

The JSON schema checks shape, not human approval, CMake execution, contract
digest trust or cross-record invariants. The accompanying static verification
records exact coverage/type/object agreement with baseline observations; that
proves the proposal is reviewable, not that baseline is approved. No unapproved
policy code is committed under `scripts/` or `tests/ci/`.
