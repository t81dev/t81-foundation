# PR #767 surviving validator checks on current main

Prepared an isolated **local candidate**, branch `candidate/pr767-main-validator`,
worktree `/private/tmp/t81-pr767-main-validator`, from main
`c2ab68e582c9f773ce94a9e13bcb805016f07dd4`. No merge, push, PR mutation,
original-branch rewrite or historical evidence edit was performed.

This is validator/CI maintenance on an existing surface. It is engineering
verification by the Codex assistant implementing this candidate, using the prior
#766/#767 reconciliation as context; it is not independent scientific acceptance.
No Gate 2, experiment authorization or historical generating-run claim changes.

| Inspected identity | Commit |
|---|---|
| Current main, fetched and finally verified remotely | `c2ab68e582c9f773ce94a9e13bcb805016f07dd4` |
| Original #767, still open/draft when inspected | `bd77c00122ff9cc63097d0ee2cd88895266878cc` |
| Original #766 / #767 sole parent | `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766` |
| Newer main provenance/schema correction | `1fe9044cb497484ecf5f89d4f1bc7581b141eae5` |

## Disposition of the old changes

| Check or inherited change | Current main assessment | Candidate disposition |
|---|---|---|
| Impossible dates, nonzero UTC offsets, timestamp terminal newline | Missing: shape regex alone admits these | Carry calendar parsing, zero-offset requirement and full timestamp match |
| NaN/Infinity in declared metrics | Missing: numeric/nonnegative checks admit these | Require finite declared floats |
| Nonstandard JSON constants and overflow, including uninterpreted extras | Missing: default Python JSON loader admits these | Strict file loading rejects NaN/Infinity/-Infinity and float overflow; finite extras remain allowed |
| Recursive supplied-schema constraints, enums, patterns, additionalProperties | Missing as a schema interpreter; many current nested field types are already covered by domain checks | Add bounded recursive checks and preflight unsupported/malformed constraints, including absent optional properties |
| SHA full-string matching; publication null/non-string rejection | Already implemented on main | Preserve, with file regression controls; no duplicate SHA correction |
| Strict scalar/counter ints, nonnegative values, weighted totals, nested malformed counters | Already implemented on main | Preserve main implementation and 94 negative fixtures; do not portray inherited rejection cases as missing fixes |
| Unexpected exceptions / wrong exit codes counted as rejection success | Already corrected on main | Preserve exact exit-1 handling and propagation; exercise it separately |
| Original #766 report/schema/evidence changes inherited by #767 | Superseded by main's provenance repair and schema minima/length constraints | Exclude entirely; do not cherry-pick the stack or restore the old report/evidence |
| Broad schema-conformance success wording | Still too broad for this bounded implementation | State “passes supported schema checks and domain rules” |
| File-based CLI tests | Not registered on main | Add normal and optimized CTests alongside inherited negative tests |

The original #767 has only one changed file relative to #766, but four files
relative to its historical main. Those inherited report/schema/evidence changes
are not a current-main patch. An actual unadapted #767 CLI run against current
main's evidence/schema exits 1 on unsupported `minLength`/`maxLength`; merely
copying its keyword whitelist would break today's valid record. Its type checker
also lacks current nullable type unions and `minimum` support.

The candidate therefore starts from main and adapts the surviving checker to
`minimum`, `minLength`, `maxLength`, and type unions including `null`. The schema
and evidence remain byte-identical to main, as does `GATE1_IMPLEMENTATION_REPORT.md`.
`evidence_generation_sha` remains **required**, with null meaning **UNVERIFIED**;
a valid SHA is shape-admissible, not independently authenticated provenance.
Publication SHA remains optional but must be a non-null SHA when supplied.
Baseline/evaluated implementation SHAs remain mandatory, non-null full SHAs.

## Executed verification

Environment: macOS 26.5.2 ARM64, Python 3.14.3, Apple Clang 17.0.0; CMake/Ninja
Release build, `T81_BUILD_TESTS=ON`, `T81_BUILD_EXPERIMENTS=ON`. Exact configure
flags, commands, timestamps, stdout/stderr, exits and source hashes are retained
in [validation.json](validation.json).

- Target `t81_exp_scg08_01_test` builds successfully in Release. The arithmetic
  executable was **not run**; this is a build check, not a new generating run.
- Registered validator CTests pass **5/5**: committed-record positive validation,
  inherited negative validation, JSON-file validation, optimized negative
  validation, optimized JSON-file validation.
- Inherited negative suite: **94 rejections** and positive publication controls,
  normal and Python `-O`. Optional `jsonschema` parity was explicitly **not run**
  because that dependency is unavailable in the selected interpreter.
- New file suite: **57 cases (49 rejection / 8 positive)**, normal and `-O`,
  using actual temporary evidence/schema files and child CLI processes. Rejection
  requires exit 1 and the intended validator error marker, without traceback or
  success output. Child interpreters inherit optimization. No Python `assert`
  or C++ `assert` is used to decide these results.
- [paired-probes.json](paired-probes.json): **20 paired main/candidate probes**
  under `-O`, retaining payloads, schema mutations, commands and raw results.
  They distinguish existing SHA/counter/provenance rejections from the surviving
  calendar/nonfinite/schema gaps. Includes the unadapted #767 failure above.
- [test-guards.json](test-guards.json): removal of strict JSON loading makes the
  new suite fail on extra-field NaN in normal and optimized modes. An unexpected
  implementation exception propagates from the inherited negative runner.
- `python3 -m tabnanny scripts/ci/validate_scg08_gate1_evidence.py` and
  `git diff --check` pass.

Reproduce the registered checks after configuring with the receipt flags:

```sh
cmake --build /private/tmp/t81-pr767-main-release --target t81_exp_scg08_01_test -j 4
ctest --test-dir /private/tmp/t81-pr767-main-release --output-on-failure -V -R '^t81_exp_scg08_01_schema_'
```

During adaptation, the old unsupported-keyword fixture used `minimum`, which
is now supported: the new file suite correctly failed because that fixture
unexpectedly validated. It was changed to unsupported `maximum`; all final
registered checks then passed. A preset-listing attempt, `cmake --preset help`,
also exited 1 (no such preset); the actual explicit Release configure passed.
These were development/tooling failures, not evidence of scientific outcomes.

## Boundaries

This remains a bounded schema/domain validator, not universal Draft 7,
meta-schema or ECMAScript regex compliance. Patterns use Python regex semantics
for the current ASCII-compatible patterns. Integral floats remain deliberately
rejected for integer fields, unlike Draft 7's mathematical-integer admission.
The finite-float loader imposes a machine-float range restriction on JSON numbers.
Calendar admission follows the tested Python interpreter; cross-version ISO
admission equivalence is not established.

Only macOS ARM64 execution is claimed. No Linux/Windows hosted CI, other Python
version, full repository test suite, benchmark, arithmetic smoke or Gate 2 run
was performed. Changes use Python stdlib and CMake interpreter discovery without
new OS-specific runtime code, but that is source inspection, not cross-platform
execution. No new PR was opened.

The original shared checkout's modified/untracked files and prior audit records
were left in place. Final remote branch tips match the pins above. Only the new
candidate contains source, CMake registration and this append-only assessment.
