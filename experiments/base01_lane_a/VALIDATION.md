# Lane A validation and readiness — November 2, 2026

**B0: validated on macOS ARM64. B1: blocked / not authorized.**
No performance timing, warm-up campaign, benchmark trials or PMU collection was
executed. `--measure` exits 2 in Python and native entry points. The interval
primitive is uncalled in B0; full campaign activation requires a later reviewed
patch after explicit authorization. This packet contains no performance results.

Audited source: `cda9135d36144d4de2771af174e52ac7b30a83c9`.
Phase A manifest: PR #687 at `ecd66137bacbce50e605cfc1466ec7d36131ba9e`.
The final receipt identifies the implementation commit and file hashes; the PR
head adds retained evidence. No runtime or frozen research artifact was modified.
No Lane B implementation was consulted. This is not independent review of
Codex's own runner and does not establish comparative or hardware advantage.

Actual commands are those in README.md with these session paths:

- source `/private/tmp/t81-base01-audited`
- source build `/private/tmp/t81-base01-source-build`
- runner checkout `/private/tmp/t81-base01-lane-a`
- native build `/private/tmp/t81-base01-native-build`
- final output `/private/tmp/t81-base01-final-validation`

Both builds used `/usr/bin/c++`, AppleClang 21, Release/C++23 with LTO disabled.
The normal audited CMake build fetched Asio `asio-1-30-2`, resolved to
`12e0ce9e0500bf0f247dbd1ae894272656456079`. Benchmarks, TUI, examples,
fuzz tests, TernaryOS and DPE were disabled. Full commands/flags and host data are
retained in the receipt, CMake caches and compile-command artifacts.

Checks performed:

- `t81_ternary_arith_test`: passed, including signed extrema and saturation cases.
- `t81_vm_workload_determinism_tiers_test`: passed, all six tiers; raw signatures retained.
- `t81_canonfs_driver_test`: passed, identity/read-back/capability checks.
- `test_resource_monitoring`: passed; this checks VM resource policies, not rusage.
- Lane A native validator: exact 1,031-input codec corpus, all six VM tiers and
  all three CanonFS sizes; passed. Python positional oracle independently
  validates encoded integer values. VM checks include tags/float-handle payloads.
- Seven Python contract tests: passed. RSS Linux/macOS units, statistical
  definitions on explicitly synthetic mathematical fixtures, unconditional
  measurement rejection before work, ineffective environment override,
  unsupported/frozen flags, wrong revisions/dirty source, wrong codec corpus.
- Commons offline discovery: succeeded after isolated dependency setup.
  `proposal.json` passed shape/reference validation. That grants no execution rights.
- `git diff --check`: passed before committing.

Observed VM instruction accounting, not performance:

| Exact builder | Trace entries | R75 ticks |
| --- | ---: | ---: |
| workload | 163 | 406 |
| micro | 5 | 5 |
| meso | 157 | 250 |
| mixed | 11 | 11 |
| policy-heavy | 7 | 7 |
| tensor-access | 8 | 8 |

Source inspection explains the distinction: `vm/vm.cpp` includes hot-trace
compilation even through `make_interpreter_vm`; `vm/runtime_state_helpers.cpp`
exposes `instruction_count` at R75. Preserve this source behavior. Do not use
trace length as an instruction denominator or call 5,000 a fixed executed count.
The original workload writes R0, which the VM resets as a system register; the
exact test program is preserved rather than repaired under this assignment.

Failed/inconclusive checks are retained, not rewritten as passes:

1. The GitHub connector transport failed; sandboxed `gh` networking also failed.
   An authorized network-capable `gh pr view 687` retrieved Phase A metadata;
   manifest bytes were read from the already-local Git object. No Lane B was read.
2. Ordinary worktree creation was denied by the filesystem sandbox. The same
   isolated-worktree operation succeeded with the managed escalation.
3. Initial CMake configure could not resolve github.com for pinned Asio. The
   network-enabled configure succeeded; no audited-source correction was needed.
4. An initial native configure globbed archives before the audited build finished,
   so linking lacked the VM archive. Reconfiguring after all four targets built
   resolved it. README explicitly requires source build completion first.
5. Development attempt 01 failed an incorrectly written Lane A semantic check:
   tensor-access R44 is a FloatHandle, not integer 9. Source inspection corrected
   the runner to inspect tag and float pool. The original native output is retained.
6. Attempt 02 passed workload checks but failed receipt creation because CMake
   stored the explicitly selected compiler as STRING instead of FILEPATH. The
   parser now handles either cache type.
7. Attempt 03 passed workload checks but failed metadata collection because the
   sandbox denied sysctl. The collector now retains each key's error, and the final
   validation is run with permission for read-only host inspection.
8. Attempts 04/05 were precommit validation passes. Their receipts identify the
   then-current checkout HEAD and per-file hashes, not a committed implementation.
   Use the final receipt for the committed implementation.
9. Stale temporary Python environments lacked jsonschema; one pip attempt hit
   externally-managed-environment protection and changed no system packages.
   A fresh temporary venv installed jsonschema 4.26.0 for Commons checks only.

Development raw records are archived byte-for-byte in
`evidence/development-attempts.tar.gz` with a per-file SHA-256 index. Extract to a
scratch directory with `tar -xzf`; archive content is not an executable recipe.
Build/setup logs remain plain text. Final raw validation is under `evidence/final/`.
Hashes establish content identity, not scientific acceptance.

Readiness limits/blockers:

- Explicit subsequent maintainer B1 authorization is absent; measurement stays disabled.
- The complete 30-trial fresh-process dispatcher, VM batch/timer-resolution
  qualification and resource-child collection need a reviewed activation patch;
  only interval/batch primitives and a detailed measurement contract are prepared.
- macOS affinity is unsupported; CPU isolation, frequency locking and boost control
  were not applied. This shared host is not B1-qualified. Host metadata is evidence
  of observed configuration, not proof of stable clocks or an idle isolated core.
- Linux observer/configuration paths were reviewed and RSS conversion tested, but
  Linux/x86_64 execution was not performed. Windows execution is unsupported.
- LLVM/Mach-O section output is retained. No cross-platform section equivalence,
  CLI artifact size, CAS serialization-size measurement, or CI pass is claimed.
- Existing ld64 duplicate-static-library warnings are retained. They did not
  prevent these builds/tests; no source-level portability fix is inferred.

Stop here: draft PR only, no merge, no benchmark execution, no governance change.
