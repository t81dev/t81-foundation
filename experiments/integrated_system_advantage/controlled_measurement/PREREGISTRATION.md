# Investigation 08 controlled measurement — proposed preregistration v1

**Status: DESIGN CONDITIONALLY ACCEPTED; apparatus awaits independent Jules
review. Confirmatory execution is NOT AUTHORIZED.**
The maintainer accepted the six cells, N=60/cell, reciprocal 10% margin,
simultaneous interval procedure, four-hour cap and append-only retention as the
planned design, subject to independent statistical validation. Preparation and
instrumentation-validation tests are authorized; measurement requires a separate
explicit authorization after Jules review. See [READINESS.md](READINESS.md).
Prepared October 28, 2026 by OpenAI Codex. This packet requests approval of
prospective criteria and a matched measurement boundary. It contains no new
performance observations, timing analysis, or scientific disposition change.
Approval of a draft PR or a JSON schema check is not experimental authorization.
The remaining "proposed" wording below preserves the candidate's rationale;
the numerical design is conditionally accepted, not permission to collect data.

## Baseline and prior knowledge

The executable/comparator baseline is merged PR #627 at
`0662e0e637f47168e56ca4ddd66e8ee877473b38`, tree
`32c4996e0e9bf7557442da9dd92dcf5f3409cfa9`. GitHub confirmed that merge and
`main` at this revision during preparation. The tree is identical to reviewed
restoration commit `351d88f3d31853e9fc4fb08e56c57b473467fd62`.

The seven-method gate, 13 required tensors and restored root-structure CTest
registration are retained. See [baseline-audit.json](baseline-audit.json) for
source hashes and the acceptance-evidence lineage. Static identity confirms
retention; no fresh parity execution or rebuild is claimed in this packet.

The author reviewed the earlier acceptance findings and historical smoke
reports. This is prospective relative to new confirmatory runs, not a blind
preregistration of those older observations. Earlier measurements cannot enter
this analysis, determine exclusions, or be pooled with the new sample. The
proposed 10% materiality margin below is an engineering proposal requiring human
approval, not an accepted historical threshold or a data-derived cutoff.

No accepted Investigation 08 statistical protocol was found in the inspected
baseline. Existing [runner](../run_experiments.py) defaults to 30 pairs per store
condition, but does not define materiality, confidence intervals or statistical
stopping. [Corrective review](../CORRECTIVE_REVIEW.md) explicitly calls for a
preregistered corpus and uncertainty protocol. Investigation 07's 1.25x area
threshold, Investigation 09's footprint threshold, and other initiative/roadmap
examples are not transferable acceptance criteria for this latency experiment.

Investigation 08 H-S1a–d remain noncanonical work-package questions. This packet
supports canonical H07 without changing H01–H08, H-S1's historical conclusions,
or the Hanoi/CWEB lineage.

## Readiness gates — complete before freezing or timing

1. Retain the maintainer's conditional design acceptance recorded above.
   Independent Jules review must validate the statistical implementation and
   record the candidate hash/commit, followed by separate measurement authorization.
   Amendments before timing need a new version and approval. After any timed
   observation, retain the original protocol and results; changes are a separate
   prospective experiment, never a replacement for an unfavorable run.
2. Freeze the research-only execution adapter and analysis code with their own
   commit/hashes. Reuse the existing harness, comparator, worker and gate;
   do not implement another numerical comparator or change production code.
3. Close the matching-obligation and failure-retention gaps below, using only
   existing public APIs and bounded research orchestration. Validate those
   paths without inspecting comparative timing outcomes. Obtain review of the
   completed boundary before execution. If it cannot be implemented within
   this scope, remain blocked rather than relaxing obligations silently.
4. Run the accepted seven-method gate against the exact frozen executables.
   Require all 15 valid scenarios' nine tensor vectors, candidate IDs/scores,
   tokens, classifications, identity/integrity and authorization checks to pass.
   Independently check a test-owned list of all 13 omitted-tensor negatives;
   verify rejection and zero artifacts in both stores. The comparator must not
   invoke T81VM. Do not use the comparator's mandatory-tensor constant as the
   sole oracle for the negative corpus.
5. Freeze actual executable, dependency, model, tokenizer, policy, input and
   environment identities before the first timing. A hash alone proves bytes,
   not correctness or provenance authenticity. An identity change stops the run.

## Matching obligations and endpoints

The accepted comparator is correct on its declared corpus. That does not make
the current timing boundaries interchangeable:

| Boundary | Baseline capability | Decision for this protocol |
| --- | --- | --- |
| Initial execution | T81 persists result/provenance; comparator also persists model, normalized input, policy, tokenizer and a recovery bundle | Existing A/B wall times are not the confirmatory endpoint unchanged |
| Recovery | T81 timed rerun uses original fixtures; comparator restores CAS inputs | Do not compare those replay times as equal recovery |
| Internal phases | Python clocks exist; T81 phase values are null | Computation, loading, authorization and persistence phase comparisons remain unmeasured |
| Failure capture | Existing runner writes the output after all observations; launch exceptions can abort before that write | Add attempt-level durable raw records before any confirmatory run |

**Proposed primary endpoint:** parent-observed fresh-process wall time of a
complete common task workflow, from immediately before launching one arm's
research worker until it returns a validated response after execution,
persistence and required hash-verified readback. Include worker startup, model/
tokenizer/policy loading, task admission, computation and persistence/readback.
Exclude fixture creation, store setup, predefined population and the separately
timed computational replay. Save command and timestamp boundaries explicitly.

For both arms, the workflow must persist and hash-verify the same seven logical
obligations: exact model bytes; normalized input; exact APL bytes; a reversible
representation of the exact tokenizer text; result; backend-specific execution
provenance; and a recovery descriptor linking those six objects. Match recovery
schema/linkage obligations to the existing comparator bundle. Exact model,
normalized input, policy and recovered tokenizer text must agree. Result bytes
must agree. Provenance/descriptor references may differ because backend evidence
differs; verify each against its own bytes and common semantic requirements.

Use the existing CanonFS public commands for the missing T81 persistence and
readback obligations; keep the accepted Python computation/recovery code intact.
Charge every required subprocess, serialization and storage operation to its
arm. Record the extra API launches: this measures the cost of the selected
implementations and interfaces, not intrinsic VM integration or ternary cost.
If a common descriptor/recovery path cannot meet these obligations without
production changes, the primary comparison remains blocked.

**Eligible secondary endpoints:** common-contract hash-verified artifact lookup;
full recovery/re-execution in a new process after original fixture access is
removed for both arms; logical object bytes; and allocated on-disk bytes where
the same filesystem exposes them reliably. Measure lookup and re-execution in
separate windows. Record payload categories and filesystem metadata separately;
do not compare unmatched bundle footprints as storage efficiency. Recovered
replay must restore the identical model, tokenizer, input and policy, apply the
same admission, and reproduce outputs and evidence linkage.

Secondary endpoints are descriptive and cannot determine the primary verdict.
Report unavailable endpoints as null/unmeasured. Never subtract Python phase
clocks from T81 wall time to invent T81 computational or authorization costs.
At this baseline, default-OFF `T81_TRITWISE_PROFILE` reports operation/byte
counts, not task phase durations; it stays OFF. No default-OFF assess-fixed
phase-clock facility was found. True warm-process execution, separately timed
computation, general continuous-policy equivalence, energy and peak-memory
claims are outside this protocol's measured endpoints.

## Conditions, sample and ordering

Use the existing demo model/tokenizer generator, literal UTF-8 tokenizer bytes,
strict deterministic mode, and original input strings `greet hello`,
`hello world`, and `greet`. For each, use the two existing store conditions:
`empty_store` and `populated_store`. These are six prespecified cells. Unknown,
normalization, tokenizer/weight mutations and policy denial remain correctness
controls, not extra selected performance cells.

Collect exactly **60 paired observations per cell** (360 pairs, 720 measured arm
workflows), exceeding the existing 30-pair planning floor. This fixed sample is
not a power guarantee and will not be enlarged after viewing effects. Each of
60 chronological blocks contains one pair for each of the six cells. Seed
`62708` generates and freezes the full schedule before timing: permute cell
order within each block; for each cell randomly permute a list of 30 A→B and
30 B→A pairs. Preserve the schedule file and SHA-256 before execution.

Every pair/arm gets a unique invocation/condition/block/arm store, initially
verified empty. For populated cells explicitly run exactly one complete common
workflow on each arm's store before the measured pair, outside timing, and
retain its record. Never reuse a store across pairs. Fresh processes are used
for each measured arm, lookup and replay. A populated store is not a warm
process, and an empty store is not a cold OS page cache. No cache-flush, daemon,
or unsupported CPU-affinity claim is made.

Run sequentially on an otherwise idle, AC-powered host; no concurrent benchmark
or build. Use a fixed 10-second idle interval between blocks, recorded with the
schedule; do not vary it after looking at outcomes. Record load/thermal/power
information where available, OS suspension and external interruptions. These
controls do not prove statistical independence.

## Statistical decision rule — proposed, not accepted

For cell c, pair i, define `d_ci = log(T81_ns / conventional_ns)`. The primary
effect is `R_c = exp(median(d_ci))`; values below 1 favor T81 for this workflow.
Compute medians using the mean of the central two sorted log ratios for even N.
Report each arm's median and arithmetic mean latency, sample SD, IQR and MAD;
report paired nanosecond differences and paired ratios, not a ratio of unrelated
unpaired summaries. Keep all 60 raw pairs in their chronological order.

Use a distribution-free order-statistic confidence interval for the population
median log ratio under independent, identically distributed block effects.
For N=60 and six prespecified cells, choose the largest k satisfying

```text
sum(comb(60, j) for j in range(k)) / 2**60 <= 0.05 / (2 * 6)
```

This gives k=20. For each cell the interval is
`[exp(d_(20)), exp(d_(41))]` (1-based order statistics). Bonferroni gives at
least 95% simultaneous coverage across six cells under the stated sampling
assumptions; discreteness gives a familywise error upper bound approximately
0.037306. Ties are retained without jitter. Independent replication across
machines/workloads is not inferred from repetitions on this one machine.

**Proposed materiality margin:** reciprocal multiplicative 10% band
`[1/1.10, 1.10]`. This means at least a 1.10x speedup, or at least 10% extra
latency, is required for a material directional result. The symmetric ratio
band is deliberate; it is not a claim that 10% is a universal engineering law.

| Per-cell interval [L, U] | Disposition |
| --- | --- |
| U < 1/1.10 | Workflow advantage supported |
| L > 1.10 | Workflow disadvantage supported |
| Entire interval inside [1/1.10, 1.10] | Practical equivalence supported |
| Otherwise | Inconclusive |

The single overall directional/equivalence disposition requires all six cells
to meet that same rule. Mixed or unresolved cells give overall INCONCLUSIVE;
report their individual directions without cherry-picking or pooling away
heterogeneity. A statistically detectable effect within the materiality band
does not become a material advantage. Failure to reject a difference does not
establish equivalence. No separate p-value claim is required.

Check chronological plots and report log-ratio autocorrelations at lags 1–5.
As a prespecified conservative warning rule, if any absolute lag correlation
exceeds 0.30, or recorded suspension/power/thermal changes establish a changing
measurement regime, withhold confirmatory classification for the affected cell
and report INCONCLUSIVE. Passing this diagnostic is not proof of independence;
nominal coverage remains conditional and this limitation must accompany results.
Do not switch intervals, thin data, or collect more favorable samples afterward.
Secondary uncertainty summaries are explicitly exploratory and cannot upgrade
an unresolved primary result or establish architectural causality.

## Exclusions, stopping and raw records

Retain every attempted arm, including priming and failed launches, in an
append-only attempt log before proceeding. Include monotonic timestamps,
commands, stdout/stderr or their retained paths/hashes, exit status, store root,
artifact identities, cell/block/order and error reason. Final JSON summarization
must never be the sole copy of previously completed observations.

Do not discard slow observations, trim/winsorize tails, filter by latency, or
replace outliers. A process timeout (180 seconds per launched process), exit
failure, nonpositive clock interval, artifact corruption, wrong identity,
authorization/rejection mismatch or unexplained parity failure stops the entire
invocation. Preserve completed and failed records; do not replace the pair.
Comparative timing for the affected cell is invalid; incomplete fixed samples
do not receive a confirmatory verdict. Restarting requires a separate disclosed
run and approval, not silent continuation until N successes.

Stop after exactly the fixed sample or at a correctness/environment failure or
a four-hour measurement wall-budget cap. Never stop early for apparent
significance or futility. No interim comparative summaries. Budget/suspension
stops retain partial data as nonconfirmatory, not a supported difference.

## Environment, identities and permitted implementation scope

Proposed host: Apple M2, Mac14,2, 8 logical CPUs, 8 GiB RAM, macOS 26.5.2 build
25F84 / Darwin ARM64. Toolchain: explicitly `/usr/bin/clang++`, AppleClang
21.0.0 (`clang-2100.1.1.101`), C++23, repository `-O3 -DNDEBUG`, deterministic
float ON; Python 3.14.3; CMake 4.3.0; Ninja 1.13.2. Keep llama.cpp, TUI,
benchmarks, sanitizers and tritwise profiling OFF for the measurement build.
The opt-in Investigation 08 probe is ON for correctness, not in timed execution.

Before timing, record actual OS/kernel, CPU/RAM, executable and standard-library
identities, filesystem/mount/space, Python runtime, environment allowlist,
build cache/commands, compiler dependencies and linked libraries. Pin all
executables, every harness/comparator source, generated model, raw tokenizer,
APL document, normalized inputs and protocol/schedule/analysis by SHA-256;
retain the existing SHA3/CAS references as separate identities. Never archive
secrets or the full ambient environment. The current proposal records hardware
identity only; it does not invent executable/model identities for a future run.

Implementation after approval is limited to extending the **existing** research
harness/worker/runner for common obligations, append-only failure capture,
schedule/identity manifests, storage accounting and offline analysis. Preserve
the accepted comparator arithmetic/tokenizer/admission/CAS/replay behavior and
all seven gate methods. Use an independent hash/oracle check for manifest
verification; do not turn a comparator's self-report into the only oracle.
No production VM/CLI, governance, historical evidence or hypothesis edits.

## Reproduction and deliverables after approval

Use the baseline's documented narrow build and standalone gate:

```sh
cmake -S . -B build-i08 -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DT81_BUILD_TESTS=OFF -DT81_BUILD_TUI=OFF -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_INVESTIGATION08_PROBE=ON
cmake --build build-i08 --parallel 6 --target \
  t81 t81_make_assess_fixed_demo t81_investigation08_probe
T81_BUILD_DIR=build-i08 python3 experiments/integrated_system_advantage/test_differential_parity.py
```

These build/gate commands were **not run** for this proposal. The current
`run_experiments.py --iterations 60` command does not implement this protocol:
it times one input, different recovery obligations and success-only output.
Do not run it and label those observations protocol-conforming. The exact
extended command and analysis invocation must be frozen and reviewed with the
implementation before collecting data; no nonexistent flags or scripts are
presented here as executable reproduction instructions.

Update this one draft PR with the reviewed adapter/analysis, approved protocol
hash, schedule, gate outputs, all raw attempts, artifact/environment manifest,
logical/allocated storage records, analysis outputs and independent reproduction
instructions. Do not create placeholder results, fabricate raw data, or replace
the older smoke packet. The proposal is the only authorized deliverable at this
stop point; confirmatory artifacts are explicitly not yet available.

## Interpretation and remaining gates

H-S1a: only the matched implementation workflow wall time is eligible for a
directional result; pure computational efficiency remains unmeasured without
matched phase clocks. H-S1b: bounded task authorization and integrity parity
does not demonstrate extra assurance or continuous VM enforcement equivalence.
H-S1c and H-S1d remain INCONCLUSIVE without the absent C–F substitution/ablation
evidence. Pure Python versus optimized C++/SIMD and extra public-API processes
cannot identify an intrinsic architecture or ternary cause. Full Llama inference
and native ternary hardware remain outside scope.

The six cells, 60 pairs/cell, reciprocal 10% margin, simultaneous procedure,
four-hour cap and append-only retention are conditionally accepted as the
planned design. Independent Jules review of the completed apparatus/statistics
and separate explicit measurement authorization remain required. Until those
gates are complete, confirmatory execution stays disabled. This does not reverse
comparator acceptance or imply a measured negative result.
