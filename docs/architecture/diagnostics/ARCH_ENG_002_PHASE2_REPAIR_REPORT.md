# ARCH-ENG-002 Phase 2: QEMU verification correctness repair

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [ARCH-ENG-002 Phase 2: QEMU verification correctness repair](#arch-eng-002-phase-2-qemu-verification-correctness-repair)
  - [Authority and baseline](#authority-and-baseline)
  - [Reproduction and implementation traceability](#reproduction-and-implementation-traceability)
  - [Result meaning and implementation scope](#result-meaning-and-implementation-scope)
  - [Executed checks and evidence levels](#executed-checks-and-evidence-levels)
  - [Remaining boundaries](#remaining-boundaries)
  - [Exact changed-file inventory](#exact-changed-file-inventory)

<!-- T81-TOC:END -->


Date: 2026-11-10. Disposition: **READY FOR INDEPENDENT REVIEW** of the bounded
engineering candidate. Full real-guest success is **NOT VERIFIED**.

Author: Codex assistant implementing and testing this candidate in the same
session, with prior Phase 1 diagnostic context. These are fresh local engineering
executions, not independent-person acceptance or scientific qualification.
No model identity beyond the supplied Codex context is asserted.

## Authority and baseline

Fetched `origin/main`: **ee0c7d90537eaa2dc4b522c1624fcabebf6e999b**.
[PR #755](https://github.com/t81dev/t81-foundation/pull/755) is merged at that
commit, including head correction `fd2439b996d9ab44f363813136843f48729314a5` after
`1785344125bc8ec4cf65bc6aeacc9aa6ed18522e`. Source, diagnostic report, visualization
report, evidence-index boundaries and diagnostic tests were inspected directly.

Final implementation/test/documentation commit:
**5beffd4058c609b1387896da0c796cfb4e4dae20**. Subsequent packaging adds this report
and evidence only; the draft PR description records the final deliverable head.

The maintainer checkout was dirty: four modified C++ files and eight untracked
paths. Exact status is retained in `evidence/phase2/validation.json`; none was
changed by this repair. Work used a clean isolated checkout of fetched main at
`/private/tmp/t81-qemu-phase2`, branch `fix/qemu-fail-closed`.

Open work inspected included #747 portability and its dependents, and earlier
QEMU/research/CI proposals such as #709 and #577. No existing PR was changed.
This branch is based on main, does not incorporate #747, and changes none of its
existing CMake, runner, runtime or compiler repairs. This is an inspected snapshot,
not an exhaustive guarantee against future conflicts.

## Reproduction and implementation traceability

| Defect | Baseline disposition and evidence | Corrective code | Regression coverage |
|---|---|---|---|
| D1 arithmetic | **NOT REPRODUCED on local Bash 3.2.57**: unsafe increment exits 0 and reaches the final marker. The version-sensitive expression remains in both baseline scripts; do not generalize the Phase 1 newer-shell result to this host. | Both automated entry points delegate to `verification.py`; completion is derived from successful checks, with no shell arithmetic command. | `test_baseline_arithmetic_and_safe_increment` records local unsafe behavior and verifies the assignment alternative from zero; positive entrypoint runs reach 5/5. |
| D2 false positives | **REPRODUCED** in extracted baseline summary code: empty, failure-containing and epoch-dated logs produce success; missing logs yield partial with exit 0. Exact ignored-wait idiom reports completion after child exit 37 or SIGTERM. These are isolated fixtures, not full guest boots. | `verification.py`: QMP assertions, exact-size RAM output, guest-ready and fresh help-response markers, checked quit/zero exit, final evidence scans, hard deadline and signal cleanup. | 20-mode negative matrix; exact child exit 37/23 and signal 143; deadline/dribbling reply/TERM-resistant child; stale/missing/empty results; failure markers; missing help and pre-command-only help. |
| D3 directory mismatch | **REPRODUCED** by executing the exact unexported caller/callee assignments with OUTPUT_DIR unset: fixed caller path differs from child's mktemp path. The legacy ternaryos entry point additionally overwrites OUTPUT_DIR. | Worker exclusively creates a fresh attempt under OUTPUT_DIR; all wrappers forward environment and current exit status. No report is searched for success. | `test_output_directory_unset_empty_relative_and_invalid`, stale-success rejection, repeated fresh runs, generated verifier and both entry points. |
| D4 paths | **REPRODUCED / STATICALLY VERIFIED**: three referenced `experimental/ternaryos/*` scripts are absent at the pinned tree (git cat-file exit 128). Actual historical monitor/analyzer utilities exist under both drivers/qemu/scripts and ternaryos. | Shell wrappers resolve BASH_SOURCE; worker resolves repository root from its own file. Generated verification and generated production-test invocation use existing paths. Diagnostic utilities are no longer acceptance authorities. | Invocation from `/` and unrelated temp cwd; missing/non-executable program; missing script, image and firmware; generated verifier extracted without running the installer. |

Raw baseline fixtures and commands: [baseline.json](evidence/phase2/baseline.json).
Run `python3 tests/qemu/reproduce_baseline.py` to repeat the pinned extraction.
Baseline full-script success is not established: obsolete paths and prerequisites
prevent treating excerpt execution as a native end-to-end result. No defect is
labeled ALREADY FIXED without evidence.

## Result meaning and implementation scope

The previous duplicated, nonasserting orchestration is consolidated into one
bounded worker, retaining five named checks and three public shell entry points.
This changes verification tooling only. It adds no OS functionality or general
QEMU framework. Python standard-library QMP sockets replace the unbounded nc
monitor exchanges, swallowed child waits and external timeout dependency.

Monitor responsiveness, a 4096-byte RAM sample and enumerated CPU/block devices
are host inspection evidence. Guest evidence requires the source-backed t81sh
ready marker and a help reply appended after the command is sent. Final success
also requires QMP quit, zero child exit and nonempty, failure-free evidence.
A check count alone cannot publish success. See
[exact check semantics and configuration](../../../drivers/qemu/scripts/VERIFICATION.md).

OUTPUT_DIR now denotes a parent containing fresh attempt directories. Existing
files are never cleared/reused. The verifier retains argv, QMP, serial, child
logs, image copy, RAM sample and summary, and prints the exact path. Caller and
callee use one owner; there is no second stale-summary decision. Timeouts return
124; interruptions map to 128+signal; observed child failures preserve meaningful
exit status. Cleanup terminates, escalates to kill if necessary, and reaps the
owned process. Raw files and input hashes establish identity, not truth.

## Executed checks and evidence levels

Local host: macOS ARM64, Darwin 25.5.0, Bash 3.2.57, Python 3.14.3,
QEMU 10.2.2. Local Unix sockets require execution outside the managed sandbox;
the initial sandbox bind rejection was an environment limit, not a test pass.

- **STATICALLY VERIFIED:** `bash -n` on all four requested shell files; Python
  byte compilation on all four new Python files; `git diff --check`; architecture
  validator `--drift-check` reports zero missing paths, stale references,
  unsupported types and inconsistencies. Exact argv/exits in `validation.json`.
- **MOCK-EXECUTION VERIFIED:** `T81_VERIFICATION_RECEIPTS=/private/tmp/t81-phase2-mocks.json python3 tests/qemu/test_verification.py`:
  **11/11 methods pass**, **42 recorded entrypoint calls: 9 accepted, 33 rejected**.
  A separately supervised interruption verifies exit 143 and absence of the
  reaped child; two arithmetic subprocesses record unsafe/safe behavior.
  These latter three subprocess calls are outside the 42 entrypoint receipts.
  Repeated positives produce equivalent summaries in distinct fresh directories;
  all rejected entrypoint receipts lack the success disposition. Raw records:
  `mock-processes.json`, `mock-tests.log`.
- **QEMU-EXECUTION VERIFIED:** final real invocation in `real-qemu.json` uses the
  unchanged tracked deployment image and discovered Homebrew EDK2 firmware.
  QMP handshake/running and RAM sample checks complete. Serial emits
  `Error: Image ... start failed` before guest readiness; worker returns **1**,
  **2/5**, **VERIFICATION FAILED**, and reaps QEMU. This verifies rejection of
  actual failed execution. It does not verify successful guest boot/integration.
- **NOT VERIFIED:** full successful real guest attempt, newly rebuilt image,
  Linux/x86 execution, Windows/WSL execution, ShellCheck and full pytest suites.
  ShellCheck is unavailable; pytest invocation exits 1 with `No module named
  pytest`. Bash/Python syntax checks are not substituted for these tools.
  Added workflow covers mock execution on Ubuntu 24.04 and macOS 15; hosted
  results remain pending at packaging. Native Windows cannot directly execute
  the POSIX Bash/signal/Unix-socket entry point, as with the prior shell tooling.

The final mock run took 34.287 seconds. A previous 0.7-second fixture deadline
produced a positive-case timeout at 4/5 on the busy host. That failed run is
preserved as `development-test-failure.json`; the fixture allowance is now two
seconds, and the real verifier default remains 60. Earlier serial-empty timing
and long-TMPDIR socket-path failures were corrected; empty evidence still cannot
satisfy the final checks, and the private sockets use a short random /tmp path.

The real image and firmware hashes, exact command, QMP/serial/process output,
RAM-sample hash and retained external attempt path are recorded in
`real-qemu.json`. The tracked image was not rebuilt, and the underlying UEFI
failure cause was not diagnosed or repaired. No successful real-guest result
or QEMU_EXECUTION_UNAVAILABLE claim is made: QEMU did execute and reject.

## Remaining boundaries

The full `/opt` installer was not executed. Its unrelated clone/build layout,
launch-generation and documentation-copy assumptions remain outside this repair;
several historical `experimental/ternaryos` paths remain in those non-verification
parts. Supply an explicitly built image to the checked-in verifier. The generated
verification body and generated production-test routing are covered without
installing system files. No installer readiness claim is justified.

Conservative textual failure detection can reject benign diagnostics; these
need review, not suppression. This is trusted local tooling, not an adversarial
host/process sandbox or proof of complete OS correctness. Host architecture,
firmware and guest inputs can alter dispositions. A future real positive run
requires a working image/firmware combination and retained raw evidence.

No ISA/arithmetic/normative-contract changes; no frozen measurements or receipts
edited; no SCG-08 Gate 2, new research campaign, Protocol 001 activation,
architecture-classification promotion, merge or protected-branch action.

## Exact changed-file inventory

19 files relative to the baseline; no deletions outside these replacements.

- `.github/workflows/qemu-verification.yml`
- `docs/architecture/diagnostics/ARCH_ENG_002_PHASE2_REPAIR_REPORT.md`
- `docs/architecture/diagnostics/evidence/phase2/SHA256SUMS.json`
- `docs/architecture/diagnostics/evidence/phase2/baseline.json`
- `docs/architecture/diagnostics/evidence/phase2/development-test-failure.json`
- `docs/architecture/diagnostics/evidence/phase2/mock-processes.json`
- `docs/architecture/diagnostics/evidence/phase2/mock-tests.log`
- `docs/architecture/diagnostics/evidence/phase2/real-qemu.json`
- `docs/architecture/diagnostics/evidence/phase2/validation.json`
- `drivers/qemu/README.md`
- `drivers/qemu/scripts/VERIFICATION.md`
- `drivers/qemu/scripts/automated_verification.sh`
- `drivers/qemu/scripts/verification.py`
- `drivers/qemu/scripts/verify_production.sh`
- `ternaryos/automated_verification.sh`
- `ternaryos/production_qemu_setup.sh`
- `tests/qemu/fake_qemu.py`
- `tests/qemu/reproduce_baseline.py`
- `tests/qemu/test_verification.py`

Evidence hashes are in `evidence/phase2/SHA256SUMS.json` (six artifacts; manifest
excludes itself). They establish content identity only.
