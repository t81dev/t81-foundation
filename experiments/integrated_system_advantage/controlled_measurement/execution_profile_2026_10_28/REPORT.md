# Execution bottleneck isolation — EXPLORATORY — NONCONFIRMATORY

**Final disposition: EXECUTION BOTTLENECK CHARACTERIZED.**

**Research decision: A. Targeted follow-up justified.** A specific phase dominates
sampled execution: building tensor canonical fixed-point caches from host floats,
with repeated wide ternary integer divisions. This last authorized diagnostic
step is complete. Stop here and return control to the researcher; no follow-up,
optimization, production change or additional measurement is initiated.

OpenAI Codex, October 28, 2026. Prior evidence:
`2d51b7019c4dfd65562d74a69dcb4d47344466bb`; unchanged apparatus source candidate
`05639a677b11ea1f4ef0328b3f67140784a4ead1`; accepted #627 executable lineage
`0662e0e637f47168e56ca4ddd66e8ee877473b38`. PR #628 remains draft and unmerged.
All new observations are exploratory and permanently excluded from the frozen
360-pair dataset. The protocol, schedule, conventional comparator, production
runtime/VM, earlier evidence, governance and H01–H08 history remain unchanged.

## Direct answers

1. **What dominates the six-second execution command?** During each covered
   sampling window, the main thread spends **98.98–99.49% of thread-top samples**
   in `t81::div_mod<384>`. Resolved callers place those divisions under
   `Fixed<192,80>::operator/` → float-to-canonical-fixed conversion → tensor
   canonical-cache construction during VM tensor operations. This is a
   representation-conversion implementation cost, not generic CLI startup.
2. **How strong is the evidence?** The same function-level chain is resolved in
   **all ten profiles**, with 4,024–4,084 main-thread samples each and no `???`
   markers in their call graphs. Source inspection confirms the concrete
   division loop. All 20 executions and all 40 emitted artifact checks passed.
   Evidence strongly localizes the sampled bottleneck; it does not measure exact
   phase durations, prove a removable speedup, or cover the full process lifetime.
3. **Is it ternary computation itself?** It is directly related to **this software
   implementation's binary-float → ternary-fixed representation bridge** on an
   M2 binary CPU. That does not establish an intrinsic cost of balanced ternary,
   the necessity of this conversion algorithm/cache policy, a native-device
   result, or the cost of ternary multiply/attention arithmetic in isolation.
4. **What about the claimed integrated-system advantage?** Earlier matched
   workflow observations show a large implementation-specific latency disadvantage;
   this packet localizes a major sampled cost within that implementation. It
   establishes no integrated performance advantage, architectural necessity or
   ternary-specific superiority, and does not replace H-S1's historical
   inconclusive disposition with a broader falsification. It adds no comparative
   assurance or C–F ablation evidence.
5. **Should diagnostic work stop?** **Yes, this authorized sequence stops now.**
   Decision A recommends only a separately authorized, narrow conversion-cost
   experiment: measure canonical cache construction for the actual tensor values,
   including repeated division-by-two rescaling, against an independent exactness
   oracle. Preserve required deterministic/representation semantics before any
   alternative is compared. No such experiment or optimization was run here.

## Confirmed call path and phase boundaries

These source paths are preserved in the archive. They identify operations;
there are no new phase clocks or claims that every source phase has measurable
sample support. Inlining means the optimized binary need not expose each source
function as a separate frame.

| Phase | Source-confirmed path | Sample evidence / limit |
| --- | --- | --- |
| Command-specific setup | `tools/cli/core/main.cpp::main` → `t81::cli::ai::run` → `cmd_task_assess_fixed` → `run_fixed_label_task`: parse/validate options, select strict reference backend, normalize input. | Outer callers resolved; startup before attach not observed separately. |
| Model/tokenizer loading | `load_weights_model` → `load_weights_model_from_selector` / `_from_path`; `detect_architecture_profile`; `find_model_companion_files`; decoder `lookup_tokenizer_prompt_token_ids`; later `lookup_tokenizer_token_pieces`. | Source-confirmed file/parse work. Individual durations unresolved. |
| Program/VM construction | `run_native_vm_probe` → `_impl` → `build_decode_program` → `build_program_from_source` (Lexer, Parser, SemanticAnalyzer, IRGenerator, BinaryEmitter); `execute_decode_program` → `make_interpreter_vm` → `load_program`. | Source-confirmed generated-program compilation and interpreter construction; no claim they dominate. Some frames are optimized/inlined. |
| Policy evaluation | CLI `parse_policy`, model-hash admission and required task-marker checks precede the probe. VM `Interpreter::step` invokes `eval_axion_call` before dispatch. | CLI APL is not explicitly passed to `make_interpreter_vm()` on this path; VM has a default engine and may consume program policy metadata. Do not infer continuous equivalence to the CLI APL or invent a policy duration. No policy/governance correction is made here. |
| Computational execution | `Interpreter::run_to_halt` → `step` → `tattn`, `ternaccum` and tensor-vector operations → tensor constructors/cache conversion. | All profile windows resolve the VM execution path and the dominant canonical conversion/division chain. |
| Serialization | `interpret_decode_execution`, sampled-logit/transition handling, `materialize_assess_fixed_task`, `render_task_provenance_artifact`, CLI JSON emission. | Source-confirmed; not separately timed. Late work can fall after the profile window. |
| Result/provenance persistence | `write_canonfs_raw_block` → persistent driver `write_object` for two output artifacts. | Both resulting refs/bytes equal earlier evidence; no separately measured I/O duration. |

Dominant resolved sampled chain (some source intermediates are inlined):

```text
main → cli::ai::run → run_fixed_label_task → run_native_vm_probe_impl
→ Interpreter::run_to_halt → Interpreter::step → tensor operation
→ T729TensorBase<float> constructor → initialize_canonical_storage_mode_
→ build_canonical_fixed_cache_ → canonical_fixed_from_float_
→ Fixed<192,80>::operator/ → div_mod<384>
```

`include/t81/tensor.hpp` eagerly builds a canonical cache when constructing a
float tensor. `canonical_fixed_from_float_` extracts binary32 mantissa/exponent;
for negative binary exponents it repeatedly divides a `DFixed` value by two.
For example, source semantics for binary32 1.0 use exponent 127−150 = −23, hence
23 fixed divisions; this is a source-derived example, not a measured call count
for every element. `dmath_types.hpp` defines DFixed as 192 trits / 80 fractional
trits; each division widens to `T81Int<384>`. Its `div_mod` performs restoring
multi-trit integer division. Tensor construction inside attention/accumulation
therefore reaches this cost repeatedly. No conversion counts, exact allocation
counts or counterfactual runtime savings were measured or fabricated.

The covered stacks are arithmetic work rather than blocking I/O/synchronization
stacks, strongly consistent with a CPU-computation bottleneck in the covered
window. No CPU utilization, hardware counters, wait-time accounting or exhaustive
whole-lifetime scheduler trace was collected. Early I/O, initialization and late
persistence remain unresolved rather than assigned zero cost.

## Collection and profiler metadata

Ten profiled (P) and ten unprofiled controls (C), shuffled seed **6281030**, five
P→C and five C→P pairs; all fresh processes and independent empty stores. Same
`greet hello`, exact 2,037-byte model, tokenizer sidecar and APL bytes as the prior
packet. The exact prior execution argv is retained, including model identifier
`investigation08`, strict deterministic mode and each new store root. No executable
was replaced or rebuilt: SHA-256 remains
`1df4bcbb1e100fe9d0e46153c3fe97a6cdf2a67a05c643c54668d8ce9d24c4ba`.

External profiler: `/usr/bin/sample PID 5 1 -mayDie -file REPORT` attached only to
the freshly spawned target. Requested sampling interval **1 ms (nominal 1,000 Hz)**,
requested duration **5 seconds**, early symbol reading with `-mayDie`. Attachment
and symbolization succeeded in all ten attempts using the same user's processes
outside sandbox restrictions. No code injection or production instrumentation.
Profiler executable SHA-256 is pinned in `run/plan.json`; raw reports include
Report Version 7, ARM64, OS/build, binary UUIDs/images and function+offset attribution.
Full profiler stdout/stderr and helper usage output are retained. The separate
`sample -h` usage capture returns 255 (missing PID); this is metadata, not a
failed profiling attempt.

Requested spacing is not calibrated delivered frequency. Reports contain fewer
than 5,000 stack samples; scheduler delay and stack collection overhead prevent
assuming exact 1,000 Hz. No sample count is multiplied by 1 ms to invent elapsed
phase time. The collapsed top-of-stack table counts **leaf samples**, not additive
inclusive frames. Its display omits entries below five; small residual leaves
remain unclassified. There is one sampled thread in each report. Function-level
symbols were sufficient without a debug rebuild; source-line timings, inlined
source boundaries and fine-grained division-loop positions remain unresolved.

The five-second window starts after attachment (first report starts about 105 ms
past process launch). Work before attachment and after the window is not sampled.
Thus the sampled percentages are not percentages of the full six-second command
or of total CPU cycles. The ten identical mechanisms across the windows and
source call paths support localization, not an exact causal decomposition.

Collection: **133.908697 seconds** (under the 30-minute cap),
2026-10-28T18:49:49.825204+00:00 through 2026-10-28T18:52:03.793509+00:00
(2:49–2:52 p.m. America/New_York). No failed task/profiler attempts, retries,
substituted samples or discarded slow runs. The collector would stop on profiler
failure, inadequate application symbols or correctness/identity failure.

This isolates **the execution command**, which emits result/provenance. The twelve
other CLI operations are outside the experiment; no new complete seven-object
workflow timing is claimed. Each of the 40 output role identities was checked
against the prior packet with an independent integer/Base-81 SHA3 identity check;
all results remain ALLOW/GREETING_PAIR/token 42. Frozen model/input/policy linkage
and exact fixture bytes are also rechecked by the offline verifier.

## All raw wall times and sampled evidence

Exact nanoseconds. P/C is within-pair; no frozen materiality threshold, confidence
interval, significance test or architectural classification is applied.

| Pair | Order | Profiled ns | Control ns | P/C | P−C ns |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | C→P | 6581548625 | 6112556667 | 1.076726 | 468991958 |
| 2 | C→P | 6647423834 | 6080189959 | 1.093292 | 567233875 |
| 3 | P→C | 6546706375 | 5984252042 | 1.093989 | 562454333 |
| 4 | C→P | 6478579666 | 6017142166 | 1.076687 | 461437500 |
| 5 | P→C | 6509418167 | 6023214708 | 1.080722 | 486203459 |
| 6 | C→P | 7444853083 | 6057150208 | 1.229102 | 1387702875 |
| 7 | P→C | 7039562166 | 6497984500 | 1.083345 | 541577666 |
| 8 | P→C | 7511487375 | 6678899917 | 1.124659 | 832587458 |
| 9 | C→P | 6988387041 | 6945079792 | 1.006236 | 43307249 |
| 10 | P→C | 7292852042 | 8230481083 | 0.886078 | -937629041 |

| Treatment | Median ms | Min–max ms | Sample SD ms | IQR ms | MAD ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Profiled | 6817.905437 | 6478.579666–7511.487375 | 404.476249 | 674.112636 | 289.843166 |
| Control | 6096.373313 | 5984.252042–8230.481083 | 703.932927 | 601.972480 | 95.676209 |

Median paired P/C: **1.082034**; median paired
P−C: **513.890562 ms**. Nine pairs have P>C,
one P<C. The observed paired median is about **8.20% longer under profiling**;
this is an appreciable disturbance to wall time, not a precise pure-profiler cost.
Profiled/control process medians differ from the median paired effect. No claim
of negligible overhead or statistical materiality is made. The large late control
and rising host load confound exact causal overhead estimation; retain all data.

| Profile | Main-thread samples | div_mod<384> top samples | Observed sample share |
| --- | ---: | ---: | ---: |
| 1 | 4044 | 4008 | 99.1098% |
| 2 | 4046 | 4018 | 99.3080% |
| 3 | 4077 | 4055 | 99.4604% |
| 4 | 4070 | 4034 | 99.1155% |
| 5 | 4084 | 4063 | 99.4858% |
| 6 | 4075 | 4038 | 99.0920% |
| 7 | 4074 | 4042 | 99.2145% |
| 8 | 4024 | 3983 | 98.9811% |
| 9 | 4074 | 4041 | 99.1900% |
| 10 | 4030 | 3998 | 99.2060% |

All full call trees and the top-of-stack tables remain in the archive; the table
above does not sum nested inclusive counts. Source paths plus repeatable stack
attribution support a narrow cost mechanism. No intervention demonstrates how
much time would disappear if cache construction, division or representation were
changed; such a causal claim would exceed this experiment.

## Host and remaining uncertainty

Validated Apple M2 Mac14,2, 8 GiB, eight logical CPUs; macOS 26.5.2 / 25F84,
Darwin 25.5.0 ARM64, Python 3.14.3, existing AppleClang21 C++23 `-O3 -DNDEBUG`
strict-deterministic build. Mach absolute time is monotonic; reported 41.67 ns
clock resolution is not a physical accuracy guarantee. No builds or other
benchmarks/tests were launched concurrently by this agent during collection.

This run was **on battery**, unlike earlier reported AC readings: 77%→76% battery,
and load averages rose 3.193/3.404/3.197 → 5.505/3.942/3.417. Thermal queries say
no warning/performance/CPU-power status has been recorded; that is not thermal or
frequency stability certification. Other host activity was uncontrolled. These
conditions limit latency comparability to earlier packets and profiler-overhead
inference. They do not erase the repeated resolved stack observations. No native
ternary hardware, other input, ARM/x86 comparison, production readiness or general
ternary efficiency claim is made. Same Codex author collected/analyzed this packet;
offline byte verification is not independent scientific review.

## Reproduction and stopping

From this PR's evidence commit, the standard-library offline verifier launches no
process and rehashes the archive, fixture/output identities, previous evidence and
source pins; it reconciles timing/order records and recomputes sample summaries:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/verify.py
```

Actual collection command (stdout/stderr retained in `collection.log`):

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/collect.py \
  --build-dir /private/tmp/t81-pr627-review/acceptance-build \
  --out /private/tmp/t81-i08-exploratory/experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/run
```

Each profiled process uses the exact retained profiler argv above. Reproduction
requires separately authorized execution, new nonexistent output paths, same-user
profiler attachment permissions, the retained validated executable and preceding
packet fixture paths (as pinned in `run/plan.json`). The binary is hash-checked;
it is not archived or rebuilt here. The archive contains all generated run files,
full profiler reports, symbol inventory and relevant unmodified source snapshots.
Absolute original paths identify provenance rather than extraction destinations.

```sh
mkdir /tmp/i08-execution-profile-inspect
tar -xzf experiments/integrated_system_advantage/controlled_measurement/execution_profile_2026_10_28/raw-profile-run.tar.gz -C /tmp/i08-execution-profile-inspect
```

**Stop.** Decision A records a scientifically justified narrow question, not an
instruction to build, optimize, reprofile, expand architecture or start the frozen
experiment. This packet completes the researcher's last authorized diagnostic
step. Control returns to the researcher with prior negative evidence intact.
