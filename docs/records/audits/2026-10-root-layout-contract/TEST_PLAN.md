# Proposed root-validator regression acceptance cases

Status: **DESIGN ONLY — NOT EXECUTED AGAINST A NEW IMPLEMENTATION**.
The authority boundary prevents implementation, so
`tests/ci/check_root_structure_test.py` is deliberately not created. Do not count
this test plan as a passing test suite. Existing regressions were executed and
are recorded separately in `regressions.json`.

After approval, use temporary Git repositories with explicit fixture contracts
and independently supplied test trust anchors. The valid fixture must be designed
from approved minimum requirements, not copied from the current root listing.
Synthetic fixture manifests are not production placeholder manifests. Resolve
the real checker's repository from a test argument or fixture placement; no
mocked `git ls-tree` output should replace committed Git-state tests.

| Case | Intentional fixture state | Required result / meaningful diagnostic |
| --- | --- | --- |
| 1. Valid minimal layout | Commit independently approved required files/directories; omit optional and historical entries | Exit 0; `committed root structure PASSED`, resolved commit, required count; no blanket exception |
| 2. Missing required directory | Remove and commit a required tree (include a tracked fixture file so Git records directories) | Nonzero; `missing required directory: src` |
| 3. Missing required file | Remove and commit `CMakeLists.txt` | Nonzero; `missing required file: CMakeLists.txt` |
| 4. Unexpected root file | Add and commit `unauthorized.txt` | Nonzero; `unknown root entry: unauthorized.txt` |
| 5. Unexpected root directory | Add/commit `unauthorized/fixture.txt` | Nonzero; `unknown root entry: unauthorized` |
| 6. File replacing directory | Commit regular blob named `src` | Nonzero; `expected directory: src; found regular file` |
| 7. Directory replacing file | Commit `CMakeLists.txt/fixture.txt` | Nonzero; `expected regular file: CMakeLists.txt; found directory` |
| 8. Symlink substitution | Commit mode 120000 for expected directory and file separately | Nonzero; `symlink forbidden: PATH`; do not dereference target |
| 9. Unapproved generated output | Commit root `CMakeCache.txt`, then separately add `build.log` without a signed/trusted exception | Nonzero; `prohibited generated artifact: PATH` |
| 10. Active CMake root omitted | Authorize a fixture contract deliberately omitting `development_tests` while committed reachable CMake references its source | Nonzero; `CMake input absent from required contract: development_tests`; existence alone cannot satisfy classification |
| 11. HEAD versus worktree | Start from valid committed tree, locally add an unknown root file or remove a required file without committing | Default evaluates unchanged committed tree and exits 0, explicitly labels HEAD; optional worktree diagnostic fails. Then commit the same violation: default must fail. Conversely a dirty repair cannot make an invalid HEAD pass |
| 12. Historical preservation | Put a historical blob/tree physically in HEAD without approval, then authorize only its exact mode/object under separate fixture trust | Unapproved case nonzero `disposition required`; authorized pinned preservation case exits 0 with distinct exception diagnostic. Added descendant, changed bytes, absence, changed type, or symlink each fails without granting replacement authority |
| 13. Contract tampering | Edit permit names, authority fields, exceptions, schema version or checker bytes without changing independently supplied trust | Nonzero; `unapproved contract revision`; updating a candidate-local digest must not help |
| 14. Invalid contract syntax/semantics | Duplicate JSON key, duplicate/cross-category path, traversal/separator, malformed mode/object, unknown field or unknown schema version | Nonzero; path-specific `invalid root contract` diagnostic; no last-key-wins or permissive fallback |
| 15. Optional entry typing | Separately omit, include correctly, and substitute wrong type/symlink for an explicitly approved optional file/directory | Absent and correct forms pass; substitutions fail with meaningful path/type diagnostic |
| 16. Git-state failure | Missing repository, invalid commit, or failed object read | Nonzero; Git/commit diagnostic; never fall back to the physical root |

Commit symlinks with Git plumbing mode `120000` when local Windows permissions
cannot create physical symlinks; this keeps committed-tree tests portable.
Working-tree symlink tests require actual `lstat` capability and must report
platform skips explicitly. Include POSIX executable mode, Windows Git mode
handling, Unicode/whitespace filenames and root gitlink substitution. Python/Git
tree parsing is architecture independent, but no Linux, Windows, ARM64 or hosted
execution of a new validator is claimed now. CI remains fail closed on unsupported
features instead of silently accepting them.
