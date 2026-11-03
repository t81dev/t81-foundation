# H04-R03 measurement engine — execution remains frozen

This change implements clock-delimited lifecycle operations, a read-only host
probe, fail-closed qualification checks, paired-order planning, canonical JSON
artifacts and analysis of synthetic timing records. It does not authorize or
perform Phase C measurements. `benchmark_harness --run-measurement` still exits
with an error. `measurement_engine.run_measurements()` also rejects every call;
there is no authorization flag, environment-variable bypass or executable worker.

Candidate A PR #679 integrated at
`d8e1376a7a77902cc26269c6e8d4991bcde117a7`, the `origin/main` baseline of this change.
The accepted Phase B reference is PR #658, head
`0a48054510e79e753d5152ccb3f9f130c17bbafd`.
The preregistration was recovered from PR #677's integration commit
[`8e44c0f667c62e0d4143bb93169fc3235d5ebd66`](https://github.com/t81dev/t81-foundation/blob/8e44c0f667c62e0d4143bb93169fc3235d5ebd66/docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md).
It is absent from current main: commit `6e5ca4ea54923b2119bac117d6c5b70de71d2586`
deleted it. This implementation does not restore or revise that historical file.
The Candidate A and Phase B validation reports are historical reports, not new
independent validation from this implementation session.

## Lifecycle and accounting

The internal `_Lifecycle` adapter uses existing block packers, block decoders,
random-access functions and length formulas. It changes no codec or workload
implementation. All arms are T243, B10, whole-block B729 and B729-u5. B243-CLONE
remains an existing correctness witness, outside the timed four-arm comparison.

| Phase | Charged work |
| --- | --- |
| 1 | Domain checks and signed little-endian int16 to compact unsigned 16-bit mapping |
| 2 | Constant-space frame geometry and destination allocation |
| 3 | Header and block packing into the allocated frame |
| 4 | Release owned raw/mapped inputs and collect garbage |
| 5 | Count/length verification and streaming canonical block validation |
| 6 | Existing bounded random-access extraction over the query schedule; consume results in a scalar XOR sink |
| 7 | Sequential reconstruction into little-endian signed 64-bit output and checksum verification |
| 8 | Release owned frame, output and query buffers and collect garbage |

Phase 7 preserves the accepted harness's sequential full-dataset reconstruction;
it does not retain the Q random-query outputs. The XOR sink is diagnostic and
is not a substitute for the independent output witness. Canonical output checksum
input is N signed little-endian 64-bit values in dataset order. The checksum
factory must implement xxHash64; tests explicitly inject SHA-256 as a test double.
No SHA-256 output is labeled an xxHash64 result.

The adapter passes a bounded signed view over mapped values to the accepted
packers. Its inverse-offset arithmetic and the packer's offset arithmetic are
both included in Phase 3. This intentionally preserves the reference packers;
it is not an optimized native implementation or evidence of baseline competence
for a future performance claim. Phase 5 streams canonical block checks without
an N-sized output or offset list. Phase 6 has no decoded-block cache, microblock
cache, persistent offset array or query-result array. The output occupies 8N
bytes in Phase 7. Compact input and query storage does not change generation,
seeds, symbol order, rejection sampling, schedule or canonical digests.

`verify_inputs` rejects incorrect lengths or SHA-256 digests before Phase 1.
Generate and serialize inputs using the unchanged `workload.py` functions outside
trials, release generator lists, and pass exclusively owned compact buffers.
External aliases to input buffers violate the retention contract. Static Python
modules, integer constants and interpreter memory count toward total RSS.

`_instrument` times all eight operations with `perf_counter_ns`, accepts an
injected clock for tests, and always attempts cleanup on failures. It records
nanoseconds for each phase, including validation, mapping, allocation, packing,
checksum and cleanup. The preregistration's explicit primary definition sums
Phases 1–7; Phase 8 is reported separately. No timer overhead subtraction,
calibration correction or unreported warm-up is applied.

## Memory enforcement and retention boundary: execution blockers

The engine requires a continuous-enforcement memory backend before reading a
clock or executing any phase. Its interface is `enter(phase)` / `leave(phase)`;
records must contain nonnegative integer `peak_rss_bytes`, `peak_scratch_bytes`
and `continuous_enforcement: true`. Cleanup additionally requires `reclaimed`.
Missing evidence, RSS above 32 MiB, query scratch above 64 KiB, checksum mismatch,
invalid clock intervals or failed reclamation invalidate the trial. These limits
are binary units, consistent with the preregistration's powers-of-two buffers.

**No production memory backend is supplied or qualified in this PR.** Synthetic
backends test the engine's accounting and failure paths. A monitor must charge
all interpreter, table, frame, output and temporary memory, detect transient
allocations, enforce immediate RSS termination, and retain failures in a parent
process even if a worker is killed. Phase-boundary RSS polling, `tracemalloc`
alone, `RLIMIT_RSS`, and a Python boolean are not evidence of that enforcement.
Linux cgroups account more than process RSS; a cgroup limit must not silently
replace the registered RSS estimand or scratch definition.

Likewise, releasing CPython references and invoking `gc.collect()` is not proof
of a physical retention barrier, allocator reclamation or a cleared CPU cache.
The engine does not claim these properties. A qualified barrier and reclamation
backend, with independently checked receipts, remains required before execution.
No execution path is exposed while those obligations remain unresolved.

## Host-qualification procedure (read-only)

A future authorized operator must capture `collect_host()` on the dedicated
Linux host, without running codec timings. Review and pin the snapshot before
scheduling trials. Qualification requires all of:

1. Linux x86_64, at least four physical package/core pairs, an exact kernel release
   pin and separately reviewed evidence that it is an approved LTS kernel.
2. Exact affinity list contained in the kernel's isolated CPU list, performance
   governor, equal minimum/maximum frequency matching the approved kHz pin, and
   disabled turbo through supported Intel pstate / cpufreq controls. Missing
   controls fail closed; alternative mechanisms need review and an adapter.
3. Complete pinned L1/L2/L3 size, type, cache line size and sharing topology.
   SMT sibling and shared-cache isolation need an operator exclusivity review;
   sysfs alone cannot establish exclusive tenancy.
4. Disabled system swap and a known physical/cgroup allocation ceiling of at
   most 16 GiB. The probe resolves the current process's cgroup v2 path. It is
   intentionally conservative: unsupported cgroup layouts require review.
5. Dedicated-host evidence, an approved cache-reset procedure between independent
   worker processes, and continuous memory-enforcement evidence.

`qualify_host(snapshot, pins)` returns every failure and the exact snapshot and
pins. It never changes governor, turbo, affinity, swap, cgroups or kernel settings.
Operator evidence fields are references for human review, not executable commands
or authentic authorization. Successful qualification does not lift the freeze.
Pin CPU model/microcode, OS image, Python build, checksum dependency/library,
allocator, controller and all relevant environment variables in the artifact's
`environment` object as part of execution review. Do not substitute macOS, Windows
or ARM64 timing results for the registered Linux x86_64 host. Deterministic tests
and pure host-policy fixtures can run on those other platforms.

## Statistical records and artifacts

`paired_orders(order_seed)` supplies 30 reproducible uniform permutations of the
four arms. Record the seed prospectively; never search seeds against results.
A future authorized controller must launch a fresh process for each arm in each
paired block and record unique process identities (including start identity,
not a potentially reused PID alone), order, cache-reset receipts and failures.
The gate deliberately supplies no subprocess launcher while execution is frozen.

Each raw trial has eight `phase_ns`, `lifecycle_ns`, `query_ns`, per-phase resource
records, output and witness checksums, process identity, validity and failure.
`summary` is computed by `summarize(blocks)` only for exactly 30 complete paired
blocks. Invalid evidence takes precedence over high noise. Missing or duplicated
arms/processes, missing resource evidence, malformed timings or checksum failures
produce INVALID, retaining the raw records in the enclosing artifact.

Lifecycle ratios are comparator median latency / T243 median latency. Query
ratios use median T243 throughput / median comparator throughput, calculated
from each run's Q / query time. In particular, ratios of median latencies are
not silently substituted for ratios of median throughputs with 30 observations.
Paired whole-run bootstrap resampling uses the inherited 10,000 resamples and
seed 42, with the inherited percentile-index convention. Lifecycle CV uses
sample standard deviation / arithmetic mean. No thresholds or decision precedence
are changed: INVALID, noise >5% INCONCLUSIVE, conclusive REJECT, complete ACCEPT,
then residual INCONCLUSIVE. Malformed input no longer receives fabricated default
confidence intervals from the existing decision helper.

`make_artifact` returns canonical sorted-key ASCII JSON plus its SHA-256 identity.
It includes exact configuration, seeds, canonical workload hashes, source commits,
environment, host qualification, randomized order, raw timings/resources/failures
and statistical outputs. Store both return values without editing the JSON.
Identical inputs yield identical bytes and hash; nonfinite JSON values fail.
Synthetic artifacts are explicitly labeled SYNTHETIC. Non-synthetic artifacts
remain UNEXECUTED while the gate is frozen. A synthetic ACCEPT means only that
the artificial numbers satisfy the predicate; it is no performance finding.

## Remaining prerequisites before authorized execution

- Review this implementation and its reference-adapter costs for comparator
  competence; do not infer performance from correctness tests.
- Resolve the absent main-tree preregistration through human review of its exact
  historical pin; do not silently revise the protocol.
- Provide and validate continuous RSS/scratch enforcement, kill/failure retention,
  a retention barrier, and process-level reclamation verification.
- Pin the xxHash64 backend, seed, dependency identity and independently derived
  expected output checksum, and verify known-answer vectors before any timings.
- Qualify one dedicated host and approve cache reset, isolation, exact toolchain,
  allocator and environment pins. No host has been qualified in this session.
- Review the output-order/64-bit serialization and sample-CV conventions above;
  if they conflict with the accepted contract, stop for a prospective clarification.
- Review a separate worker/controller activation change covering 30 independent
  paired process blocks, uniformly randomized arm order, input digest preflight,
  cache-reset receipts and preservation of killed-worker failures.
- Obtain explicit human execution authorization and a separately reviewed freeze
  lift. Neither test success, qualification JSON nor this implementation PR grants it.

This synthetic workload establishes no production applicability, radix-exclusive
advantage, hardware advantage or performance advantage.

## Validation receipt — 2026-11-02

Author: Codex agent; exact model identity not recorded. This is author validation,
not an independent scientific review or independent performance reproduction.
Host: macOS 26.5.2, ARM64, Python 3.14.3. Linux host enforcement, CPU control,
cache reset, barrier semantics and qualified xxHash64 integration were not run.

| Exact command | Observed outcome |
| --- | --- |
| `python3 -B -m unittest experiments.h04_r03_matched_codec.test_measurement_engine experiments.h04_r03_matched_codec.test_harness -v` | 22 tests passed at the initial implementation slice |
| `python3 -B -m unittest experiments.h04_r03_matched_codec.test_measurement_engine -v` | Repeated after edits; 14, then 15 tests passed |
| `python3 -B -m unittest discover -s experiments/h04_r03_matched_codec -v` | Initial 52 and final 53 tests passed; zero failures, errors or skips |
| `git diff --check` | Passed |
| `python3 -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H04"}}'` | Failed before discovery: `ModuleNotFoundError: No module named 'jsonschema'`; pinned source documents inspected directly |

Final full-suite stdout/stderr was retained locally at
`/private/tmp/h04-r03-phase-c-tests.log`, SHA-256
`9748c8a06db0fa0a3559409049a35844a8be2ad8a7a5a7c780d05c438c1e8cba`.
Test runner elapsed-time summaries are ordinary correctness-test metadata, not
experiment measurements. All new instrumentation tests inject clocks; all
statistical observations and memory-enforcement receipts in those tests are
synthetic. Existing workload tests reproduce both full canonical hashes without
collecting performance observations.

Changed files are confined to `experiments/h04_r03_matched_codec/`:
`benchmark_harness.py`, `measurement_engine.py`, `test_measurement_engine.py`,
and this document. No codec, workload, oracle, historical report, governance or
CI file changes are included. No thresholds, workloads, seeds, comparison arms,
wire formats or bootstrap definitions were changed. Execution conformance is
not established: the unresolved backend and runner prerequisites above remain
explicit blockers, and this PR is a draft infrastructure increment.
