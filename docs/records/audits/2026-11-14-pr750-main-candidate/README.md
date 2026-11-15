# PR #750: explicit execution-policy reconciliation on refreshed main

Status: proposed replacement, draft pending review and hosted qualification. No merge,
policy-model expansion, policy activation or scientific-gate advancement.

## Pinned sources and authorship

- Refreshed main: `a3ae3b16b5afebff4a7ba3c0d2cffe8a69c11b1d` (includes #792).
- Original #750: `1522f14aa1994d03af72441f2554718c9cfc11cb`, branch
  `fix/cli-execution-policy-reconciliation`, left unchanged.
- Original stacked parent: `b18075ad6de93091f9aea30707ef5237d479c9d1` (#747,
  closed without merging). This candidate targets main and does not import that
  historical stack wholesale.
- Tested code commit: `4a93e0392b5deec5fed22dd95849c91871465344` on `candidate/pr750-main-explicit-policy`.
  The following evidence commit changes documentation only. `code-binding.json`
  binds source hashes and the locally tested executable hash.
- Prepared, reconciled and locally executed by Codex, continuing the same shared
  maintenance effort that contributed to #750 and #782. This is implementation
  evidence, not independent review or independent acceptance of those contributions.
  Original #750/#613 claims remain historical author reports; the receipts below
  are fresh executions against the specifically identified main/candidate sources.

## Bounded source changes

Five active files: `CMakeLists.txt`, `tools/cli/core/main.cpp`,
`tools/cli/core/driver.cpp`, new private `tools/cli/core/execution_policy.hpp`,
and `tests/python/test_cli_execution_policy.py`.

The parsed explicit policy is retained by the existing VM command entry point.
The private reader opens once in binary mode, rejects empty paths/directories,
checks open/read failures, validates the complete text with the existing parser
(including exception failures), and publishes it only after validation. The
existing VM loader must retain a policy before explicitly requested execution
can proceed. This does not introduce new evaluator rules or authorization powers.
Missing policy, invalid syntax, empty text and numeric parse overflow fail with
exit 1 before VM execution. Zero instruction budget retains existing SecurityFault
semantics: run/trace exit 13, count=1, limit=0, compute executed=no. Existing
inspector trap reporting remains exit 1. No-policy defaults remain unchanged.

Restoring the common policy binding also affects debugger and existing inspector
paths. The same validation is therefore applied before debug, until, step, regs,
stack, mem, state, profile and explain-trap execution. JSON error responses remain
`t81.error.v1` on stdout; these are diagnostics, not program output. Debugger valid
controls use EOF, not an interactive command/step campaign.

The original patch fails only at its CMake context; the saved patch/check log
records that conflict. Registration was reconciled alongside #782's required
repro-test working directory rather than replacing it. The expanded 249-process
CTest has a 180-second timeout. All checks are ordinary explicit Python checks,
active with the CLI compiled under Release / `-DNDEBUG`.

`782-preservation.json` verifies main.cpp byte equality outside run_vm_command
and its helper include, driver.cpp equality outside run_tisc/debug_tisc and its
include, and CMake equality after removing the four registration lines. Process
capture implementation, argument conversion/entry point, cleanup, shared tests,
repro module normalization and golden hash are unchanged. No VM, Axion parser,
policy-model, CanonFS, arithmetic, fixture or historical evidence file is edited.

## Fresh local execution

macOS ARM64, AppleClang 21, CMake 4.3, C++23 Release, strict deterministic float
ON. Real CLI freshly compiled locally; not a mock or prebuilt T81 library.
Local Asio checkout and installed GTest supplied to avoid dependency downloads;
exact configure flags and compiler commands are retained. Benchmarks, TUI,
examples, fuzz tests and TernaryOS disabled. See baseline-commands.json for the
complete configure/build invocation, compile-commands.log for actual `-O3
-DNDEBUG` flags, and candidate-commands.json for exact follow-up commands.

| Executed check | Unfixed main | Candidate |
| --- | --- | --- |
| Original #750 script, byte-identical to its pinned test | 73 processes, 138 failed checks (expected exit 1) | 73 processes, zero failed checks |
| Final expanded script, identical test hash on both binaries | 249 processes, 670 failed checks across 196 processes (expected exit 1) | 249 processes, zero failed checks |
| Registered policy, CLI repro-hash, process-capture CTests | not claimed as a baseline run | 3/3 passed; policy reran all 249 processes |

The original script includes one fixture compilation plus 72 probes; expanded
script includes one compilation plus 248 probes. Counts are processes, not
independent security properties. Multiple failed checks can arise from one
process. For example baseline vm --policy missing.apl run sentinel.tisc exits 0
and prints 47; candidate exits 1 with no program stdout. Raw receipts retain
command, working directory, status, stdout/stderr, trace content and failures.

Expanded coverage: VM run/trace and code run from source/bytecode; all three
supported policy positions; valid, missing, malformed, empty, overflowing,
trailing-invalid and directory policy inputs; zero budget; absent-policy controls;
unsupported placement; missing and empty arguments; nine additional VM actions.
Negative runs with absent and pre-existing trace paths preserve existing contents
and create no new traces. Fixture-tree hashes include child temp files and detect
unexpected output, trace and temporary-artifact changes. Successful traces are
the only allowed artifact changes. Candidate has zero artifact violations and no
rejected program sentinel output. Final baseline has 61 artifact/trace-violation
cases, retained as negative evidence, not normalized away.

Reproduce after configuring/building:

```sh
python3 tests/python/test_cli_execution_policy.py --t81-bin /absolute/build/t81 --receipt /tmp/policy-receipt.json
python3 docs/records/audits/2026-11-14-pr750-main-candidate/evidence/baseline-test.py --t81-bin /absolute/build/t81 --receipt /tmp/original-receipt.json
ctest --test-dir /absolute/build --output-on-failure -V -R '^(t81_cli_execution_policy_test|t81_cli_repro_hash_test|t81_process_capture_test)$'
```

## Preserved negative and corrective evidence

The initial expanded harness (`attempt1-matrix.py`) produced 668 baseline failed
checks. Candidate attempt 1 produced 118 failed checks: directory reads were
classified as parse errors and the test wrongly rejected intentional JSON error
stdout. Attempt 2 produced 30 failed checks: directory diagnostic classification
and one missing trace-output argument in the empty-policy test. Their raw logs,
receipts, reader and test snapshots are retained. Checked stream reads alone did
not distinguish directories on this macOS stream implementation; explicit
directory rejection fixed the input classification. The final harness checks the
existing JSON error object strictly, and gives vm trace its required output path.
The final baseline and candidate comparison uses the same corrected harness hash.
Nothing in these failed runs is relabeled as passing verification.

## Evidence and limits

`evidence/` retains metadata, original/candidate patches, build and CTest logs,
source/binary identities, original/final/intermediate matrix receipts and snapshots,
and preservation comparisons. SHA256SUMS covers every other file in this packet;
content hashes establish identity, not independent provenance authentication.
Source-only whitespace checks pass. Packet-wide `git diff --check` reports raw
CTest JSON and patch-context whitespace; those evidence bytes are preserved, not
normalized. See packet-whitespace-check.json.

- Native execution here is macOS ARM64 only. Linux and Windows MSVC/clang-cl
  builds/tests and current-head required CI are pending at publication. Source
  uses portable standard filesystem/stream operations; portability is not proved
  by macOS execution. #782-specific Windows behavior is preserved by source
  comparisons; this packet does not independently requalify it on Windows.
- Directory rejection executes locally; checked handling of partial stream I/O
  failure is source-inspected, without a deterministic injected mid-read fault.
  Directory open/read behavior varies by platform. No privileged permission-denial
  simulation, sanitizer campaign or complete repository suite is claimed.
- Artifact assertions cover the isolated fixture and child-temp tree, not the
  entire host. Inherited T81_CANONFS_ROOT is removed; configured external stores,
  constructor recovery/bootstrap, adversarial concurrent mutation, blocked special
  files, and resource-exhaustion behavior are outside this test scope.
- Source compilation/setup and program loading can precede policy rejection;
  this change promises rejection before execution, not before every possible CLI
  setup side effect. It adds no policy-file size/time cap or new filesystem model.
- REPL, determinism, memory-stats, batch and other command families are not a
  universal CLI hardening claim. Existing embedded/no-policy behavior, public
  APIs and Axion semantics remain untouched. Approval and integration remain
  maintainer decisions; no gate activation follows from this record.
