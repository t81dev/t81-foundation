# Combined #794 / #795 / #780 integration candidate

Base main: `c2ab68e582c9f773ce94a9e13bcb805016f07dd4`.
Isolated branch: `candidate/pr794-pr795-pr780-main-integration`.

Accepted component pins:

- #794: `5f5751ae9b9f634f2af2cb020afef6f9e2cd610d`.
- #795: `43747b60675444ab58f693713577d6cd42e3f234`.
- #780: `c08c211bd3475d8b09e35648f84c6b5c0a7e31a2`.

`accepted-file-identities.json` records every component's changed file, Git blob,
mode and SHA-256. All 48 files were copied byte-for-byte from those pins. Neither
the three component deltas nor newer main changes overlap. No substantive
resolution or source rewriting was necessary. Original heads are preserved.

#794's wrapper, skill text, Python harness and top-level CMake registration are
retained with its historical receipts. #795's validator, experiment CMake and
original audit packet are retained. Its separately retained parity evidence is
copied unchanged to `../2026-11-15-pr795-validator-ci-parity/`; that packet still
identifies execution against #795, not the combined candidate. #780's corrected
CPU-time/batch timing documentation, source and Cell correctness checks are
retained, including byte-identical raw measurements and their original label
strings. No production Cell arithmetic is changed and no new BASE-PERF-00
performance campaign is authorized or performed by this integration.

Integration-only CI wiring adds direct positive, negative and JSON-file validator
execution normally and under Python -O to the existing main CI docs/spec job,
with the existing jsonschema==4.26.0 pin and retained log artifact. This closes
the previously reported hosted selection gap without a new standalone workflow
or any schema-engine changes. Hosted validator execution is direct Python;
local CTest execution remains separately identified in the qualification record.
The existing unfiltered benchmark job is restricted to the SIMD cases consumed
by its unchanged regression checker, with report writes disabled. Its threshold
check and the separate VM workload guardrail remain active; Cell timing is not
rerun as part of qualification.

Local correctness ran against assembly commit
`1201ccd421d2fec5630dbe05c6eb06e39a9134b0` on macOS ARM64 / AppleClang 21 /
Python 3.14.3, Release with NDEBUG. See `local-validation/summary.json`, per-process
wrapper/policy receipts, command logs, compile_commands and JUnit. All component
source bytes remain identical in the published candidate; subsequent changes
are CI wiring/hygiene and retained qualification evidence.

- Fresh Release build: CLI, process capture, repro-hash, exhaustive Cell test and
  benchmark_runner compile successfully; benchmark_runner was not executed.
- Direct wrapper: 86 process receipts / zero failed checks.
- Direct execution-policy matrix: 249 processes / zero failed checks.
- Direct validator positive, negative and JSON-file suites: all six normal/-O
  invocations pass with jsonschema==4.26.0. Each negative invocation runs 94
  negative fixtures and Draft 7 parity (61 rejected / 2 positive controls).
  Each file invocation runs 57 cases, including 49 rejection / 8 positive cases.
- Registered targeted CTests: 10/10 pass, including wrapper, execution policy,
  repro-hash, process capture, Cell exhaustive oracle and all five validator
  registrations. Optimized positive validation is direct Python only, not a
  sixth validator CTest.
- Cell oracle retains 243 conversions, exhaustive 59,049-pair arithmetic checks,
  441 safe multiplication pairs, 352 documented in-range intermediate-overflow
  cases, 58,806 valid divisions and 243 divide-by-zero rejections.

The local runner's final workflow audit failed on inherited main findings:
seven mutable action references and one missing explicit permissions block.
`inherited-workflow-audit-failures.json` reproduces both against unchanged #795's
main-equivalent workflow files. Integration-only fixes pin those seven refs to
actual upstream-resolved commits and add contents: read to the x86 QEMU workflow.
Raw resolution responses are in `action-pin-resolution.json`; no component file
was affected. `local-validation/workflow-audits-corrected.json` records both
corrected checks passing (112/112 pinned refs, 22/22 explicit permissions).
The original failing receipt remains retained rather than relabeled.

Full hosted qualification is pending at publication. Hosted results must be tied
to the final candidate head and actual PR merge ref; local passing checks do not
substitute for that run. No source PR is merged or closed.

This is Codex-authored engineering integration with prior context, not
independent scientific acceptance, new experiment approval or governance change.
