# Investigation 08 — phase costs and observer effects

**Disposition: INCONCLUSIVE for architectural performance or assurance superiority.**
The bounded OFF/ON semantic gate passed. This report explains measured costs of a
specific implementation; it does not close Investigation 08 or establish limits
of native ternary hardware. No optimization was performed.

## Baseline, environment and evidence

Main was `c930b8c2ee7478782bf3c6d231bdd59984595ef2`; #602 was unmerged at
`7c61afa3ba0e97c3c800ca5cde567cf374d7a5cc`. This work is a separate stacked
branch. Instrumentation and the primary protocol were pinned before collection
at **`e2d605db6777973355a319deed345df64a0449b2`**.

Hardware: Apple M2, 8 logical CPUs, 8 GiB RAM, Mac14,2; macOS 26.5.2 ARM64.
Compiler: `/usr/bin/c++`, AppleClang 21.0.0; CMake 4.3.0, Ninja 1.13.2,
Python 3.14.3. Both Release builds use `-O3 -DNDEBUG` plus the existing global
`-O3 -DNDEBUG` policy, C++23 and existing deterministic build settings. The only
T81 Boolean option difference is `T81_ENABLE_INVESTIGATION08_TIMING` OFF/ON.
Full CMake caches, binary hashes, source identities, hardware metadata and
commands/logs are preserved in [the evidence manifest](phase-evidence-2026-10-27/manifest.json).

The model is the **2,037-byte** assess-fixed demo, not a production LLM. Its full
file SHA3-512 begins `f84c8b2d521d7019` and ends `9d161ae0c70c925`; full model,
input, policy, result and provenance identities are in every observation. Input
is `greet hello`; admission is the explicit one-model/task-marker APL fixture.
The probe still constructs its existing default interpreter engine; this is not
a test of every APL constraint or a universal authorization boundary.

See the [call-path map](PHASE_CALL_PATH.md), [methodology](PHASE_METHODOLOGY.md),
[raw observations](phase-evidence-2026-10-27/observations.json),
[same-binary recording control](phase-evidence-2026-10-27/recording-control.json),
and [recomputed summary](phase-evidence-2026-10-27/summary.json).
Prior #602 and earlier negative/inconclusive results were not overwritten.

## Semantic noninterference and accounting

Both builds passed **eight unittest methods**, including the same **15 valid
scenarios** as #602, malformed models/policies, valid nonsymmetric weight mutations,
BPE/tokenizer changes, binary identity/tamper tests, recovery and store isolation.
The extra method checks accounting with deliberately invalid negative controls.
OFF took 317.641 s; ON took 317.891 s. These corpus durations are validation runtime,
not controlled performance measurements.

All 15 OFF/ON fingerprints match: intermediate tensors, candidate scores/IDs,
selected output, policy outcome, model/input/policy identities, exact result and
provenance references, verification and independent conventional replay. Each
scenario also repeats the full T81 CLI in a fresh process with identical output.
All **96 gate sidecars** pass independent parent/child, chronology, inclusive,
exclusive and root-total checks. Five measured triples also match identities and
candidate vectors, and all recorded artifact fetches/replays verify successfully.
OFF ignores the timing environment variable; ON runs idle without it; an unusable
timing output path does not change the version command's output/exit status.

Application costs below are sums of **exclusive** spans. For every record, the
sum equals the application root exactly; no inclusive parent is added again.
Inclusive enclosing measurements are explicitly labeled. Startup/shutdown remain
null, and the parent wall-minus-application residual is not relabeled as startup.

## Fresh-process observations

Five randomized triples, seed 608; each arm gets a fresh process and distinct,
initially empty store. All initial-entry counts are zero; none of the arm roots
are reused. Stores are fresh, but CPU/page caches are not flushed. No affinity,
thermal, power or background-load isolation is claimed.

| Implementation / mode      | Mean wall ms | Sample SD ms |          Range ms |
| -------------------------- | -----------: | -----------: | ----------------: |
| T81 OFF build              |     5986.432 |        9.743 | 5978.766–6003.441 |
| T81 ON, recording          |     5854.420 |        4.812 | 5846.377–5857.701 |
| Conventional Python worker |      105.571 |        7.215 |   100.757–118.290 |

These are implementation observations, **not a fair architectural speed ranking**.
Python loops/hashlib and C++/SIMD/VM/compiler/canonical-state work are not comparably
optimized conditions. Persistence contents, state evidence and replay guarantees
also differ. Bounded parity does not establish every architectural guarantee.

### Partition of the instrumented application window

The ON application window averages **5843.019 ms**. Its parent process wall is
5854.420 ms, leaving a **derived combined boundary residual of 11.401 ms**.
That residual includes loader/pre-main work, activation overhead, recorder output,
final teardown, pipes and scheduling; no standalone startup/shutdown estimate is made.

| Exclusive measured component               |  Mean ms | Share of application |  Sample SD ms |
| ------------------------------------------ | -------: | -------------------: | ------------: |
| Attention kernel calls                     | 3980.711 |              68.128% |         5.246 |
| State serialization for signatures         |  768.599 |              13.154% |         0.828 |
| Context vector addition                    |  598.613 |              10.245% |         1.615 |
| Candidate accumulation/scoring instruction |  486.563 |               8.327% |         1.138 |
| All other exclusive spans combined         |    8.533 |               0.146% | See raw spans |

The residual row is derived by subtraction from the checked root partition, not
assigned to one hypothesized cause. Individual smaller measured exclusive spans
include unscoped VM work 2.455 ms, task residual 1.784 ms, compilation/binding
1.441 ms and driver object persistence 1.206 ms. Very short spans are near clock
and observer overhead scales and should not be overinterpreted.

Enclosing **inclusive** values, shown separately and not added to the table:
VM execution 5068.687 ms; program construction including compilation 1.625 ms;
model loading including file read/parsing 0.114 ms; policy admission including
file fingerprinting/APL parsing 0.371 ms. Per-instruction VM policy evaluation
adds 0.156 ms in its own scopes. VM initialization is 0.154 ms. These small toy-
fixture values do not establish loading or governance costs for large models or
complex policies. VM-exclusive residual is not an isolated dispatch benchmark.

There are **three** attention-kernel invocations in every measured execution,
averaging 1349.480, 1316.186 and 1315.046 ms in chronological order. The source
contains the two layer calls plus an additional `attn1_export` attention call in
`vm/decoder.cpp`. This is concrete repeated implementation work. It is not proof
that removing it preserves tensor-pool, transition or provenance identity; no
such optimization was attempted.

### Verification, storage and replay outside initial execution

Two fresh ON CLI artifact fetches together average **18.699 ms wall**. Their
inner driver read/integrity spans sum to **0.121 ms**. This distinguishes a
subprocess-based inspection interface from its underlying read/check operation;
external semantic verification is checked but not isolated as a separate kernel.

A fresh T81 replay command averages 5973.283 ms OFF and 5875.032 ms ON. It repeats
compilation, VM creation, computation and output work using original fixture
files; it is not a cache lookup or standalone model recovery from result CAS.
The conventional worker's independent persisted-input recovery/re-execution
averages 80.377 ms process wall. Those are different recovery interfaces and
must not be used to declare assurance or replay superiority.

The conventional worker's separately observed mean phases are fixture loading
0.321 ms, policy evaluation 0.239 ms, model materialization 0.130 ms, computation
1.444 ms and persistence 2.611 ms. Module initialization is 22.650 ms and its
parent/worker boundary residual is 51.689 ms. These do not exhaust its wall time;
argument parsing, worker setup, replay/output serialization and other work remain
outside those particular phase scopes. They are not directly interchangeable
with all T81 phase definitions.

### Warm native process

With a loaded model reused for four native API invocations in one ON process,
measured probe windows are **5898.643, 5898.574, 5894.561 and 5892.345 ms**.
Compilation/binding repeats (2.896, 0.558, 0.583, 0.528 ms), as does VM setup
(0.120, 0.019, 0.017, 0.019 ms). Outputs remain equal within repetitions and
between OFF/ON. The large probe costs persist with the model already loaded;
this workload's seconds are not explained by model file reads alone.

The research adapter performs additional stage validation after those windows.
Its whole wall time must not be divided by four to invent an OFF per-call time.
There is no warmed full-task CLI or persistent VM in this protocol, and no
conventional warmed-process architectural comparison is claimed.

## Instrumentation overhead and observer perturbation

The separate-build ON-minus-OFF paired wall difference averages **−132.011 ms**,
SD 14.388 ms, range −157.064 to −121.359 ms. All five values are negative.
This is a **net build-plus-recording perturbation**, not negative clock cost or
an architectural speedup. Adding scopes can affect generated code and execution
conditions; these data do not isolate which mechanism caused that difference.
The measured partition applies directly to the ON executable, not an assumed
identical OFF phase distribution.

After observing that result, an additional exploratory control used the **same
ON binary** with recording idle versus active, five randomized pairs, seed 609.
Active-minus-idle differences in ms are **−7.242, +24.148, +26.022, −60.638,
+31.418**: mean **+2.742 ms**, median +24.148 ms, sample SD **38.533 ms**.
This sample does not resolve a small positive recording cost reliably. Allocation,
scheduling, thermal and cache effects remain; it is not a pure clock-call
microbenchmark. The control's binary hash is checked against the successful gate.

No observation was excluded. Post-collection descriptive 3×MAD flags identify
primary paired-difference index 4 and recording-control indices 0 and 3; these
are retained tail observations, not grounds for significance or exclusion claims.
Five observations/pairs are exploratory and insufficient for strong statistical
inference. No fixed per-scope overhead is fabricated.

## Attribution: what is known and what remains open

| Evidence class                     | Conclusion                                                                                                                                                                                                                                                                                                    |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Directly measured                  | Most ON time is inside attention, vector addition, candidate accumulation and state serialization. CLI parsing, small-model parsing, bounded admission and object writes do not explain the seconds in this fixture.                                                                                          |
| Source-backed implementation facts | Three attention calls include an explicit export recomputation. Serialization calls `tensor.serialize`, which can materialize canonical fixed caches (`include/t81/tensor.hpp`); state-signature hashing occurs after that timed serialization. These are concrete software paths and representation choices. |
| Hypotheses, not isolated causes    | Fixed/canonical conversion, generic arithmetic and allocation may explain expensive operator/serialization spans. This timing tree does not separate their contributions or prove that binary-host ternary emulation alone dominates. Build-codegen effects may contribute to observer perturbation.          |
| Derived estimates                  | Root/wall boundary residual, phase shares, means/SDs and ON-minus-OFF contrasts follow from retained timestamps. They are not separately measured OS startup or pure recorder cost.                                                                                                                           |
| Unknown                            | Standalone loader/shutdown costs; within-kernel division/softmax/conversion/allocation shares; a precise recording-cost estimate; optimized matched controls; large-model behavior; other platforms; native ternary devices.                                                                                  |

Some policy, identity, execution and serialization work is required by the chosen
contracts. **This does not make the observed magnitudes intrinsic to T81 or to
ternary computation.** Specific precision, caches, repeated export operations,
process interfaces and runtime implementation are engineering choices. No
architectural lower bound or native-hardware limit is derived here. Existing
hypothesis IDs and governance classifications are unchanged.

## Next discriminating experiment

Use the same pinned tensor values and existing serialization API to separate
first canonical-cache materialization from steady serialization. Charge explicit
cache/setup costs, compare cold-cache, prepared-cache and full setup-plus-serialize
costs, and require byte-identical serialized state, signatures and task outputs.
A lack of a material first/steady difference would falsify the cache-materialization
explanation; a lower steady time without lower setup-plus-work cost would not
establish an end-to-end gain. Isolate one attention operation similarly only after
this smaller discriminator. Do not remove export work or optimize this PR.

A later architectural comparison still needs competent optimized conventional
controls with the same representation/precision, authorization, evidence,
persistence and replay guarantees, including setup/conversion costs and valid
ablations. Until then, Investigation 08 remains **INCONCLUSIVE**.

## Validation and reproduction

Primary runner (exit 0):

```sh
python3 experiments/integrated_system_advantage/phase_experiment.py --off-build /tmp/t81-pr601-review/build --on-build /tmp/t81-phase-on --observations 5 --warm-repeats 4 --seed 608 --work-dir /tmp/t81-phase-review/runs --out /tmp/t81-phase-review/phase-observations.json
```

Same-binary control (exit 0):

```sh
python3 experiments/integrated_system_advantage/phase_recording_control.py --gate-packet /tmp/t81-phase-review/phase-observations.json --on-build /tmp/t81-phase-on --pairs 5 --seed 609 --work-dir /tmp/t81-phase-review/runs --out /tmp/t81-phase-review/recording-control.json
```

Recompute the descriptive summary from committed raw data:

```sh
python3 experiments/integrated_system_advantage/summarize_phase_costs.py experiments/integrated_system_advantage/phase-evidence-2026-10-27/observations.json
```

Both variants built the CLI, demo builder and probe. Targeted `t81_vm_trace_test`
and `t81_vm_predispatch_policy_deny_logging_test` pass 2/2 on each build, with
explicit checks active despite global NDEBUG. Freeze integrity passes its bounded
209-entry checks; the legacy v1.1.0 script banner is not a new ISA-version claim.
Python compilation, added-file formatting, whitespace, artifact hashes and local
links are checked. Protected specs, public headers, charters, governance records
and workflows are unchanged. No full CTest, Linux/Windows/x86-64 execution,
hardware measurement, generalized model-quality test, or CI-green claim is made.
