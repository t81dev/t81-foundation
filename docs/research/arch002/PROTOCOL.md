# T81-ARCH-002 — Packed-trit equality/reduction preregistration

**Disposition: GO for a subsequent, separately authorized correctness-only
implementation.** No experiment, pilot, benchmark, semantic change or merge is
authorized by this document. Performance sections are design only. Status:
proposed; date: 2026-11-05; source baseline:
`d134407ae1059c4d7f6a9418177e695e0e5d3b51`.

Parent: canonical **H03**, with H05 Boolean-equivalence context; ARCH-002 does
not replace either hypothesis. This is a synthetic representation/kernel-cost
discriminator, not evidence of application relevance, native ternary hardware,
or processor superiority. A conventional encoder can emit identical bytes.
Codex authored ARCH-001 and this protocol with the same context; model identity
is unestablished/null. No blinded or independent-person replication is claimed.
Jules's ISA-002 work, Axion, governance and frozen CPA evidence are excluded.
See [proposal](proposal.json) and [documentation validation](VALIDATION.md).

## 1. Reuse and non-duplication gate

All paths/line anchors in this table refer to the baseline above. Existing
implementations are evidence and reusable interfaces, not independently
validated behavior in this task. Do not edit them or weaken their tests.

| Exact reusable source/function | Existing question; boundary for ARCH-002 |
|---|---|
| [ComputeTritVector::from_trits/from_packed/to_trits](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/include/t81/experimental/packed_trit_vector.hpp#L203), stable aliases in `include/t81/packed_trit_vector.hpp` | Canonical compute packing, validation and conversion; no equality-count API found. Use public aliases in future adapters. |
| [pack_pt5/unpack_pt5](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/codec/trit_packing.cpp#L70); `PackedTritVector::from_packed/to_trits` | Reuse valid PT5 encoders. Decoder accepts extra bytes and does not enforce unused-digit canonicality: apply the experiment's strict envelope outside production code. |
| [T81::EncodeTrit/DecodeTrit/UnpackDigits](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/include/t81/native.hpp#L33) | Different two-bit fields and positional arithmetic. Its decoder aliases code 11 to +1; this is not compute-vector decoding. Native adapter is correctness-only here. |
| `include/t81/support/bitopts.hpp::popcount64`, `include/t81/simd/simd.hpp` feature helpers | Eligible scalar population count and compile-time backend information; not proof of runtime feature access or optimized code generation. |
| [test_backend_equivalence](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/tests/cpp/test_tritwise_backend_equivalence.cpp#L34); `tests/cpp/test_packed_trit_vector.cpp::{test_phase2a_from_packed_validation,test_trailing_byte_masking,test_cross_representation_consistency}` | Reuse boundary cases and byte-identity ideas; independently specify equality oracle. Existing tests concern logic/conversion, not this count. Do not rerun all kernels to establish feasibility. |
| [BM_MaskApply_*, BM_PatternMatch_*, BM_CanonPipeline_*, BM_TensorLane_*](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/benchmarks/BM_TritwiseWorkloads.cpp#L24); `BM_PackedTritVector.cpp::{BM_ComputePack,BM_ComputeUnpack,BM_Kernel_TAnd_*}` | Min/max/negation pipelines, conversion and allocation costs; not a scalar count of corresponding equal symbols in two immutable arrays. Reuse infrastructure only after auditing timing boundaries and effective checks. |
| `experiments/h04_r03_matched_codec/SPECIFICATION.md`, §§1–5; H04-R02 inventory/results | Related codec/access work over 729-state elements; constructive binary-clone control already defeats exclusive density claims. Does not close this three-state equality/reduction question. No adaptive policy is tested here. |
| [AIOPT M2 adversarial assessment, §§1–2](https://github.com/t81dev/t81-foundation/blob/d134407ae1059c4d7f6a9418177e695e0e5d3b51/docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md#L16) | Related wildcard catalog filtering and mandatory bit-sliced controls. This experiment has no wildcards, fixed catalog, match-list output or repeated indexed queries. Preserve the critique; equality is not uniquely ternary algebra. |
| `experiments/radix_comparison/`, `experiments/packed_ternary_matvec/`, `docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md` | Preserve scalar-arithmetic, MatVec and matched-overlay negative/qualified findings. No instruction expansion or reopened frozen optimization campaign. |
| `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json` (H03/H05); `docs/research/commons/manifest.v1.json` | Canonical identities and curated evidence, not a live exhaustive experiment registry. H05 compares equivalent Boolean functions; H03 charges packing/access/conversion. |

Inspection used tracked-main path/content searches for `count_equal`,
`equality_count`, `equal_positions`, `hamming`, `popcount`, equality/reduction
phrases and ARCH-001/002; inspected the benchmark functions above, experiment
specifications, register and Commons records. Remote PR searches for equality
and ARCH-002 found related work but no matching protocol closing this question.
**No equivalent completed equality-count experiment was identified in that
scope.** This is not an all-history/all-unpublished-branches assertion. If an
independent reviewer locates an equivalent accepted protocol/results packet
before implementation, **STOP**, link it, and reuse it rather than execute a
duplicate. [VALIDATION.md](VALIDATION.md) retains search/identity checks.

## 2. Mathematical operation and experimental API contract

Inputs are two immutable, ordered arrays `a,b` of the same length N over
`{-1,0,+1}`. Elements are independent symbols, not positional integer digits.
Output is `C = sum([a[i] == b[i]], i=0..N-1)` as unsigned **uint64**, with
`0 <= C <= N`. Empty arrays return exactly zero. No approximate equality,
wildcards, reorderings, saturation, carry propagation or early “any equal” exit.

The standalone adapter accepts one named format, two byte spans and two
uint64 lengths. Mixed formats are rejected; representation transformations are
tested separately. Valid N is 0..2^24 inclusive. This is a resource/domain limit,
not the mathematical or uint64 count limit. Payloads are disjoint, immutable
buffers; identical values do not imply shared addresses. Input generation and
kernel execution have separate APIs; kernels receive no seeds/distributions,
expected counts, pair-specific summaries or catalog/index metadata.

Return exactly `(status,count)`: `status=OK` and a uint64 count on success;
otherwise `count=null`. Error precedence: `ERR_FORMAT` (unknown/mixed format),
`ERR_LENGTH` (N_a != N_b), `ERR_LIMIT` (N>2^24), `ERR_PAYLOAD_LENGTH` (not exactly
the required bytes), `ERR_SYMBOL` (invalid active symbol/code), `ERR_PADDING`
(noncanonical unused digits/bits). Check active symbols of A then B before
padding of A then B. No count is released for partially valid input. Diagnostic
strings/error positions are non-governed and not comparison outputs.

All primary implementations expose the same checked contract. Validation may
fuse with counting, but must cover both complete inputs before returning OK.
Payload length arithmetic uses checked unsigned calculations. N=0 requires
zero payload bytes in every format; no dereference is allowed. SIMD loads must
not overread allocation boundaries; use bounded tails, never assume guard bytes.

Each checked result is consumed by the same opaque, separately compiled harness
sink, which incorporates status and count into a rolling uint64 digest modulo
2^64. Record the final digest and per-block expected counts. No output-only
pointer observation or unused result is sufficient. Disable interprocedural/LTO
optimization across the timing-call boundary, retain full optimization inside
kernels, and inspect assembly to rule out count reuse/dead-code elimination.

## 3. Canonical representations and transformations

All formats encode increasing logical indices from low fields/bits to high.
Sizes exclude the identical in-memory descriptor (format, N, span lengths);
record its actual size separately. No ISA/container/wire format is introduced.

| Format | Canonical payload and validation |
|---|---|
| BYTE | N bytes: `FF=-1`, `00=0`, `01=+1`; reject every other byte. |
| C2 (ComputeTritVector) | `ceil(N/4)` bytes; slot j at bits `2j..2j+1`: `00=0,01=+1,11=-1`; reject active `10`. Unused fields must be `00`. |
| PT5 | `ceil(N/5)` bytes; `b=sum((t[j]+1)*3^j,j=0..4)`, 0..242. Reject 243..255 as ERR_SYMBOL, including a partial final byte. Missing trailing trits must be zero, hence their base-3 digits must be 1. Exact re-encoding of the decoded N symbols must reproduce the payload. |
| NATIVE (optional correctness adapter) | `32*ceil(N/128)` bytes of 32-byte blocks; four fields/byte, `00=-1,01=0,10=+1`; active `11` rejected by this experiment even though production DecodeTrit aliases it. Unused fields through the final block encode zero (`01`, whole zero-padding bytes `55`). Do not call native arithmetic. |
| PLANES | Positive plane followed by negative plane, each `ceil(N/8)` bytes. Bit i%8 of byte i/8 marks +1 or -1; neither means zero; both set on an active position is ERR_SYMBOL. All unused bits in both final bytes are zero. |

The independent BYTE-to-format transform follows these equations, validates
source symbols and emits canonical padding. Reverse transformation decodes
exactly N active symbols and validates the entire envelope. Cross-format
conversion goes through mathematical symbols, not reinterpret_cast or a shared
lookup. Existing production converters may be reused as candidates, **not as
the sole oracle**. The strict PT5/native envelope is experimental admission,
not a correction, relaxation or promotion of existing production semantics.

Payloads are N, ceil(N/4), ceil(N/5), 32*ceil(N/128), and 2*ceil(N/8) bytes
respectively. PT5's asymptotic 20% reduction from C2 is arithmetic, not a speed
result. Headers, tables, scratch, alignment and live buffers are additional.

## 4. Independent oracle and correctness case specification

Oracle author writes explicit scalar mathematical decoders from §3 without
calling optimized paths, production packers, popcount, SIMD, or candidate LUTs.
After envelope validation, accumulate `uint64(a_symbol == b_symbol)` in index
order. For small cases cross-check a separately structured oracle: count the
three diagonal entries of the 3x3 joint symbol histogram. Candidate-produced
expected counts/hashes cannot define expected truth.

Mandatory cases, no new execution authorized now:

1. All nine ordered symbol pairs, plus empty payloads; verify exact counts and
   every format's zero representation. Same values in separately allocated
   buffers; equality at first/last position; all equal; no equal; one mismatch.
2. Exhaust all ordered vector pairs for N=1..5 (the N=5 family has 59,049 pairs).
   Encode independently; compare all formats with both scalar formulations.
   This covers every valid PT5 group pair and every valid C2 group pair.
3. At each N in `{0,1,2,3,4,5,7,8,9,15,16,17,31,32,33,63,64,65,127,128,129,
   255,256,257,1023,1024,1025,4097}`, test constants, alternating/cycling symbols,
   one matching position at each boundary, and seeded independent vectors.
   Test pointer offsets 0..31 with exact-length allocations; test canaries and
   ASan/UBSan separately from any later timing. Include unequal lengths and
   domain limit N=2^24+1 without allocating its advertised payload.
4. Try all 256 single-byte encodings for BYTE, C2 and PT5 in operand A and B
   against a valid peer; enumerate all forbidden fields/bit-plane overlaps and
   all unused-slot mutations at partial lengths. Extra/truncated payloads,
   nonzero C2/plane padding, nonzero PT5 unused trits, native code 11/padding
   mutations, mixed faults and precedence must produce the specified error.
5. Seeded correctness vectors: seeds j=0..31, both distributions below, the
   boundary lengths above. Also validate N=2^20 and 2^24 once per distribution
   with j=0 using streaming or compact-byte oracles, not Python integer lists.
6. Build with explicit Debug/Release flags including `-O3 -DNDEBUG`. Checks must
   remain active. Mutate one computed count by +1 (N=1, a=b=[0]) and require
   nonzero exit in both modes. A test-driver mutation that suppresses its main
   comparison must also be detected by that deliberately wrong result. No
   `assert`-disabled “pass”; preserve commands, binary/source hashes and exits.
   An external always-active supervisor independently checks the child output
   against the anchor count 1: count 2 must fail even if the child suppresses
   its own comparison and exits zero. Seal agreed dual-oracle expectations and
   hashes before candidate execution; never repair them to match its output.

Fixed generator for later authorized implementation: SplitMix64 with uint64
wrap, initial state S; on each draw add `9E3779B97F4A7C15`, then
`z=(z xor (z>>30))*BF58476D1CE4E5B9`, then
`z=(z xor (z>>27))*94D049BB133111EB`, return `z xor (z>>31)`.
Draw bounded r in `[0,m)` by rejecting u>=`2^64-(2^64 mod m)`, then r=u mod m.
Generate interleaved a[i], b[i] from successive accepted draws:

- Uniform: m=3; r=0,1,2 maps to -1,0,+1.
- Skewed: m=20; r=0..17 maps to 0, r=18 to -1, r=19 to +1 (0.90/0.05/0.05).
- `S=0x4152434830303200 + phase*2^32 + endpoint*2^24 + j`, modulo 2^64.
  Endpoint 0=(2^20,U), 1=(2^20,S), 2=(2^24,U), 3=(2^24,S).
  Phases: 0 correctness, 1 control-selection, 2 variance pilot, 3 confirmation,
  4 resident secondary, 5 batch calibration. For boundary correctness use endpoint 0/1 by
  distribution and j as above; generator prefixes are intentionally reusable.

Oracle results and canonical input hashes are sealed before timed execution;
the seed/phase receipt is not delivered to kernels. Inputs are immutable per
invocation. Do not infer actual count from the distributions' expected match
probabilities (1/3 and 0.815). Retain every mismatch; stop scientific acceptance
on the first confirmed error. Repeated passing runs are not independent-person
validation.

## 5. Comparator qualification and cost boundaries

| Arm | Qualification and role |
|---|---|
| T-C2 candidate | One fixed checked C2 equality/count implementation, using the public representation. No PT5-vs-C2 candidate selection after seeing results. Backend/dispatch fixed in source before any pilot; no candidate tuning during selection. |
| B-BYTE | Optimized byte equality + widening reduction and validation; native NEON on M2, auto-vectorized loop also eligible. A scalar-only byte baseline is insufficient. |
| B-PLANES | `valid_mask & ~((Pa xor Pb) | (Na xor Nb))`, population count and uint64 sum; validate no active P&N overlap. Permit scalar hardware popcount and NEON byte-count/reduction variants, with justified ISA availability. |
| B-C2 | Conventional two-bit symbol equality/reduction over the **same C2 bytes**, independently implemented; permit word/SIMD masks, nibble LUTs and popcount. This same-byte control prevents attributing library tuning to ternary exclusivity. |
| B-PT5 | Conventional base-243 symbol groups; eligible 243x243 table of equality counts for full groups (59,049 one-byte entries), with scalar partial-group handling, validation, table construction and memory charged. This is the same logical alphabet; do not exclude it as “ternary.” |
| NATIVE | Correctness/encoding compatibility only; not a fifth performance endpoint or a proxy for positional addition. |

All four controls are mandatory on M2. Before pilot, each must pass §4 and have
source/assembly records confirming intended ISA and non-eliminated work. Permit
all above implementations to optimize the same operation, including fused
validation; prohibit expected-count metadata, pair preprocessing outside the
charged boundary, or layout-only reinterpretation. If a control cannot be
qualified, performance is blocked, not measured against a weaker replacement.

**Primary: single-use byte-input end-to-end.** Each timed invocation receives
the same pre-generated immutable BYTE arrays. Time starts before validation,
representation allocation/conversion and table initialization, and ends after
counting, common sink consumption, scratch/payload destruction and deallocation.
BYTE may operate directly, with no artificial conversion/allocation. Packed
arms must materialize their declared encoding; record its digest outside timing
on an equivalent verification invocation. No metadata about pair equality is
created outside timing. Runtime-generated lookup tables are constructed inside
each invocation; compile-time tables are permitted but charge their binary size
and record whether they are compile-time or constructed. No cold disk/process
launch is timed. Library one-time initialization belongs inside the first call;
any persistent cache must be reset between invocations or be a declared static
table. Correctness output must stay identical if any table/cache is absent.

**Secondary: resident-packed.** Start with validated canonical arrays, supplied
in each arm's declared format, no pair metadata. Time the same checked function
(including revalidation), dispatch, count and sink; storage conversion/table
preparation stays separately reported, never hidden. No confirmatory speed
decision from this mode; it requires its own later approved schedule. In both
modes separately itemize bytes, allocations, conversion, validation, table and
dispatch costs without subtracting “overhead” from the primary elapsed time.
Optional instrumented attribution is not mixed with clean timing.

Repeated calls use an opaque separately compiled boundary, so immutable inputs
cannot justify compiler-hoisted counts. No multiplication of an already computed
answer. Hashing input arrays and diagnostic serialization are outside timing;
the common count/status sink is inside. Reset/pre-touch the same source arrays
before each timed batch; no cache-flush claims. Report working sets, not assumed
L1/DRAM residency. Include read/write traffic and cache behavior as explanatory
measurements only when counter access is actually available.

## 6. Host qualification and fixed future execution conditions

Primary target is user-specified 2022 MacBook Air M2, 8 GB RAM, 256 GB SSD.
This is a target identity, not a newly collected hardware receipt. Checklist
before any performance authorization:

- Capture model identifier, architecture/CPU features (e.g. `sysctl`), OS/kernel,
  core topology, available RAM, native/Rosetta/emulation status and virtualization.
- Record compiler path/version/hash, target triple, exact flags/link commands,
  executable hash, enabled backend and disassembly. No `-ffast-math` relevance
  is assumed; operation is exact integer. Feature macros alone are not a host
  capability receipt. Never execute unavailable instructions.
- One process and one worker thread; BLAS/OpenMP thread pools disabled. Record
  scheduling/QoS and migration evidence where available; do not claim physical
  core pinning on macOS without evidence. Do not pool P/E-core or host identities
  as independent architectural measurements.
- AC power, Low Power Mode off, no intentional concurrent workload, sleep
  disabled for the bounded run. Record battery/power and available thermal
  pressure state before/after each block. Do not require privileged monitoring.
- Record process baseline/peak RSS, OS pressure, swap-in/out counters and free
  storage before/after. No compressed-memory/swap-dependent timing. OS thermal
  warnings, pressure above nominal, sleep, swap activity, lost output or cap
  breach invalidate the campaign for confirmation; preserve it as inconclusive,
  not a deletable outlier. If required states cannot be observed, performance
  qualification is blocked. No automatic retry campaign.
- Record timer source/resolution; use a monotonic elapsed timer. Record PMU/
  hardware-counter permissions. Lack of counters permits qualified elapsed-time
  results but no cycle/cache-miss/energy claim. CPU features do not measure speed.

Jules's Linux container remains **UNKNOWN** until CPU, ISA/OS-enabled state,
container/VM/emulator, vCPU quotas/affinity/steal, physical-host visibility,
compiler, memory and PMU access are recorded. It may supply labeled correctness
results; no native M2 timing or pooled primary inference follows. Any Linux/x86
timing study requires a separate preregistration with a new comparison family.

## 7. Performance preregistration — DESIGN ONLY

Four primary endpoints are the two fixed sizes x two fixed distributions in §4,
on qualified native M2 only, using §5 end-to-end costs. Let T_B and T_C be positive
batch times per operation for a pilot-selected control and T-C2. Within paired
block j define `D_ej=ln(T_B/T_C)`. Primary estimand is
`theta_e=E[D_ej]` over the frozen generator/block-order schedule and qualified
run conditions; exp(theta_e) is a geometric-mean paired time ratio. This is not
ratio of separate arithmetic means, a best run or a pooled endpoint average.

**Threshold clarification:** “at least 10% improvement” means **10% lower
latency**, T_C/T_B<=0.90, so `theta0=ln(10/9)`. A 1.10 speedup is only 9.09%
lower latency and would weaken ARCH-001's stated time criterion. Success requires
each endpoint's simultaneous lower bound strictly above theta0 (conservative
at the exact boundary). No density-only or resident-only success substitutes.

**Pilot separation:** after correctness and separate performance approval:

1. Phase 1: j=0..7, all qualified control variants and fixed candidate; same
   inputs within an endpoint/block. For each endpoint select the control with
   lowest geometric mean end-to-end time. Exact ties use B-BYTE, B-PLANES, B-C2,
   B-PT5, then lexical variant ID. Freeze control IDs, source hashes, candidate
   dispatch and timing repetition counts; publish the full pilot. No pilot
   observations enter confidence intervals. This comparison is against the
   pilot-selected control, not a proof it is globally fastest.
2. Phase 2: j=0..15, selected control/candidate only, for variance planning.
   Freeze n once, before any phase-3 outcomes. Review log-ratio independence/
   stationarity and a normal-model diagnostic receipt. If lag-1 absolute sample
   autocorrelation exceeds 0.30, Shapiro-Wilk p<0.01, drift/host qualification fails,
   or sample SD is zero, mark pilot inconclusive and stop; do not switch analysis
   methods or tune the kernel to rescue it. Passing diagnostics does not prove
   the assumptions; the confidence/power claims remain conditional on them.
3. Phase 3: j=0..n-1, fresh phase-3 seeds only. Both arms get identical logical
   arrays. Never inspect significance to stop.

In phases 1,2,3, randomize each block with a separate SplitMix stream initialized
to `0x4152434830324F52 + phase*2^32 + j`. Fisher-Yates-shuffle endpoint IDs
`[0,1,2,3]` with bounded rejection, visiting indices 3 down to 1. Then, in
endpoint-ID order, phase 1 shuffles the full frozen arm/variant roster (initial
lexical ID order); phases 2/3 draw r in [0,2), using candidate/control order for
r=0 and control/candidate for r=1. Execute in the shuffled endpoint order with
those local arm orders. Calibration uses lexical arm order and no inference.
No data-generation stream is shared with ordering.

The diagnostic lag-1 statistic is Pearson correlation of consecutive pairs
`(D[0:n-1],D[1:n])`; zero variance is invalid. Define drift as an absolute
difference greater than `ln(1.05)` between the mean of the first floor(n/2)
ratios and the mean of the remaining ratios, in time order. Use these exact
diagnostics and thresholds in both phase 2 and confirmation. No discretionary
visual rejection or undocumented stationarity test may replace them.

The 64-block cap applies to confirmation, as in ARCH-001; the fixed 8+16 pilot
blocks and calibration are additional but share the single 30-minute/resource
cap. No new campaign or extra pilot blocks may be added automatically.

Before phase-1 collection, calibrate power-of-two repetition count R_e using
phase-5, j=0 data: double R from 1 until the shortest batch across the candidate
and all preregistered qualified control variants lasts >=20 ms, cap R at 2^20.
Freeze that common R_e for all arms in selection and all subsequent phases.
Record calibration separately; it cannot select a winner or enter inference.
Each repetition performs a complete single-use invocation; no answer
reuse or amortized packing. If cap cannot reach 20 ms, stop inconclusive. Use
one untimed full invocation per arm followed by the source pre-touch before each
timed batch; count this work toward resource caps. Each block consists of all
four endpoint pairs. Timer/allocator states are part of the observed workload.

**Coherent power target:** 90% power cannot be promised at theta=theta0: that
is the null boundary. Plan at **20% lower latency**, `theta1=ln(1.25)`, retaining
the 10% decision threshold and 64-block cap. This explicit alternative is a
planning assumption, not an expected benefit. To target at least 90% probability
of passing all four endpoints, require per-endpoint modeled power >=0.975;
the union bound limits total miss probability to 0.10 without endpoint independence.

Under the stated independent normal log-ratio model, with phase-2 SD s_e and
df=15, use conservative variance planning:
`sigmaU_e=sqrt(15*s_e^2 / chi2_quantile(0.0125,15))`.
Select the smallest common integer n in [20,64] for which every endpoint has
`P(noncentral_t(n-1, sqrt(n)*(theta1-theta0)/sigmaU_e)
   > t_quantile(0.99375,n-1)) >= 0.975`.
If none exists, **INCONCLUSIVE_POWER_CAP**, no confirmation. Conditional pilot
variance uncertainty prevents an unconditional 90% guarantee; report this.
Freeze the numerical package/version and independent verification of quantiles
before pilot (e.g. SciPy t/chi2/nct with survival functions); do not estimate n
from confirmatory data. No numerical power calculation or pilot ran here.

At confirmation compute the two-sided interval
`[L_e,U_e]=mean(D_e) +/- t_quantile(0.99375,n-1)*sd(D_e)/sqrt(n)`.
These four 98.75% intervals give >=95% simultaneous coverage under the model
by Bonferroni. **PASS_COST_IN_SCOPE** requires all L_e>theta0, correctness
and every host/resource gate. The same intervals govern every threshold decision;
do not choose a different one-sided or two-sided family after seeing data.
If an upper bound <=theta0, label that endpoint **NOT_SUPPORTED_AT_THRESHOLD**;
otherwise a failed lower-bound criterion is **INCONCLUSIVE_PRECISION**. Do not
call an inconclusive result proof of equality. Report all endpoints/intervals.
Confirmation diagnostics failing the same phase-2 thresholds make the campaign
inconclusive; no alternative bootstrap or selective run deletion afterward.

References for the design (not newly executed statistics):
[NIST Bonferroni method](https://itl.nist.gov/div898/handbook/prc/section4/prc473.htm),
[NIST t statistic](https://itl.nist.gov/div898/software/dataplot/refman1/auxillar/t_test.htm),
[SciPy noncentral-t functions](https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.nct.html).
An independent statistical reviewer must approve the model/planning receipt
before performance authorization. Correctness implementation does not depend
on a favorable power result.

## 8. Resource feasibility, risks and next gate

Caps apply separately to any future authorized correctness or full pilot-plus-
confirmation campaign: one process/thread; <=512 MiB additional RSS above an
idle-process baseline; <=30 minutes elapsed from campaign start (generation,
oracle, warmup and analysis included); <=100 MiB total retained output. Do not
restart the clock at each phase. No SSD input/output is inside timing; no
swap-dependent runs or large executable/trace dumps.

At N=2^24, two immutable BYTE sources use 32 MiB; two C2/PLANES payloads use
about 8 MiB, two PT5 payloads about 6.4 MiB; two NATIVE payloads use 8 MiB.
Keep only one arm's working pair live, <=64 MiB scratch and <=64 MiB additional
source/verification copies, <=1 MiB tables, <=128 MiB runtime/analysis reserve:
modeled headroom is below 300 MiB. This is arithmetic feasibility on an 8 GB
machine, not measured RSS or proof of available memory. Use compact byte buffers
and streaming oracles; never materialize per-element Python objects/JSON.
Do not retain all input arrays: preserve generator/specification, seeds, exact
input hashes, per-case expected/observed counts/statuses, all raw block times,
binary/source identities and host events. Retain bounded mismatch slices and
their indices; inputs are exactly regenerable. Reserve <=20 MiB numeric logs,
<=10 MiB host/commands, <=10 MiB diagnostics and <=10 MiB manifests/source diffs;
stop safely before 100 MiB. Overflow/cap breaches yield inconclusive evidence,
never silent truncation or a weakened experiment.

| Risk / falsifier | Required disposition |
|---|---|
| Duplicate accepted equality-count packet discovered | STOP and reuse; do not silently rename the experiment. |
| Wrong count, acceptance of invalid code/padding, ineffective Release control | FAIL_CORRECTNESS; no timing authorization. |
| Strong byte/bit-plane/same-byte control missing | BLOCK comparator qualification, not a weak-baseline pass. |
| Same bytes/algorithm can be conventionally encoded | Report implementation/packing effects only; no exclusive ternary claim. |
| Win disappears after encoding/validation/allocation | End-to-end threshold not supported; resident result remains secondary. |
| Model, power cap, timer, thermal, swap, budget or evidence gate fails | Preserve all outputs as inconclusive; no auto-retry, relaxed threshold or removed sample. |
| Generalization to CPA, ISA, application benefit or physical energy | Unsupported by this protocol; requires a distinct authorized question. |

**Next authorization gate:** independent maintainer review of this protocol,
non-duplication finding and admission rules, followed by explicit commit-scoped
authorization for isolated correctness-only adapters, oracle, test harness and
the cases in §4 under §8 caps. This draft grants none. No production changes,
timed calls, pilot-selection runs, performance CI, ISA-002 work or historical
artifact edits belong to that implementation. After correctness review, a
separate host/comparator/statistical qualification and explicit performance
authorization are required. Do not merge this PR automatically.
