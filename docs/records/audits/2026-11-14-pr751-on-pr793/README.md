# PR #751 wrapper hardening stacked on #793

Draft replacement; dependent on #793. No merge, policy expansion, governance
activation or scientific-gate advancement. Original #751 remains unchanged.

## Pins and review scope

- Base: #793 `15b6b2b63fe056d68f959d658a351eba603ce6ea`, branch
  `candidate/pr750-main-explicit-policy`.
- Original #751: `b68853335264d0758124a359f705fb5fbb8dfdc3`, branch
  `fix/skill-policy-boundary-reconciliation`.
- Original #751 parent/#750: `1522f14aa1994d03af72441f2554718c9cfc11cb`.
- New implementation commit: `24596a28d5aebe75ebb86edc1f4a97b377953538`, branch
  `candidate/pr751-on-pr793-wrapper`; following audit commit changes only this
  evidence package. This is a four-file delta against #793, not an import of the
  original #747/#750 stack onto main.

Prepared and freshly tested by Codex within the shared maintenance effort that
contributed earlier corrections. This is author execution and source inspection,
not independent technical approval or blanket security acceptance. Historical
#614/#751 reports remain historical; new receipts are identified below.

## Surviving bounded delta

Three files are byte-identical to original #751:

- `skills/policy-checker/validate-policy.sh`: fixed `cmd=(t81 policy validate
  "$policy_file" --json)` and `exec "${cmd[@]}"`, with no shell command string or
  delegation to check.sh. Wrapper grammar allows one `.apl`/`.axionb` path,
  optional single --json, and -- for a literal option-shaped filename. Unsupported
  operations/options, duplicate/extra/empty inputs and non-readable/non-regular
  inputs reject before the only backend exec. Relative paths gain ./ to protect
  against the native CLI's option parsing; path text is otherwise preserved.
- `skills/policy-checker/SKILL.md`: narrows the advertised operation to native
  syntax/metadata validation and states its trust assumptions and limits.
- `tests/python/test_skill_policy_boundary.py`: keeps fake-backend argv checks
  and real-CLI validation separately labeled; inherited HOME is preserved.

The fourth file, `CMakeLists.txt`, adds exactly #751's ten registration lines at
the current location. It preserves #793's 180-second policy-test timeout and
#782's repro working directory. The UNIX-only test requires Bash and uses the
real built target. No workflow, CLI, VM, evaluator, policy model, arithmetic,
process capture, existing fixture or prior audit/evidence bytes are changed.
See source-comparison.json and the complete original/candidate patches.

The dependency is explicit: retain #793 as the base during review. Proposed order
is #793 first, then refresh/retarget this wrapper PR to main and obtain applicable
hosted checks. Neither PR is merged or independently approved by this packet.

## Fresh Release execution

Native macOS ARM64, /bin/bash 3.2, AppleClang 21, CMake 4.3, C++23,
Release / -O3 -DNDEBUG, strict deterministic float ON. A fresh isolated CLI build
was made from this candidate. No C++ source differs from #793; the new executable
and test identities are retained, not inferred from old author reports.

| Check | Original wrapper/dispatcher on same fresh CLI | Corrected wrapper |
| --- | --- | --- |
| Identical matrix | 86 process receipts, 141 failed checks | 86 receipts, zero failed checks |
| Fake-backend argv calls | 40 calls | 40 calls |
| Wrapper calls configured with real CLI | 44 calls | 44 calls |
| Missing-backend probe | 1 call | 1 call |
| Successful compiled-policy fixture setup | 1 process | 1 process |
| Persistent fixture-tree changes | 12 cases | zero cases |

Candidate fake mode has 16 exact-argv backend responses and 24 pre-backend
rejections. Real-configured mode has 20 native validation responses (including
expected parse failures) and 24 pre-backend rejections, not 44 policy parses.

85 wrapper calls plus one fixture compilation make 86 receipts. No permission
probe was skipped: chmod-000 unreadability was effective for this local user.
The first baseline-receipt.json uses the previously built #793 executable and
also reports 141 failed checks. The baseline-fresh-release-receipt.json uses the
same fresh binary and the same harness hash as candidate-receipt.json, giving
the direct before/after comparison. Baseline scripts are copies from the pinned
#793 source, made executable so the original dispatcher remains callable.

Negative controls show the original dispatcher reaches the fake backend for
unsupported operations and creates configured store directories. Dollar-command
substitution and operation canaries execute in both fake and real modes before
backend rejection; the fake quote-breakout case also creates its canary. A real
CLI exit 1 therefore does not imply no prior wrapper side effects. These are
observed local paths, not a claim that every historical invocation is exploitable.
Exact commands, stdout/stderr, statuses and before/after hashes are retained;
canaries are confined to the temporary fixture tree and removed by the harness.

Candidate fake checks verify the exact vector [policy, validate, literal path,
--json], including spaces, single/double quotes, dollar signs, backticks, shell
metacharacters and --help.apl after --. Unsupported operations/options emit no
fake backend response; source inspection corroborates rejection before the sole
exec. Backend exit 23 propagates, and missing backend exits 127. These checks do
not prove that any policy parses or authorizes an operation.

Separately, the real CLI tests text and compiled-policy parsing, schema
`t81.policy-validate.v1`, literal input identity, native success/failure status,
malformed/empty text, invalid compiled bytes and all filename cases. A zero-budget
policy is valid metadata; it is not an execution request or an authorization
verdict. Source inspection of run_policy_validate confirms native parse/deserialize
and metadata reporting, not VM execution or object import. #793's execution-policy
matrix is separately rerun by CTest and must not be conflated with this validation.

Registered CTests passed 4/4: wrapper boundary (86 receipts), execution-policy
(249 processes), CLI repro hash and process capture. Bash syntax and source-only
whitespace checks pass. Compile-commands.log records actual -DNDEBUG commands;
the harness uses explicit checks rather than assert, and remains active in Release.
No runtime assertions or policy tests were weakened.

Exact configure/build/test commands are in candidate-commands.json. Reproduce:

```sh
python3 tests/python/test_skill_policy_boundary.py --bash /bin/bash --t81-bin /absolute/build/t81 --receipt /tmp/wrapper-candidate.json
python3 tests/python/test_skill_policy_boundary.py --bash /bin/bash --wrapper docs/records/audits/2026-11-14-pr751-on-pr793/evidence/baseline-wrapper/validate-policy.sh --t81-bin /absolute/build/t81 --receipt /tmp/wrapper-baseline.json
ctest --test-dir /absolute/build --output-on-failure -V -R '^(t81_skill_policy_boundary_test|t81_cli_execution_policy_test|t81_cli_repro_hash_test|t81_process_capture_test)$'
```

## Limits and outstanding qualification

- Symlinks to readable regular files remain allowed; broken symlinks, directories
  and FIFOs reject. File/readability checks are not descriptor binding and do not
  eliminate check-to-open races or concurrent mutation. No path canonicalization,
  workspace containment, allowlist, file-size/time cap or resource sandbox is added.
- Trusted PATH executable, host, shell and environment remain prerequisites.
  The harness removes ambient shell-startup/function hooks for controlled tests,
  not as a new wrapper guarantee. Caller-shell expansion happens before argv
  reaches the wrapper. No wrapper can undo that expansion.
- Snapshot assertions cover the temporary fixture tree, not every host write or
  transient filesystem event. No OS-level no-write guarantee, full-suite run,
  sanitizer qualification or arbitrary parser-failure completeness is claimed.
- Other wrappers (check.sh, simulate.sh, test-rule.sh, list-policies.sh) remain
  unchanged and outside the safe advertised boundary. The inherited underscore
  skill identifier is retained; this packet does not certify external skill-name
  validators or agent-integration eligibility.
- Local execution establishes macOS Bash behavior only. Linux Bash, Windows/WSL,
  and other platform behavior are not freshly executed here. The registered wrapper
  test is UNIX-only. Standard CI/CodeQL/format pull-request workflows target main;
  a PR stacked on #793 does not automatically qualify through those filters.
  Obtain applicable hosted qualification after the dependency is integrated and
  the wrapper PR is refreshed/retargeted; no workflow bypass is introduced.
- Existing #793 evidence remains byte-for-byte intact; its earlier platform
  results are not promoted into fresh wrapper coverage. Review and integration
  remain maintainer decisions.

## Packet layout and integrity

`evidence/` retains refreshed PR metadata, original/candidate source patches,
baseline wrapper/dispatcher copies, the original harness, all three raw matrix
receipts and their logs/commands, configure/build/CTest/syntax logs, compiler
commands/versions, and byte/source comparisons. SHA256SUMS covers every other
file in this packet. Hashes establish content identity, not provenance truth or
independence. Raw CTest JSON and patch context whitespace are preserved; source
checks pass but packet-wide whitespace checks can flag those raw evidence bytes.
