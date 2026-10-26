# Architecture-target governance investigation

## Integration state and baseline

Current main was fetched at `01c85019319fd020922402ba356878e392cb2231`.
PR [#574](https://github.com/t81dev/t81-foundation/pull/574) remains open and
unmerged at `10df367102151b087ee64b1fbb737c471f4c87fb`. Investigation and the
separate corrective branch use that exact head. No #574 commit or setting is
changed. Its C++ CodeQL analysis and aggregate CodeQL result are **SUCCESS**:
[analysis job](https://github.com/t81dev/t81-foundation/actions/runs/40261760738/job/121375453375).
GitHub reports zero submitted reviews and zero inline review threads at the
captured inspection. The Jules APPROVE FOR MERGE recommendation supplied by the
user is recorded as that external recommendation, not a GitHub review submission.

The original command was reproduced both locally and in the retrieved
[Ubuntu CI job](https://github.com/t81dev/t81-foundation/actions/runs/40261760705/job/121375453720):

```sh
python3 scripts/ci/check_architecture_targets.py
```

Exit **1**, complete stdout:

```text
ARCHITECTURE.md references targets not declared in CMake:
  - t81
  - t81_c_api
  - t81_core
  - t81_io
  - t81_isa
  - t81_lang_frontend
  - t81_llvm
  - t81_python
  - t81_tool_cli
  - t81_vm
```

There is no stderr for this command. Hosted CI runs Python 3.12 on Ubuntu and
uses explicit Bash/pipefail; the failing producer and failing step are visible.
Fetched PR merge-test and reviewed-head Git trees are identical:
`8bc15b6cef921e0b27f011702bdcbbb61e2caa7a`. Thus the hosted failure is attributable
to the inspected source, not a different merge result. Local reproduction uses
Python 3.14.3 on macOS ARM64. This validator reads text; it does not configure or
build a platform-specific CMake graph.

## Justified contract and independent root causes

The narrow existing contract is: **each target name in the architecture target
table must have a literal declaration in the project's CMake source hierarchy**.
Optional/platform branches remain part of the declaration inventory. This is
not an assertion that every target is enabled in one configuration, nor a
certificate of target kind, linkage, runtime behavior or scientific advantage.

The architecture overview calls CMake the supported build authority. Root
CMake delegates to `src/CMakeLists.txt`, which delegates to component files.
The original validator instead reads only root and benchmarks CMakeLists.txt.
Its 421 discovered names are mostly other targets; it never inspects the nine
component declarations below. No missing production component is established
by these nine errors.

| Failed old expectation | Actual declaration at baseline | Classification |
| --- | --- | --- |
| t81 | src/tools/CMakeLists.txt, add_executable | Validator discovery defect |
| t81_c_api | src/ffi/CMakeLists.txt, add_library | Validator discovery defect |
| t81_core | src/core/CMakeLists.txt, add_library | Validator discovery defect |
| t81_io | src/io/CMakeLists.txt, add_library | Validator discovery defect |
| t81_isa | src/isa/CMakeLists.txt, add_library | Validator discovery defect |
| t81_lang_frontend | src/lang/CMakeLists.txt, add_library | Validator discovery defect |
| t81_llvm | src/lang/CMakeLists.txt, STATIC/INTERFACE branches | Validator discovery defect |
| t81_tool_cli | src/tools/CMakeLists.txt, add_library | Validator discovery defect |
| t81_vm | src/vm/CMakeLists.txt, add_library | Validator discovery defect |
| t81_python | Root declares _t81 instead | Obsolete documented target name |

Two commit-grounded decisions establish the distinction:

- `c36ccaff98cee8e36f246b147e2dab94dde1cd60`, “Refactor CMake layout without
  changing build behavior,” moved the component declarations to `src/` while
  retaining their names and preserving root output locations. The validator's
  fixed two-file search was not updated. Moving targets back into one file or
  recreating components is not justified.
- `028cca2e994e3ef8608c5fe62722a9d09a9aea84`, “Python bindings — pip install t81,”
  explicitly renamed the target/module from t81_python to _t81. Current root
  `pybind11_add_module(_t81 ...)` and `install(TARGETS _t81 ... DESTINATION t81)`
  agree with `PYBIND11_MODULE(_t81, m)` in lang/python/t81_python.cpp,
  `from t81._t81 import ...` in ffi/python/__init__.py, and pyproject.toml's wheel
  configuration and `import t81` smoke command. Updating the table's target name
  is supported by this deliberate packaging contract, not merely current code.

A third independent coverage defect was discovered: the old row regex silently
ignores `benchmark_runner` because its cell has the `(subdir)` annotation. The
table contains 11 intended targets, but the old parser extracts only 10. The
corrected parser includes that row and still requires its declaration in
benchmarks/CMakeLists.txt. Optional does not mean exempt.

Independent combinations confirm the two causes of the original exit failure:

| Discovery / table | Missing targets |
| --- | --- |
| Old discovery, old table | Ten listed above |
| Corrected discovery, old table | t81_python only |
| Old discovery, corrected table | Nine component names |
| Corrected discovery, corrected table | None; all 11 checked |

The repaired source inventory reads 19 CMakeLists.txt files and finds 490
literal declared names. Full old/new inventories, all file edges and each
documented name's declaration provenance are in [inventory.json](inventory.json).
These counts are source inventories, not enabled/configured-target counts.

## Authority, limits and conflicting historical metadata

TISC opcode semantics/freeze requirements in spec/tisc-spec.md and the canonical
registry, T81VM semantics in spec/t81vm-spec.md, numerical contracts, and accepted
RFC-00B8 governance/FFI obligations remain authoritative on their respective
surfaces. They do not require those CMake declarations to be textually in the
root file or require the obsolete t81_python module name. No ISA requirement,
governance boundary, implementation target, public alias or production build
definition is changed here. The frozen check still verifies 209 opcodes.

Commit `6ba11a0c7405def8aca70c5395a6e3da7211cf5d` previously corrected this
validator's architecture-document location and removed the obsolete t81_cog
expectation. Commit `c087907d8f1158f330a0124248500f180793cd7c` restored that checker
after a deletion. Those records support maintaining source-grounded expectations
rather than recreating old component names to satisfy stale metadata.

Some legacy tests/python files still import t81_python, and the architecture
table's VM INTERFACE/responsibility text is older than its current STATIC target.
These inconsistencies are recorded, not silently certified or repaired. They
do not override the explicit packaging rename or establish that old import
compatibility is currently promised. Python API compatibility, target-kind and
dependency claims need separate review if pursued. This change checks names only.

The inventory follows literal project-local add_subdirectory edges and includes
conditional branches; it never scans arbitrary backup/build/dependency trees.
Comments, message strings, and unexecuted function/macro definitions cannot
supply fake target declarations. It does not evaluate CMake variables, include
modules, target factories, platform predicates, or generator expressions.
Computed declarations therefore cannot satisfy a documented name by guesswork;
such a future refactor would require explicit review or configured-build evidence.
This is deliberately not a general CMake interpreter or a configuration validator.
The checked project declarations all fit its supported literal form. Missing
reachable files, repository escapes, malformed commands, and empty expectation
sets fail rather than yielding a vacuous pass.

## Corrective options

| Option | Evidence / risk | Decision |
| --- | --- | --- |
| Restore root target declarations or add a legacy Python target | Reverses deliberate layout/packaging decisions; risks duplicate targets or misleading import compatibility | Reject; no production repair needed for these errors |
| Remove missing names or exempt optional targets | Makes violations invisible | Reject; weakens the name-presence constraint |
| Recursively scan every CMakeLists.txt on disk | A disconnected old/generated copy can hide a missing active declaration | Reject; negative regression covers this |
| Configure every optional/native backend inside the lint step | Needs toolchains/dependencies; one configuration omits legitimate optional targets; changes the gate's meaning | Not the minimal declaration-check repair |
| Follow literal local source edges and correct the documented Python name | Matches the refactor and packaging records; preserves all intended names and includes the omitted benchmark row | Implement on separate corrective branch |

The CI step runs focused regressions before the unchanged validator command,
both logged through tee with #574's pipefail behavior. No failure suppression,
policy relaxation, dependency version change, runtime change, or other governance
repair is included.

## Regression and existing-check evidence

Local commands:

```sh
python3 tests/ci/check_architecture_targets_test.py
python3 scripts/ci/check_architecture_targets.py
python3 scripts/ci/test_failure_propagation.py --output-dir /tmp/t81-architecture-ci-controls-new
python3 scripts/ci/check_tisc_freeze_integrity.py
```

- Focused CLI regression suite: **18/18 pass**, including positive nested/quoted
  paths, optional pybind declarations and annotated benchmark rows.
- Negative controls reject deleted/renamed targets, removed source edges, stray
  build/backup copies, commented or quoted examples, uninvoked functions/macros,
  unsupported computed names/paths, missing files/docs, empty expectations,
  outside-root edges and malformed commands. These are actual checker CLI runs
  against isolated source fixtures, not copied comparison arithmetic.
- Corrected original command: **exit 0**, `architecture target sync: ok
  (11 documented targets, 19 reachable CMakeLists.txt files)`.
- Existing failure-propagation controls: **47/47 pass**.
- Existing frozen TISC check: **pass**, 209 opcodes, bitwise/reserved checks intact.
- Whitespace/scope checks pass. No production compilation, Python wheel execution
  or nonlocal runtime/platform conformance result is claimed.

The original failing command was executed in Ubuntu CI; the corrective branch's
hosted regression/validator results must be read at its own run/head. No local
macOS result is relabeled as a Linux or Windows execution.

## Relationships, remaining blockers and next action

`check_architecture_coherence.py` independently exits 1 with FileNotFoundError
for core/vm/vm.cpp. It also encodes other path/build assumptions. This shares
historical layout drift but checks different ISA/VM/Axion/trace properties;
changing its paths or regexes without tracing those contracts would be a separate
repair. It is intentionally untouched. Spec/code mapping and other governance
failures remain separately recorded in #574's audit.

Current related heads inspected without importing fixes:

- #536 `6555f0b1636f18baa77e8141118fec47d0cbdc47`: overlaps src/CMakeLists.txt and
  src/direct_api/CMakeLists.txt; it is not required for these already-existing
  component declarations. Its separate runtime/security concerns remain outside.
- #556 `db09eb984fbd9cc3eea3740b891803575ae7277a`: adds CMake portability targets;
  it does not repair the validator's source discovery or Python expectation.
- #560 `056f0d6307f59049ef9dfa42c482f727a02c2b96`: experiment-only changes, no
  direct architecture-validator overlap.
- #572 `a99fbe5ef8a89671e68b3963973f076855b101ec`: research changes, no direct
  overlap with these validator/architecture-table files. It remains unchanged.

Recommendation: review/integrate #574 separately under the maintainer's decision;
review this bounded follow-up as a **draft stacked on #574's exact branch head**.
If #574 merges, retarget the follow-up to the resulting main and recheck the
exact diff/checks. Do not merge/rebase existing PRs as part of this investigation.
Next governance investigation, if authorized, should trace coherence's stale
paths and each remaining invariant independently. No claim of overall green CI,
fully corrected architecture documentation, or production readiness is made.

[evidence.tar.gz](evidence.tar.gz) preserves complete baseline/after diagnostics,
hosted baseline metadata/logs, source inventories, independent cause comparisons,
related-head snapshots, regression/freeze/control logs and a hash manifest.
No research experiment, OS primitive, ternary-native architecture, scientific
conclusion or canonical H01–H08 entry is modified.
