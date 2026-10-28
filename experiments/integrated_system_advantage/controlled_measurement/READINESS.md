# Investigation 08 measurement apparatus — review handoff

The maintainer conditionally accepted the preregistered numerical design and
authorized preparation/validation only. **No confirmatory execution is authorized.**
Update this same draft PR #628; do not merge it or open another experiment PR.
The next gate is independent Jules review of the apparatus/statistics, followed
by separate explicit measurement authorization. This implementation was written
and tested by the same Codex author: it is not that independent review.

## Files and responsibility

- `apparatus.py`: extends the accepted `AblationHarness` for validation-only
  matched workflows. It calls `ConventionalTaskRunner` unchanged for B and the
  existing `t81` public CLI for A. No numerical comparator is reimplemented.
- `schedule.py`, `schedule.json`: fixed seed 62708; 60 blocks, six cells/block,
  exactly 360 pairs; each cell has 30 A→B and 30 B→A orders. The canonical frozen
  file SHA-256 is `b9f3fdee3424840dd10c54932f42185df7d20213bb730cf4464cf136289aa5ef`.
- `analysis.py`: offline, imports neither the comparator nor apparatus. It
  independently checks the frozen schedule identity, exact rows/order and N,
  rejects incomplete/unverified data, and implements the registered statistics.
- `test_apparatus.py`: synthetic/statistical and real correctness checks. The
  real checks use the accepted fixture generator and existing binaries.
- `validation/`: actual correctness-test outputs, source/binary identities and
  validation-only attempt records. These are not performance observations.

Accepted comparator, original worker/harness/runner, seven-method gate, production
sources, baseline audit, historical measurements and research conclusions are
unchanged. The new code is research orchestration under the existing experiment.

## Matched seven-object contract

Both arms persist model bytes, normalized input, exact APL, reversible exact
tokenizer text, result, their own provenance and a recovery bundle. Each worker
then reads and hash-verifies all seven logical objects, checks result/model/input/
policy linkage, task/mode, selection and common evidence, and reports role refs.
The identity verifier uses SHA3-512/truncation and integer Base-81 conversion,
independent of the comparator's encoding routine.

The first five role refs (model/input/policy/tokenizer/result) must match across
arms. Provenance and bundle may differ by backend; obligations and common
evidence agree. Logical role bytes, unique payload bytes, actual file bytes and
allocated filesystem bytes are distinguished. Deduplication does not remove
logical obligations; allocation is null if unsupported.

T81's missing input/bundle persistence uses `canonfs put-file`; readback uses
`canonfs get --out` with read verification enabled. B retains its accepted CAS
and execution implementation. Denials and invalid models return before the
additional input persistence. Each pair uses fresh independent stores; populated
conditions run an explicit priming workflow outside the validation pair.

Lookup and replay are separate worker operations. Replay accepts only a store
and bundle, never original model/policy/tokenizer/input overrides. A reconstructs
temporary fixtures from verified CAS bytes and invokes the accepted CLI; B uses
the accepted `verify_and_replay` implementation. Both must reproduce their own
result/provenance/bundle references. Tests delete original fixtures before new
process recovery. No continuous VM-policy equivalence is claimed.

## Attempt retention and execution boundary

The journal appends a launch-start record and fsyncs it **before spawning**.
Launch completion retains exit code and lossless stdout/stderr as Base64.
Spawn errors, nonzero exits, timeouts and incomplete pairs retain their records;
a completed first arm is not lost when the second fails. Each record identifies
its context, purpose, process, UTC/monotonic timestamp and unique event/attempt.
POSIX locking serializes coordinator/awaited-worker appends. Files and the newly
created journal directory are fsynced. A truncated record is retained and blocks
continuation. Journal write/fsync failure prevents another launch.

Outer workers get their own process group; nested CLI processes inherit it so
outer timeout/interruption kills that group. Nested public CLI timeout kills
the CLI. Stores/journals assume a trusted local sequential owner; this is not
hostile-host, concurrent distributed logging, or power-loss certification.

Every implemented entry point is explicitly `instrumentation_validation` with
`confirmatory=false` and `measurement_eligible=false`. There is **no batch
measurement entry point or execution of the frozen 360-pair schedule**. No
comparative `elapsed_ns` observation is emitted. Audit timestamps and test-suite
duration exist to diagnose correctness; they must not be repurposed as timings.
The analyzer rejects validation-purpose inputs. Synthetic durations exist only
inside unit tests and cannot receive an eligible scientific claim.

This deliberate execution lock means later measurement authorization must also
freeze/review the collector's actual timing boundary before collection. Do not
substitute the older `run_experiments.py` command: its endpoint is unmatched.
Public CLI launch counts and durable instrumentation costs differ across arms;
the eventual result concerns the specified instrumented workflow, not isolated
VM arithmetic. No internal phase costs are inferred by subtraction.

## Statistical implementation and overall disposition

Sort exact paired latency ratios A/B as `Fraction`s. Monotonicity of log/exp
makes the interval endpoints exactly ratios at 1-based ranks **20 and 41** for
N=60. The central point estimate is the geometric mean of the middle two ratios,
equivalent to exp(median(log ratio)). Display rounding cannot change a decision:
strict directional and inclusive equivalence boundaries use `10/11` and `11/10`
as exact fractions. The exact integer binomial calculation is checked with an
independent Pascal-triangle oracle in the tests.

All six cells must have the **same supported result** for an overall supported
direction/equivalence result. Any unresolved cell, mixed advantage/disadvantage,
or mixture with equivalence yields overall **INCONCLUSIVE**. No selected cell or
pooled mean can override that rule. The test suite checks all 4^6 combinations.
The serial-dependence and environment-change warnings suppress the affected
cell's classification; their absence does not establish IID block effects.
Simultaneous coverage remains conditional on the preregistered assumptions.

## Reproducible correctness commands

From the repository root, using the Python standard library:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py
T81_BUILD_DIR=/absolute/path/to/accepted-build python3 -B \
  experiments/integrated_system_advantage/controlled_measurement/test_apparatus.py
```

Without `T81_BUILD_DIR`, three real-workflow methods are explicitly skipped; a
unit-only pass is not an integration pass. The supplied build must contain
`t81`, `t81_make_assess_fixed_demo`, and `t81_investigation08_probe` from the
accepted build configuration. Original build/gate commands remain in
[PREREGISTRATION.md](PREREGISTRATION.md); no production rebuild was needed here.

To preserve real-test raw validation records outside the checkout, set
`T81_VALIDATION_EVIDENCE_DIR` to a **nonexistent** output directory. To validate
one pair manually (still not a timing sample):

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/apparatus.py validate \
  --build-dir /absolute/path/to/accepted-build --work-dir /tmp/i08-validation \
  --condition populated_store
```

To reproduce the schedule into a new file, without launching any workload:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/schedule.py \
  --out /tmp/i08-schedule-new.json
```

Offline analysis accepts a JSON object with `purpose`, `schedule_sha256`, and
chronological `pairs`; each pair has frozen cell/block/order, `status=complete`,
`equivalent=true` and verified A/B objects with positive integer `elapsed_ns`.
Future real data additionally needs a separate measurement-authorization record.
The analyzer is not an authorization system and cannot authenticate a supplied
record. Its CLI is `analysis.py --input DATA.json --out NEW.json`; `--synthetic`
requires `purpose=synthetic_unit_test` on the dataset and every row, and produces
`eligible_for_scientific_claim=false`. No synthetic sample result is committed
as measured evidence.

## Limits and next gate

The journal implementation is POSIX-only and has been exercised on macOS ARM64;
Windows is rejected explicitly, and Linux execution is not claimed. True warm
processes, comparable internal phase clocks, optimized-baseline competence,
concurrent security/durability and causal compositional/ternary attribution
remain outside the demonstrated scope. Allocation bytes are filesystem-specific.

Jules should independently inspect/reproduce confidence ranks, exact thresholds,
overall rules, chronological schedule checking, fault retention, byte identity,
fixture-free replay, and instrumentation bias. Only after that review and a
separate explicit measurement authorization can confirmatory collection begin.
No new scientific disposition or merger authorization is issued by this packet.
