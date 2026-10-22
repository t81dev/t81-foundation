# Phase 1 audit and research report

2026-10-22. Experimental, bounded implementation of Candidate 3 selected by
[PR #550](https://github.com/t81dev/t81-foundation/pull/550), not promotion of a
new verified surface. CPU packed-ternary optimization remains frozen.

## Baseline and dependency audit

Remote `main` was inspected through GitHub and fetched before work:
`a472147f044cf62755c0009a932909a8e5c6c6c9`, the latest merged commit, PR #550.
The working checkout was on an unrelated experiment branch; this work uses a
separate worktree based on that SHA. No other PR was modified or merged.

[PR #536](https://github.com/t81dev/t81-foundation/pull/536) was open at head
`3e52b6b5247b578e5f6a861a1b9a971b6e93c7cd`. Its file patches were inspected,
including `value_ops.cpp` division/modulo/negation, BigInt single-limb sign
normalization, float/fraction flag dereferencing, tier mapping, ControlledAIBackend
and direct API corrections. The PR's stated 393 passing suites were not rerun or
treated as evidence here. It does not remove every helper add/sub/mul overflow
risk. We do not depend on that PR: the workload has small positive integers;
negative cancellation, helper signed overflow, `INT64_MIN / -1` and float/fraction
branch flags remain excluded. Minimum follow-up: audit the merged fixes and add
isolated sanitizer fixtures before admitting these domains.

| Existing surface inspected | Reuse and evidence boundary |
| --- | --- |
| RFC-0028; `vm/jit/jit_compiler.cpp`, `jit_trace_cache.cpp`; `jit_repro_oracle_test.cpp` | Existing instruction identity and CanonFS JIT cache; no second JIT/replay-cache implementation. This experiment records the interpreter's actual trace, never claims JIT parity. |
| RFC-0030; `T81Float.hpp`; existing float boundary/soft-math tests | Canonical conversion probes only. The RFC's correctly rounded/full-domain goals exceed its tolerance-based acceptance examples; no transcendental accuracy or libm performance claim here. |
| RFC-0043; Determinism Surface Registry | Explicit corpus, traps, ordered trace, platform/compiler/source identity and adversarial verification. Same-host equality does not establish cross-platform identity. |
| `vm/vm.cpp`, `state.hpp`, `value_ops.cpp` | Real `run_to_halt`, pre-dispatch policy, state trace and Axion log. Actual VM arithmetic takes BigInt paths; helper arithmetic is not assumed safe. |
| Axion `policy_engine.cpp`, CanonFS `kernel/axion/canonfs_hook.cpp`, `persistent_driver.cpp` | Existing policies deny VM Store or output-object creation before their protected writes. Existing text-hook hardening is reused. |
| `tracing/canonhash.hpp`, `canonhash81.cpp` | Production SHA3-512 truncated to 256 bits and Base81 references; verifier independently uses Python hashlib. |
| `trace_repro_gate.py`, `cli_trace_export_test.cpp`, accountable-computation harness/verifier | Existing golden trace replay/export and detached computation verification are preserved. This increment connects actual ordered VM traces, policies and stored result bytes into one bounded record. |

Discrepancies recorded rather than broad documentation churn:

- `PROJECT_BOUNDARIES.md` says "100% tested", "393/393", and calls RFC-00D1
  stable v1. `SYSTEM_STATUS.md` scopes Axion as governed non-DCP, lists historical
  counts including 406, and keeps the broader RFC draft. A passing test count is
  neither complete coverage nor proof of platform identity.
- The registry calls soft-float strict/verified and defines cross-platform replay
  partly through identical pass/fail. RFC-0043 requires exact governed artifact
  equality. Here actual bytes are compared; pass/fail alone is insufficient.
- RFC-0028's original identity formula includes state/policy; its acceptance note
  narrows the implemented cache key to the instruction sequence. The complete
  experiment record separately binds input, configuration and policy.
- `T81Float::from_double` itself uses host `log`, `floor`, `powl` and `llround`,
  even in strict builds. Integer-backed transcendental wrappers do not make
  this conversion path independently verified across all hosts.
- Recent closure research falsifies CPU packed-ternary execution-speed advantage
  and selects Candidate 3. It supplies no evidence for Candidate 3's <5% overhead
  hypothesis or WASM identity. This experiment does not infer either result.
- GitHub check runs at baseline showed successful documentation/deployment/format
  checks, not a fresh full runtime-suite result. Existing CI workflow definitions
  are coverage intent, not proof that this new experiment passed remotely.

## Observations

The admitted program computes `((input + 2) * 3) / divisor`, modulo-tests a zero
remainder, takes a branch that skips a poison assignment, stores the quotient in
VM memory and then persists result bytes through CanonFS. Inputs are canonical
integers 0..100 and divisor 0..3. The verifier reconstructs this fixed program and
path independently; it is not a general ISA emulator.

- `40 2` yields exact `63\n`; `41 2` yields `64\n`. Inputs/configuration repeated
  on native macOS ARM64 produce identical complete evidence bytes per case.
- Nine-instruction budget denies the tenth dispatch, Store. Its trace lacks Store,
  memory remains zero, and no persistent store is opened. A denying write policy
  leaves computed memory at 63 but creates no result payload.
- Division by zero yields `DivisionFault`; no result object is created.
- `INT64_MAX + 1`, `INT64_MAX * 3` and wide quotient-by-one probes use production
  BigInt, with independent Python integer expectations. These are arbitrary-width
  results, not claims about native signed overflow semantics.
- `T81Float<27,9>` converts both signed zeros to `+0E0`, NaN to `NaE`, infinities
  to signed Inf, and the sampled subnormal, minimum-normal and +/-maximum finite
  binary64 values to `+0E0`. These pinned observations expose a range/conversion
  limitation. They do not establish IEEE-conforming extreme-value semantics.
- Adversarial tests reject changed output, trace order/content, input/program,
  policy text/audit, references, numeric probes, missing fields, duplicate JSON
  keys and caller-pin mismatches, even after an attacker recomputes record hashes.
  Store inspection detects missing, modified or unauthorized payloads.

Native ARM64: tested. Native x86_64, Linux and Windows: **NOT TESTED locally**.
WASM: **NOT TESTED**. No QEMU measurements used. Exact cross-platform byte identity
remains unestablished until CI/native artifacts are available.

## Measurement limits and follow-up

The closest valid baseline is the same production VM/program and storage path
without configured VM/write policies; its built-in trace generation remains on.
This isolates configuration/enforcement increment rather than comparing unrelated
host arithmetic. Readback authorization is common to both arms. Five interpreter
warm-ups precede every sample; rotating order and 30 repetitions expose variance.

The machine-readable native observations include separate setup, execution,
trace/audit serialization plus trace hashing, storage/readback, numeric probes
plus record JSON/hashes, producer total, whole subprocess and verifier timings.
`total_ns` excludes stdout delivery and Python canonical-envelope hashing;
`process_ns` includes startup and warm-up, and is reported separately. Memory
includes whole-process peak RSS and retained trace/audit vector capacity; artifact
bytes and stored payload bytes are separate. RSS includes warm-up and is not a
marginal allocator measurement. No CPU affinity, governor or cache flush is assumed.

**Exact remaining measurement blocker:** the stable VM API always generates its
trace inside `run_to_halt`; trace generation alone cannot be timed separately.
Minimum change for a later experiment is a research-only instrumented VM build or
an explicitly reviewed tracing toggle. No fabricated trace-generation cost is
reported. Likewise, this integer workload cannot resolve the <5% deterministic
math-versus-libm hypothesis; a later pinned transcendental corpus and comparable
host baseline are required. Local timing ratios do not establish that hypothesis.

The existing cross-platform workflow now builds this opt-in experiment, runs its
regression suite and repeated measurements, uploads metadata and canonical
artifacts, and compares exact evidence bytes for all eight cases between Linux x86_64
and macOS ARM64. The pinned fixture additionally gates each local admitted case.
Remote results are pending, not assumed passing. Windows remains a native build
and Unicode CanonFS filename follow-up; experiment code uses fixed decimal/LF
serialization, portable chrono, and optional getrusage (unsupported hosts report
RSS as -1). No shell-only runtime dependency is added to the producer/verifier.

Next finishable steps: collect the native CI pair and inspect divergences; run a
Windows native build; audit PR #536 with UBSan boundary tests; decide whether
extreme conversion behavior warrants a separate correctness fix; only then widen
to bounded deterministic transcendentals and a WASM toolchain/runtime. Preserve
all failures and platform exclusions before expanding the claim.

## Preserved native measurement

Measured source commit: `77d63b5025bb86634cf96c35f8af7482b9b0cef5` (clean tracked tree). The binary and all experiment source digests are recorded in [observed-arm64.json](observed-arm64.json).

| Stage | Baseline mean ± sample stdev (µs) | Governed mean ± sample stdev (µs) |
| --- | --- | --- |
| VM execution (trace included) | 22.82 ± 0.87 | 25.05 ± 2.13 |
| Trace/audit serialization + trace hash | 5.99 ± 0.11 | 6.05 ± 0.11 |
| Storage + readback | 529.17 ± 429.32 | 543.64 ± 451.60 |
| Numeric probes + record encoding/hashes | 136.30 ± 2.26 | 140.15 ± 4.15 |
| Producer interval | 699.32 ± 429.61 | 720.76 ± 452.69 |

30 samples/arm; VM execution mean increment 9.78%. This single microbenchmark run neither validates nor falsifies the soft-math/libm <5% hypothesis. Raw samples, extrema, medians and variance remain in the JSON.

Canonical allowed evidence: 1516 bytes versus 1459 baseline bytes; stored payload: 3 bytes in both. Retained trace/audit capacities: 384/432 bytes in both. Mean peak RSS: 2500.3 KiB governed, 2462.9 KiB baseline. Whole-process RSS includes warm-up and cannot isolate the incremental tracing allocation.

Local validation: all six selected CTest suites passed (including the seven-test experiment suite); standalone fixture replay and the determinism-claim governance check passed. All 240 committed-source measurement executions passed actual storage inspection and exact per-case repeatability.
