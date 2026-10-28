# Investigation 08 CLI diagnosis: EXPLORATORY — NONCONFIRMATORY

**Final disposition: CLI OVERHEAD CHARACTERIZED.** The latency is concentrated
in the single execution CLI, not the twelve additional persistence/readback
launches. This characterizes measured command boundaries; it does not isolate
process initialization or identify an architectural cause.

OpenAI Codex, October 28, 2026. Existing draft PR #628; preceding evidence
`96db9bb4a758ab6d9efa46356bc0aa91ff94e477`. Underlying apparatus/comparator source
candidate remains `05639a677b11ea1f4ef0328b3f67140784a4ead1`, using the accepted
#627 executable lineage `0662e0e637f47168e56ca4ddd66e8ee877473b38`.
The earlier exploratory packet is unchanged and verifies successfully. All new
records are permanently excluded from the frozen 360-pair dataset. No protocol,
schedule, accepted comparator, production or governance file changed; no
confirmatory path was enabled/invoked and no new PR or merge was requested.

## Method and successful execution

Ten randomized paired diagnostic observations, `greet hello`, empty independent
stores, fresh processes. Seed **6281029** fixes five AB/five BA orders before
measurement. Ten separate `t81 --version` controls follow the task pairs. All
30 outer launches succeeded; each A worker made exactly 13 nested CLI calls,
each B worker zero. All 140 logical role objects independently verify, common
results/evidence agree, and **every role ref for each backend matches the prior
exploratory packet**. ALLOW / GREETING_PAIR / token 42 and common candidate
scores agree. No failed diagnostic task/control attempt was discarded or replaced.

Measurement phase: **65.057794 seconds**, under the 30-minute cap,
from 2026-10-28T18:36:59.584939+00:00 to 2026-10-28T18:38:04.689529+00:00
(2:36–2:38 p.m. America/New_York). Four recorder selfchecks exercised success,
nonzero/lossless output, spawn failure and timeout. Their deliberate failures are
retained separately and are not task/control performance observations. The prior
accepted seven-method gate and matched recovery/denial checks were not rerun:
the prior packet, source and binary hashes were verified unchanged. This sprint
checks instrumentation and matched task execution rather than claiming new
independent numerical/comparator validation.

## The 13-call path and clocks

[EXECUTION_TRACE.md](EXECUTION_TRACE.md) maps every command, argv template, input,
output, filesystem/CAS obligation, process boundary and corresponding conventional
in-process operation. Exact instantiated argv lists and parent/child PIDs are in
`per-command-timings.jsonl` and the original lossless launch journals.

The new `worker.py` imports the unchanged apparatus. It proxies only that module's
subprocess namespace and wraps its original launch function. Original command
arguments, environment, admission, execution, persistence, readback, verification,
timeout handling and durable launch journals execute unchanged. Each command adds
clock reads and one fsynced diagnostic timing row. No arithmetic or numerical
comparator is reimplemented. `collect.py` extends the previous exploratory wrapper
in a separate directory; it freezes sources/binaries/fixtures before collection
and checks their identities between launches. [AUTHORIZATION.md](AUTHORIZATION.md)
records the user's scope and the premeasurement choices.

- Outer worker time: coordinator immediately before Popen through communicate
  return. Includes worker startup/imports, temporary fixture work, all CLI calls,
  independent byte/semantic verification, journal/observer I/O, output/teardown.
  Excludes store/fixture setup, outer journal writes and parent JSON/parity checks.
- Each nested interval: invoking worker immediately before CLI Popen through
  communicate return; includes process creation, useful command execution,
  stdout/stderr capture, wait and teardown. Original journal and added observer
  fsync occur outside nested clocks, inside the outer interval.
- **All 130 nested intervals are strictly positive, sequential, nonoverlapping
  and contained in their own outer interval.** This is checked before summation
  and independently recomputed offline, not inferred from aggregate durations.
- Per observation: nested total = execution + five puts + seven gets;
  residual = outer − nested total. Residual includes Python startup, fixture reads/
  temporary writes, independent hashes/parsing, directory statistics, original
  journal work, added recorder work, gaps, teardown and scheduling. It is not a
  pure compute or startup measure. Median component values are not additive in
  general; exact per-pair integer accounting appears below.

## Observed decomposition

All summaries below use n=10; ms are display units. SD uses n−1, IQR uses inclusive
interpolated quartiles, MAD is median absolute deviation. Full integer-nanosecond
observations and clocks are retained in `observations.json` and `raw-run.tar.gz`.

| Boundary/category | Median ms | Min–max ms | Sample SD ms | IQR ms | MAD ms | CV |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A outer | 6269.387833 | 6190.725541–6549.970083 | 110.817099 | 70.240791 | 42.771625 | 1.76% |
| A nested total | 6130.507669 | 6066.506792–6418.817626 | 109.483870 | 63.822718 | 34.367333 | 1.78% |
| A execution CLI | 6048.450604 | 5985.315167–6330.616292 | 106.786267 | 64.677604 | 34.774812 | 1.76% |
| A five persistence CLIs | 38.140729 | 36.857332–43.239542 | 2.353474 | 1.548749 | 0.878770 | 6.05% |
| A seven readback CLIs | 44.936146 | 42.115626–49.038126 | 1.810529 | 0.627655 | 0.365353 | 4.03% |
| A outer residual | 131.274207 | 121.071457–156.688872 | 10.023204 | 8.972136 | 4.708543 | 7.60% |
| B outer | 108.893604 | 102.062291–117.954416 | 5.315663 | 7.277478 | 3.466146 | 4.84% |
| Version control (separate) | 6.886604 | 6.402750–7.735125 | 0.472524 | 0.671271 | 0.143583 | 6.70% |

Median within-observation nested share of A wall time:
**97.936348%**; range
97.506250–98.064564%.
The execution CLI alone occupies a median **96.617521%**
of A's outer interval. The twelve puts/gets together have median
**82.643667 ms**, or **1.327268%**
of outer time. These shares are calculated per observation before taking medians.
B's whole worker has no nested CLI intervals; its nested total is zero and its
outer interval remains entirely outside this nested-command decomposition.

### Every pair and accounting residual, exact ns

`Closure` = outer − execution − puts − gets − residual. Zero closure is accounting
consistency, not proof of correctness or evidence of pure startup cost.

| Pair | Order | A outer | Execution | Five puts | Seven gets | Nested total | Outer residual | B outer | Nested share | Closure |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | B→A | 6230905000 | 6024523292 | 37318459 | 45022249 | 6106864000 | 124041000 | 106749292 | 98.009262% | 0 |
| 2 | A→B | 6190725541 | 5985315167 | 39075999 | 42115626 | 6066506792 | 124218749 | 111539917 | 97.993470% | 0 |
| 3 | A→B | 6222327417 | 6012434959 | 38036126 | 44910500 | 6095381585 | 126945832 | 114448333 | 97.959834% | 0 |
| 4 | B→A | 6255513875 | 6054400583 | 37070085 | 42971750 | 6134442418 | 121071457 | 102062291 | 98.064564% | 0 |
| 5 | A→B | 6292313792 | 6074584834 | 36857332 | 44508709 | 6155950875 | 136362917 | 111037917 | 97.832865% | 0 |
| 6 | B→A | 6420227250 | 6193957542 | 43095125 | 49038126 | 6286090793 | 134136457 | 104607625 | 97.910721% | 0 |
| 7 | B→A | 6283261791 | 6042500625 | 38421168 | 45651126 | 6126572919 | 156688872 | 116366042 | 97.506250% | 0 |
| 8 | A→B | 6206309167 | 5989766792 | 38245333 | 45215792 | 6073227917 | 133081250 | 117954416 | 97.855710% | 0 |
| 9 | A→B | 6549970083 | 6330616292 | 43239542 | 44961792 | 6418817626 | 131152457 | 106472000 | 97.997663% | 0 |
| 10 | B→A | 6295512208 | 6081984583 | 37498791 | 44632877 | 6164116251 | 131395957 | 106434334 | 97.912863% | 0 |

All 130 per-command rows retain sequence, context/category, exact argv, invoking
parent PID, child PID, UTC start, start/end ns, elapsed ns, return code and error.
Original journals separately retain prelaunch starts and lossless stdout/stderr.
The archive also retains empty B nested logs, every store object, fixture bytes,
plan, environment readings, complete attempt journal and build metadata.

## Version control: useful negative evidence, not pure startup

`CLI --version` returns through `parse_args`/`print_version` before task/CAS dispatch
in `tools/cli/core/main.cpp::main`. It leaves task stores alone and is intentionally
separate from the matched obligations. The ten successful controls measured
median **6.886604 ms**, range 6.402750–7.735125 ms.
It includes executable/static/dynamic initialization, argument parsing, version
formatting/output, pipe capture, scheduling and teardown; it is **not** an exact
initialization measurement. All controls followed the pairs, so their cache/time
regime is not randomized against the task calls. Controls use the coordinator's
new process session; nested CLIs inherit their worker's process group.

This control and the small measured puts/gets give no support for generic repeated
process initialization accounting for the multi-second latency. Task-specific
initialization inside the execution CLI could still be expensive. No multiple
of the control median is subtracted from task time or interpreted as startup's
exact contribution. No conventional-time subtraction is labeled ternary overhead.

## Answers and smallest next experiment

1. **How much is inside the 13 invocations?** Median **6.130508 s**, occupying a
   median **97.936348%** of outer time. Outer median is **6.269388 s**.
2. **Which categories dominate?** The single execution command: median
   **6.048451 s**. Five persistence calls total **0.038141 s**; seven readback calls
   total **0.044936 s**. The extra twelve calls are about **1.327268%** of outer time.
3. **Does repeated process initialization contribute substantially?** Generic
   startup as the dominant explanation is not supported by these observations.
   Version control is about **0.006887 s** per invocation, and the twelve real
   non-execution calls total only **0.082644 s**. This narrows the earlier packet's
   leading launch-overhead hypothesis; its preserved observation remains valid.
   Task-specific initialization and useful execution remain combined in EXEC.
4. **What remains unattributed?** Median **0.131274 s** lies outside nested clocks.
   More importantly, the **6.048451 s inside EXEC is not causally decomposed**:
   model/tokenizer/policy work, program construction, VM computation, result/
   provenance persistence and command-specific startup are not individually timed.
   Calling only the outer residual 'all unattributed cost' would be misleading.
5. **Can the disadvantage be attributed to orchestration?** Not to the twelve
   additional persistence/readback CLI calls on this host/input. Further isolation
   inside the execution command is required. Eliminating launches would not imply
   eliminating useful command work or the full observed difference. This measures
   implementations and instrumentation, not intrinsic ternary/architectural cost.
6. **Smallest useful next experiment:** externally sample the existing execution
   CLI, with an unsampled fresh-process control and the same pinned fixtures,
   policy and matched workflow, to locate where its roughly six seconds are spent
   (e.g. loading/construction versus VM operators versus persistence). Preserve
   sampling overhead and all failures. This requires no production optimization
   or comparator replacement and would guide any later narrow phase study. It is
   proposed here, not executed or authorized as a continuation of this fixed sample.

## Environment, limits and reproduction

Same validated Apple M2 Mac14,2, eight logical CPUs, 8 GiB; macOS 26.5.2 build
25F84 / Darwin 25.5.0 ARM64, APFS, Python 3.14.3. AppleClang21 C++23 binaries,
`-O3 -DNDEBUG`, deterministic float ON, llama.cpp/TUI/benchmarks/sanitizers/tritwise
profiling OFF. All three binary hashes match the prior validation manifest.
The candidate source, loaded Python dependencies, binaries and generated fixtures
are pinned in archived `run/plan.json`; build cache/rules and linked library
versions are archived. OS shared-cache library bytes are not independently saved.
Clock is monotonic mach_absolute_time with reported 41.67 ns resolution, not an
accuracy guarantee. Nanosecond recordings are exact clock readings, not exact
physical costs.

Load averages: 2.841/2.693/3.113 before; 2.579/2.671/3.071 after. This agent ran no
concurrent build/benchmark/test during measurement; other host activity was not
controlled. Both power readings report AC and 80% battery discharging. Thermal
queries exit zero but return unavailable-status errors, retained verbatim.
Idle host, fixed CPU frequency, thermal stability and absence of suspension are
not certified. No cold-cache claim, internal production clocks, instrumentation
observer-control estimate, energy result, Linux/Windows/x86 replication, IID
claim, confirmatory CI/materiality verdict or scientific-status change is made.
Added diagnostic fsync has unknown perturbation; it lies mostly in outer residual,
but indirect cache/scheduling effects on nested measurements are not ruled out.
The same Codex author prepared and checked this packet; offline verification is
implementation-separated, not independent scientific review.

Run from the repository root. Offline verification uses only Python stdlib,
launches no workloads, imports no collector/worker/apparatus/confirmatory analyzer,
rehashes every packet/archive member and all 140 role objects, checks preserved
candidate/previous packet identities, reconciles original command journals and
130 sequential intervals, and recomputes accounting and medians:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/verify.py
```

Actual recorder check and measurement commands, with logs retained:

```sh
python3 -B experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/check_instrumentation.py \
  /private/tmp/t81-i08-exploratory/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/recorder-checks
python3 -B experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/collect.py \
  --build-dir /private/tmp/t81-pr627-review/acceptance-build \
  --out /private/tmp/t81-i08-exploratory/experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/run
```

Use new nonexistent output paths for a separately authorized reproduction; existing
outputs cannot be overwritten. The collector enforces candidate-validated binary
hashes; executables remain available on this host but are not archived here. A
new-toolchain/path rebuild is not guaranteed byte-identical and requires new
validation and a separately identified packet. No new build was performed.

Inspect all original files without execution:

```sh
mkdir /tmp/i08-cli-diagnostic-inspect
tar -xzf experiments/integrated_system_advantage/controlled_measurement/cli_diagnostic_2026_10_28/raw-run.tar.gz -C /tmp/i08-cli-diagnostic-inspect
```

`manifest.json` lists packet/archive-member SHA-256 identities and preserves the
prior packet and frozen candidate hashes. Earlier validation timestamps have not
been repurposed as timings. PR #628 remains draft and unmerged.
