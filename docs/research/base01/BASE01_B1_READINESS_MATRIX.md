# BASE-01 B1 readiness matrix — proposed design v0.1

**Design only. No workload is ready for measurement. Authorization is absent.**
This matrix routes future work; its labels neither approve execution nor report
qualification outcomes. Use the [measurement contract](BASE01_B1_MEASUREMENT_CONTRACT.md)
and [host qualification](BASE01_B1_HOST_QUALIFICATION.md). Their immutable source,
Phase A and B0 pins apply unchanged. No Phase A/B0 integration audit, PR change,
benchmark, timer probe, warm-up, PMU, SPICE or protected experiment execution was
performed here. Jules' integration task is separate and neither PR is presumed
merged. The new directory contains only these three proposed design documents.

## Classification meaning

- **READY FOR QUALIFICATION**: a sufficiently specified observation can be
  evaluated after the required authority and environment exist. This is not a
  passing host/workload report or permission to collect evidence now.
- **REQUIRES ENGINEERING**: the scope is defined, but isolated future harness,
  retention, observer or safety work must be reviewed before qualification.
- **BLOCKED**: a scientific/semantic prerequisite is unresolved; writing a timer
  loop does not resolve it. Do not substitute a cleaner workload or metric.

| Approved workload | Classification | Basis and smallest exit criterion |
| --- | --- | --- |
| WL-ARITH-CODEC-I64 | REQUIRES ENGINEERING | Actual allocating encode and pre-encoded decode are separately specified, with exact B0 corpus/oracle. Need reviewed retained-output batching, optimized retention inspection, timer/memory qualification and complete result verification. No allocation-free claim. |
| WL-VM-TISC-DETERMINISM-TIERS | BLOCKED | R75 is a software counter, not yet a qualified executed-instruction denominator. Need an independently checked per-builder ledger of interpreter/trace attempts, side exits, final synchronization and explicit halting. Then bounded fresh-machine chunking and timer qualification are engineering prerequisites. Preserve all six historical builders; do not infer completion from successful run_to_halt alone. |
| WL-CANONFS-MEM-THROUGHPUT | REQUIRES ENGINEERING | Three exact payloads, memory-only read/write scopes and API costs are specified. The A1 protocol-integration candidate fixes 100 reads/writes per cell and proposes 128 MiB retained-output capacity for CanonFS reads only; capacity checks occur after timing. Need independent CanonHash expectations, actual capacity/footprint and containment qualification, bounded streaming evidence, documented first-insertion/allocator state, verification enabled and qualified timing. Persistent/conditional storage is not included. |
| WL-RES-RUSAGE-METRICS | REQUIRES ENGINEERING | B0 checks orchestrator counter invariants only; the VM policy test is not this oracle. Need per-worker lifetime/self/waited-PID accounting, raw units, no-descendant/reaping contracts and normalization/failure checks. No independent timed kernel, incremental-RSS inference or hard RSS enforcement claim. |
| WL-RES-ARTIFACT-SIZE | READY FOR QUALIFICATION | Stat/hash and native section accounting have a defined non-timed scope. With an authorized build inventory, verify actual linked artifacts, tools, clean-build identities, disk versus memory sizes and raw format-specific output. No tool qualification or new footprint measurement occurred here; no ELF/Mach-O equivalence is established. |

All five share blockers to **execution**: no B1 permit, no qualified Linux/x86-64
host, no reviewed activated dispatcher and no qualified Linux measurement build.
Those global blockers do not turn a design-readiness label into measurement
readiness. The full initial campaign requires every cell to become eligible:
14 timed subcases, 30 fresh workers each, with the two observers attached. A
partial campaign requires an explicit amended scope, not dropping the blocked VM.

## Decisions and unresolved questions

| Topic | Design decision | Evidence still required; do not infer |
| --- | --- | --- |
| Source identity | Measure only `cda9135d36144d4de2771af174e52ac7b30a83c9`; instrumentation separately pinned. | Human approval for any revision change; no silent adoption of main or PR #686. |
| Phase references | Phase A `ecd66137bacbce50e605cfc1466ec7d36131ba9e`; accepted B0 `91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e`, historical implementation `54c1a92ffe5c19871c3b1a68c71083ac87fc7f0c`. | References are available as immutable Git objects/links; their merge/order decisions belong to integration, not this design. |
| Arithmetic | Separate API latency with allocating encode and retained results, decode setup excluded. | Optimizer retention, footprint and allocator-profile effect; no population claim from the small deterministic corpus. |
| VM meaning | Report R75 software ticks and completed-program latency with distinct denominators; no hardware/retired-instruction label. | Whether final R75 consistently reflects the chosen attempt-count semantics on all six builders. Source existence/signature repetition is insufficient. If not, amend scope or retain block. |
| CanonFS semantics | Exactly 100 repeated-content writes including first insertion; exactly 100 preloaded verified reads, per size. No generic pilot doubling. | Independent content-hash check, all intrinsic costs, output lifetimes, 128 MiB read-only retained-capacity budget and 1 GiB worker-RSS acceptance; separate human-selected cgroup containment ceiling. No unique-object or disk-throughput claim. |
| Resource meaning | Per-child whole-process peak/CPU observations, distinct from parent and interval work. | OS API scope and boundary agreement; zero coarse CPU deltas remain zero. No RSS subtraction or cgroup-to-process equivalence. |
| Artifact meaning | Actual linked build inventory, raw native sections and file identities. | Stable rebuilds/tool outputs; format mapping or shared-dependency attribution requires additional justification. |
| Host | Dedicated Linux/x86-64, verified core reservation, reproducible policy and bounded timer/noise. | Candidate inventory, driver/control readbacks, thermal sensors, effective affinity, accounting, build and timer/pilot evidence; none collected. |
| Statistics | Thirty complete trials per subcase; explicit warm-up/counts; median, inclusive-linear IQR and sample CV; >5% CV or >5% unexplained half-session drift inconclusive. | Raw prospective samples and stationarity checks after authorization; no outlier removal or favorable rerun selection. |
| Authorization | Human-approved trust root and signed separate qualification/campaign permits; host and cell eligibility checked independently. | Permit format, verifier/revocation/expiry/replay review and negative tests. A flag, key supplied by a caller or status field grants nothing. |

VM blockage is a denominator-validity question, not a reproduced defect report
or authorization to repair the runtime. No new counter implementation, alternate
VM, scheduler, benchmark runner or fixed-source modification is proposed here.
CanonFS independent oracle work is bounded to the selected hashing contract; no
new storage subsystem is needed. The statistical policy is descriptive, not an
inferential proof of superiority. Resource ceilings are proposed human decisions,
not permission to allocate resources during this task.

## Minimum future sequence

1. Human reviewers accept/amend these three documents and the VM count meaning.
   Integration status is handled separately; do not treat PR existence as acceptance.
2. Review the smallest isolated future instrumentation/permit-verifier patch.
   B0 remains non-measuring. Tests must cover denial before timer/warm-up/worker
   launch, forged/mismatched/replayed permits and unsupported host controls.
3. Identify a candidate Linux host and gather authorized static evidence. Review
   proposed settings, budget and the operator-controlled trust root; no timer
   probe or pilot is authorized by static readiness alone.
4. If explicitly permitted, conduct bounded qualification-only probes/pilots and
   preserve every failed/inconclusive result. Stop on missing semantics or controls.
5. Only a later separate campaign permit binding reviewed qualification evidence
   and exact cells/counts can allow trials. No ordinary CLI switch changes this.

This task stops at the design draft. No item in that sequence was activated.
No campaign is authorized until all necessary human decisions and evidence exist.

## Exact validation of this design packet

Validation for this documentation-only increment consists of source-call-path
inspection at the pinned revisions, primary OS documentation review, static
Markdown/link/pin/scope checks, `git diff --check`, and mathematical checks on
explicitly synthetic dimensionless fixtures. No binaries were built or run; no
B0 acceptance tests were repeated and no host qualification was attempted.

The synthetic definitions in the contract were checked with Python's
`statistics.median`, `quantiles(..., method="inclusive")`, `stdev` and arithmetic
unit conversions. Inputs 1..30 give median 15.5, IQR 14.5 and CV
56.7961834247%; equal positive fixtures give IQR/CV zero. The synthetic VM unit
conversion uses 2,000,000 ns / 10,000 ticks = 200 microseconds/1,000 ticks.
These are mathematical fixtures, not samples. Static checks require exactly the
three requested new Markdown files, all five IDs with one matrix classification,
preserved revision/hash/input identities, resolved sibling links and Git source
paths, and absence of executable/workflow/Phase A/B0 changes. The source review
is not an independent execution or a new measurement result.

The design commit and new draft use `[skip ci]`; no benchmark-containing workflow
is dispatched or rerun. CI status must be reported as skipped/not run, never a
pass. This packet adds no executable or workflow and cannot activate the existing
B0 measurement gate. Operational cancellation of earlier PR checks is historical
context, not a new CI validation claim. No frozen protocol or governance state
changes and no merge occur.
