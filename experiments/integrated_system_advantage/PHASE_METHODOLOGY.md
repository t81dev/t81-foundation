# Investigation 08 phase methodology

Protocol/code checkpoint: `e2d605db6777973355a319deed345df64a0449b2`, stacked on
unmerged PR #602 (`7c61afa3ba0e97c3c800ca5cde567cf374d7a5cc`). See
[the pre-instrumentation call-path map](PHASE_CALL_PATH.md).

## Activation and boundaries

Configure with `-DT81_ENABLE_INVESTIGATION08_TIMING=ON` **and** set
`T81_I08_TIMING_OUT` to a unique, absent sidecar path. Normal builds default OFF;
the OFF header contains no clock, environment or file-recording implementation.
An enabled build without an output path records nothing. Output failures are
non-fatal to the task; the measurement harness treats missing/invalid timing
records as a failed observation. Existing files are not deliberately overwritten.
Use sequential unique paths; concurrent races or hostile path replacement are
outside this research facility's contract.

The private, single-thread recorder uses C++ `steady_clock` (statically checked
monotonic), nanoseconds, relative timestamps and parent IDs. Its application
window starts in `main` after activation/path lookup and ends before sidecar
serialization and final process teardown. All sidecars are outside VM state,
instruction logs, model data, output/provenance/bundle bytes and CanonFS identity.
It is deliberately not a public tracing interface or production default.

For each span, the independent Python validator checks chronology, parent
containment, non-overlapping direct children, inclusive duration from timestamps,
exclusive duration after subtracting direct children, and equality of the sum
of all exclusive durations with the application root. Negative controls must
reject incorrect inclusive/exclusive values, parent IDs and intervals. Summing
inclusive parents and descendants would double-count; only exclusive values are
summed for cost partitioning. Inclusive values may be shown alone to describe a
whole enclosing operation (for example VM execution).

Parent-process `perf_counter_ns` measures complete subprocess wall time. Wall
minus the application window is a **derived combined boundary residual**: loader,
pre-main initialization, activation overhead, recorder emission, final teardown,
pipes and scheduling. Startup and shutdown cannot be separated here and are
explicitly null. An application-exclusive span is measured time not assigned to
its children, not proof that the interpreter or another specific mechanism caused it.

Tensor spans include their existing allocations, conversions and bookkeeping.
Native fast-path attempts are nested inside complete instruction spans, so a
failed fast-path attempt is not confused with a complete operation. Candidate
scoring and token selection are distinct. State serialization/signature work is
distinct from selection. Driver persistence measures identity calculation and
object writes/cache handling; driver construction and task JSON rendering remain
in task-exclusive residual. Driver read/verification includes payload integrity;
external semantic checks verify the results but are not separately timed as a
pure verifier kernel. These are disclosed limits on attribution.

## Semantic gate before observations

Run the predecessor's complete corpus on independently built OFF and ON variants.
The ON corpus gives every CLI/probe process its own sidecar and validates its
accounting. The gate compares all 15 valid scenarios' intermediate tensors,
candidate vectors, selected tokens, model/input/policy identities, result and
provenance references, policy outcomes, verification and independent conventional
replay. Every scenario additionally repeats the full CLI in a fresh process and
requires identical output. All predecessor negative model/policy/binary/tamper
and store-isolation tests still run. A new accounting-negative-control method is
added. A missing binary, failed test, missing sidecar, accounting mismatch or
OFF/ON fingerprint mismatch stops the runner before trial generation.

The gate also checks that an OFF binary ignores the timing path, that an ON
binary can run idle, and that an unusable sidecar path does not change CLI version
results. This establishes bounded empirical noninterference, not behavior under
all resource exhaustion, concurrency, platform or model conditions.

## Observations

Use five randomized OFF/ON/conventional triples, seed 608, each arm in a fresh
process and empty unique store. Record the exact order, initial directory-entry
counts, model/input/policy identities, result identities, candidate vectors and
verification. Each T81 observation separately records execution, two fresh-process
artifact fetches and a fresh CLI re-execution. The conventional worker follows
its existing fresh-process protocol, returning its measured phase times and
independent CAS-input recovery/replay. Those replay guarantees differ and are not
ranked as interchangeable operations.

Four native-probe repetitions in one OFF process and one ON process reuse the
loaded model, check identical numerical output and expose repeated compilation
and VM setup. This is a warmed native API observation, not a warmed full CLI or
persistent VM. There is no supported warmed-process full-task CLI in this protocol.
Do not subtract its time from CLI wall time to invent a startup estimate.

Retain all raw values. Report means, medians, sample standard deviations, ranges
and paired ON-minus-OFF wall differences. Five observations are exploratory, not
strong statistical inference; run order does not control CPU load, temperature,
power management or OS cache state. Fresh stores do not imply cold OS page caches.
Ranges/tails and any descriptive outlier flags must retain every sample; no
outlier exclusion or significance claim is authorized by this protocol.

Record hardware, OS, compiler, optimization, complete CMake caches, Python,
source commit and binary hashes. Freeze the implementation before measurement;
make no optimization during collection. Preserve both earlier inconclusive
measurements and unsuccessful gate evidence. Timing totals are accounting checks,
not independent validation of the clock implementation or scientific truth.

## Reproduction

Build both variants with the same effective C++/optimization options, with examples
and `T81_BUILD_INVESTIGATION08_PROBE=ON`. The only differing T81 option is
`T81_ENABLE_INVESTIGATION08_TIMING`. Then:

```sh
python3 experiments/integrated_system_advantage/phase_experiment.py --off-build /path/to/off-build --on-build /path/to/on-build --observations 5 --warm-repeats 4 --seed 608 --work-dir /tmp/t81-phase-runs --out /tmp/t81-phase-new.json
```

The output path must not already exist. Corpus logs, individual gate sidecars and
fingerprints remain in the invocation directory. The final raw packet embeds
trial phase records, warm-process records, metadata and summaries. Preserve that
packet verbatim with its content identity. The resulting cost report must keep
architectural superiority **INCONCLUSIVE** absent matched optimized controls and
valid ablations; binary-host timings do not bound hypothetical native ternary devices.

## Exploratory recording-cost control added after the primary run

The primary OFF/ON contrast was negative. A follow-up uses the same gated ON
binary, fresh stores/processes, five randomized idle/active pairs and seed 609.
`phase_recording_control.py` requires a successful gate packet and the identical
ON binary hash, then checks equal CLI outputs and validates active timing totals.
This control was added after observing the primary result; it is exploratory,
not a preregistered confirmation. Its raw values, dispersion and retained tails
are reported separately rather than replacing the original contrast.
