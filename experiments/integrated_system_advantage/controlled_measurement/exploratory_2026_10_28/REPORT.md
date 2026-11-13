# Investigation 08: EXPLORATORY — NONCONFIRMATORY

**Final disposition: EXPLORATORY DATA COLLECTED.** Ten randomized matched pairs,
20 fresh workers, one predefined input (`greet hello`), empty independent stores.
Collection took 64.630480 seconds, within the 30-minute cap.
Every exploratory record is permanently excluded from the preregistered dataset.
There are no confirmatory observations, confidence intervals or materiality verdicts.

Prepared October 28, 2026 by OpenAI Codex. Candidate:
`05639a677b11ea1f4ef0328b3f67140784a4ead1`; accepted executable/comparator lineage:
`0662e0e637f47168e56ca4ddd66e8ee877473b38` (merged #627). The wrapper and collection
are Codex-authored; this is not independent Jules review of the apparatus/statistics.
This is the researcher's explicitly authorized exploratory sprint, supporting
canonical H07 and Investigation 08/H-S1 without redefining H01–H08 or its history.
It exercises existing deterministic CLI, policy and CanonFS surfaces; no subsystem
or runtime-maintenance scope was expanded.

## Boundary and host

A = existing T81 C++ CLI through the candidate matched adapter.
B = unchanged accepted pure-Python `ConventionalTaskRunner` through the same worker.
Both persist and hash-verify model, normalized input, exact APL, reversible exact
tokenizer text, result, backend provenance and recovery bundle. Model, input,
policy, tokenizer and result references agree across arms; provenance/bundle
references intentionally differ. All role identities are stable across pairs.

`collect.py` is the only added timing wrapper. It launches the candidate's unchanged
single correctness worker. Its `perf_counter_ns` interval starts immediately before
`Popen` and ends after `communicate` returns; the worker completes seven-object
readback/verification before returning. Includes interpreter/import startup, fixture
reads, A's temporary fixture construction, computation, admission, persistence,
readback, serialization, nested diagnostic journal fsync, pipe capture and teardown.
Excludes fixture generation, store setup, outer launch journal fsync and parent
JSON/parity checks. Original validation payloads remain unchanged and ineligible;
none of their audit timestamps was repurposed as a duration. Replay is validated
separately and is not part of these timed intervals. See [authorization/boundary](AUTHORIZATION.md).

Host: Apple M2 Mac14,2, eight logical CPUs, 8 GiB, macOS 26.5.2 build 25F84 /
Darwin 25.5.0 ARM64, APFS. Python 3.14.3. Existing AppleClang 21.0.0 C++23 binaries,
`-O3 -DNDEBUG`, strict deterministic float ON; llama.cpp/TUI/benchmarks/sanitizers/
tritwise profiling OFF; correctness probe ON. No rebuild or optimization occurred.
All three binary SHA-256 values match the candidate validation report. Cache/build
rules, linked library versions, loaded Python-module hashes, executable hashes,
fixture bytes and source hashes are retained. OS shared-cache library bytes are
not independently archived. `mach_absolute_time()` reports a 41.67 ns clock resolution;
nanosecond records do not imply nanosecond measurement accuracy.

Collection UTC interval: 2026-10-28T18:23:41.538697+00:00
to 2026-10-28T18:24:46.201196+00:00 (2:23–2:24 p.m. America/New_York).
Seed **6281028** shuffled five AB/five BA orders before timing, independently of the
frozen 360-pair schedule. No priming, cache flush, warm process or inter-pair delay.
No benchmark/build/test was launched concurrently by this agent during collection.
Other host activity was not controlled: load averages were 3.858/4.433/3.846 before
and 4.080/4.355/3.855 after. `pmset` reported AC power and a discharging 83% battery
at both endpoints; this does not establish stable CPU power. Thermal queries exited
zero but printed unavailable-status errors, retained verbatim. Sandboxed `sysctl`
was denied; a separate read-only invocation verified hardware identity and is
recorded in `host-identity.json`. Idle host, thermal state and absence of suspension
were not independently established. Linux, Windows and x86 execution are untested.

## All raw paired timings — EXPLORATORY — NONCONFIRMATORY

Exact integer nanoseconds; ratios are A/B, rounded here only for display.
No slow observation was excluded or replaced. Full clocks, UTC timestamps,
commands, output, store paths and identities are in `observations.json` and the
append-only journal in `raw-run.tar.gz`.

| Pair | Execution order | A elapsed ns | B elapsed ns | Paired A/B |
| --- | --- | ---: | ---: | ---: |
| 1 | B→A | 6271216333 | 111920250 | 56.032901 |
| 2 | A→B | 6254128875 | 107112417 | 58.388458 |
| 3 | B→A | 6190348000 | 102685375 | 60.284612 |
| 4 | A→B | 6371921833 | 110627125 | 57.598187 |
| 5 | A→B | 6281763792 | 110089542 | 57.060495 |
| 6 | B→A | 6211024125 | 102049709 | 60.862732 |
| 7 | B→A | 6296959833 | 144483625 | 43.582516 |
| 8 | A→B | 6212230209 | 138151750 | 44.966714 |
| 9 | A→B | 6419738875 | 110533708 | 58.079467 |
| 10 | B→A | 6214186375 | 103782250 | 59.877160 |

| Arm | Median ms | Min–max ms | Sample SD ms | IQR ms | MAD ms | CV |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A | 6262.672604 | 6190.348000–6419.738875 | 74.645961 | 80.441572 | 49.464312 | 1.19% |
| B | 110.311625 | 102.049709–144.483625 | 14.825025 | 6.982177 | 4.864292 | 12.99% |

Median paired latency ratio: **57.838827 A/B**
(arithmetic median of the ten within-pair ratios, not the ratio of arm medians).
Ratio range 43.582516–60.862732;
SD 6.196822, IQR 3.215185,
MAD 1.922129, CV 11.13%.
IQR uses inclusive interpolated quartiles; SD uses n−1. No confirmatory analyzer,
confidence interval, significance test or frozen 10% classification was applied.

## Correctness and identity evidence

- Fresh accepted seven-method gate: **7 passed, zero skips**, 246.130 seconds
  (correctness-suite duration, not a performance endpoint). All 15 valid scenarios
  and their intermediate/candidate checks are recorded in `gate-scenarios.json`.
- Fresh matched recovery/tamper and denial/invalid-model checks: **2 passed,
  zero skips**, 20.055 seconds. Original fixtures were deleted before new-process
  recovery; hash-valid false evidence and byte tampering were rejected. Deliberate
  negative attempts are retained separately under `recovery-validation/` in the archive.
- Collection: **10 complete equivalent pairs, 20 successful workers, zero failed
  measurement attempts**, 20 distinct initially empty stores. All 140 logical role
  readbacks passed. Common result: ALLOW / GREETING_PAIR / token 42; candidate scores
  `[16,16,13,16,16,8,16,16]` for IDs 42–49 agree. Policy evidence is allowed and strict
  deterministic. There is no claim of arbitrary model or continuous-policy equivalence.
- Offline archive verification independently rehashes every one of those 140 stored
  role objects and checks common refs, journal-clock consistency and summary medians.
  All raw fixture/store bytes, including each backend's provenance/bundle, are retained.
  `artifact-identities.json` exposes all refs and input SHA-256 identities.
- Existing protocol, schedule, statistics, comparator, apparatus, production sources
  and historical records remain byte-identical to the candidate. The frozen schedule
  SHA-256 remains `b9f3fdee3424840dd10c54932f42185df7d20213bb730cf4464cf136289aa5ef`.
  No existing execution lock was removed and no confirmatory entry point was invoked.

## Scientific interpretation

1. **Did the matched workflow execute?** Yes, on this one input/host with all seven
   obligations, correct common evidence and stable artifact identities in every pair.
2. **Stable enough to justify confirmatory collection?** A was relatively stable
   (CV 1.19%); B varied more (CV 12.99%), particularly pairs 7–8. The observed ordering
   was consistent across all ten pairs, but ratio variability and uncontrolled host
   activity prevent declaring the six-cell confirmatory measurement regime validated.
   The observations justify independent review of a confirmatory *workflow-cost*
   experiment; they do not authorize starting it or establish its sampling assumptions.
3. **What costs appear to dominate?** A requires **13 nested CLI launches per timed
   worker**: one execution, five puts and seven gets, versus zero nested CLI launches
   in B. Repeated CLI initialization, temporary files, serialization, filesystem I/O
   and synchronous diagnostic logging are leading explanations for A's roughly
   6.3-second workflow. Their individual shares, including how much is VM computation,
   are unmeasured; no comparable internal phase clocks or observer-control experiment
   exists here. Do not infer isolated startup/compute costs from these aggregates.
   Pure-Python versus optimized C++ and different interface launch counts also
   confound architectural interpretation. Empty stores do not imply a cold OS cache.
4. **What prevents the 360-pair experiment?** No host/binary/correctness blocker was
   found for this bounded path. Independent Jules apparatus/statistical review,
   a frozen/reviewed actual confirmatory timing collector, and separate explicit
   confirmatory authorization remain missing. This exploratory-only wrapper cannot
   run that schedule. Host power/thermal/activity control also remains unresolved;
   the one-minute sample does not validate a four-hour measurement regime.
5. **Smallest next action?** Have Jules review the candidate apparatus and this
   observed timing boundary, including launch/logging overhead and host-control gaps.
   Then freeze/review only the minimal confirmatory collector and obtain separate
   authorization before executing the unchanged schedule. No production optimization
   or comparator change is warranted by this packet alone.

The observed difference describes these instrumented implementations on binary
hardware. It establishes no architectural or ternary-specific causality, general
speed ranking, native-device claim, Investigation 08 closure or change to H-S1's
historical inconclusive disposition. Negative observations are retained as evidence.

## Reproduction

Run from the repository root at this PR's evidence commit. Offline verification
needs only Python's standard library and launches no workload:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/verify.py
```

To inspect every retained attempt/store without running an experiment:

```sh
mkdir /tmp/i08-exploratory-inspect
tar -xzf experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/raw-run.tar.gz -C /tmp/i08-exploratory-inspect
```

Actual premeasurement checks (environment variables are explicit):

```sh
T81_BUILD_DIR=/private/tmp/t81-pr627-review/acceptance-build \
T81_PARITY_REPORT=/private/tmp/i08-new-gate-scenarios.json \
python3 -B experiments/integrated_system_advantage/test_differential_parity.py
T81_BUILD_DIR=/private/tmp/t81-pr627-review/acceptance-build \
T81_VALIDATION_EVIDENCE_DIR=/private/tmp/i08-new-recovery-journals \
python3 -B experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py \
  RealWorkflowTests.test_files_removed_before_new_process_recovery_and_tamper \
  RealWorkflowTests.test_policy_denial_and_invalid_model_leave_no_objects
```

Actual collection command (stdout/stderr redirected to the retained `collection.log`):

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/collect.py \
  --build-dir /private/tmp/t81-pr627-review/acceptance-build \
  --out /private/tmp/t81-i08-exploratory/experiments/integrated_system_advantage/controlled_measurement/exploratory_2026_10_28/run
```

For a separately authorized reproduction use a new nonexistent output directory;
the wrapper refuses overwrite. It enforces the three candidate-validated executable
hashes and freezes sources/dependencies/fixtures before launch, checking them between
arms. The validated binaries remain on this host; this packet archives hashes and
build metadata, not the executables. A rebuild on another toolchain/path is not
promised byte-identical and requires renewed validation and a separately identified
packet. Baseline build instructions remain in the unchanged preregistration;
no fabricated rebuild command/result is claimed. Every repeat is exploratory and
must remain separate from both this run and the future preregistered dataset.

`raw-run.tar.gz` contains all original run files plus fresh recovery-validation
journals; `manifest.json` lists exact SHA-256 values for packet files and every
archived member. Absolute original paths are provenance, not extraction requirements.
PR #628 must remain draft and unmerged. No governance or unrelated engineering
correction accompanies this evidence.
