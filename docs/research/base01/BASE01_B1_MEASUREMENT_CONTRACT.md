# BASE-01 B1 measurement contract — proposed design v0.1

**Status: DESIGN ONLY. Measurement authorization: absent.**
Authored by Codex (GPT-6), independently of Jules' integration work, November 2,
2026. Codex authored Lane A B0 and has that prior context; this design is not an
independent scientific review of its own implementation. No qualification probe,
warm-up, pilot, timing trial, resource campaign or artifact-size measurement was
performed for this design. No implementation or execution permission is granted.

Read with [host qualification](BASE01_B1_HOST_QUALIFICATION.md) and the
[readiness matrix](BASE01_B1_READINESS_MATRIX.md). Only these three proposed
files are added. `docs/research/base01/` follows the location proposed by Phase A;
creating it does not assert that Phase A or B0 has merged. Jules owns integration
readiness; neither PR #687 nor #688 is modified or re-audited here.

## Fixed scope and evidence identities

| Reference | Immutable identity |
| --- | --- |
| Measured project source | `cda9135d36144d4de2771af174e52ac7b30a83c9` |
| Phase A snapshot, PR #687 | `ecd66137bacbce50e605cfc1466ec7d36131ba9e` |
| Phase A manifest SHA-256 | `1367b021f393594e4aab3bc4b52a7b76ca94ee70a49b8f0bda62aea96d46e4ea` |
| Accepted B0 reference, PR #688 | `91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e` |
| Historical validated B0 implementation | `54c1a92ffe5c19871c3b1a68c71083ac87fc7f0c` |

Pinned references: [Phase A manifest](https://github.com/t81dev/t81-foundation/blob/ecd66137bacbce50e605cfc1466ec7d36131ba9e/docs/research/base01/BASE01_MANIFEST.json),
[B0 contract and proposed batching](https://github.com/t81dev/t81-foundation/blob/91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e/experiments/base01_lane_a/README.md),
[B0 input-identity receipt](https://github.com/t81dev/t81-foundation/blob/91e3a7d7f6082b0397f8d6ef3d7ea8a95f3a929e/experiments/base01_lane_a/evidence/final/receipt.json).
The separate historical PR #686 pin remains
`ac7397aa5f8751d34adcb6ce2d38e3141e65b36c`; it is not a measurement source.
Future instrumentation must remain outside the audited source tree and have its
own reviewed commit, executable/archive hashes and compiler-command record.
Changing measured source, inputs or this contract requires an explicit amendment.

Only the five IDs below are in scope. CMP3, H04-R03 Phase C, PMU, SPICE,
persistent/block-backed storage and other Phase A candidates remain excluded.
This is characterization of hosted software on a conventional binary CPU, not
an optimized binary-versus-ternary comparison, hardware result, energy estimate
or proof of canonical H01–H08. It supports the research evidence apparatus and
does not replace the historical Hanoi/CWEB lineage or activate Protocol 001.

## Common measurement boundary

One trial is one fresh **exec-created** worker for one subcase, not a fork-only
continuation of a warmed parent. One worker runs at a time. Startup, input
construction, allocation of harness containers, loading, correctness checking,
serialization and destruction of retained outputs are outside intervals.
Allocations/copies performed inside the public API remain inside its interval.
Report exactly which allocator state and result lifetimes are defined below.

Every worker checks its immutable inputs and relevant oracle before warm-up,
then prepares new measured state. It verifies all measured outputs after timing;
any mismatch invalidates the trial regardless of speed. Results escape to
caller-owned storage across separately compiled, non-LTO batch functions. No
per-item volatile checksum/barrier is permitted. Preserve and inspect optimized
assembly during future qualification to establish retention; self-authored
passing tests alone do not establish it. Pure input/output digesting is outside
intervals. Policy/trace work intrinsic to the API remains inside.

Use the same monotonic timer backend throughout a session, identified and
qualified as specified in the host document. UTC labels identify events only;
they never determine elapsed duration. Record integer raw nanoseconds, every
interval, operation counts and boundary counts. Do not subtract estimated timer
or loop overhead, allocation cost, a control run, or setup time from raw samples.
The empty-control overhead ratio is a qualification bound, not a correction.

## WL-ARITH-CODEC-I64

**Operation/oracle.** Separate `encode_i64(int64_t)` and `decode_i64(vector<Trit>)`
from the [audited arithmetic header](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/include/t81/ternary/arith.hpp).
Check each output's canonical balanced digits and exact value with the B0 Python
arbitrary-precision positional oracle; check decode against the original signed
integer. The existing arithmetic regression test covers saturation/extrema as
supporting evidence; overflow/saturation inputs are not added to this corpus.

**Inputs.** Preserve B0's ordered 1,031 integers: signed extrema and adjacent
values, zero, +1, -1, followed by 1,024 LCG values. Seed `0x424153453031`,
multiplier `6364136223846793005`, increment `1442695040888963407`, modulo 2^64;
low 63 bits are magnitude, high bit selects sign. Canonical compact sorted-key
ASCII JSON integer-list SHA-256:
`0fefb0e6b968e8c11556699700530d133505a0bac2013674a42203b10fcb54ef`.
No claim of uniform coverage of the int64 domain follows. A base batch cycles
this list for 100,000 conversions: 96 complete cycles plus the first 1,024 items.

**Boundary.** Encode starts with empty preallocated outer output slots; each API
call allocates its returned vector and moves it into a slot. Hold every output
until the batch ends; destruction is excluded. This is allocating API latency
with retained results, not allocation-free arithmetic or encode-and-free cost.
Decode inputs are completely pre-encoded and expanded in setup; preallocated
integer output stores are inside the loop. No encode is inside decode timing.
Warm-up uses separate buffers; allocator caches may remain warm and are part of
the stated process profile. Repeat fresh output batches only as fixed in the
qualification record; never overwrite live outputs and charge hidden frees.

**Metrics.** For elapsed `T` ns and `N` conversions, latency `T/N` ns/conversion;
throughput `N*10^9/T` conversions/s. Primary: latency for each API separately.

**Confounders/qualification.** Digit lengths, retained-output footprint,
allocator/libc version, page faults, cache state and compiler specialization must
be recorded. Qualify batch retention, memory bounds and timer overhead at the
registered count. Reject input drift, allocation failure, oracle failure or
incomplete count; flag failed overhead/stability/host gates inconclusive.
Do not extrapolate the corpus to all integers or claim radix superiority.

## WL-VM-TISC-DETERMINISM-TIERS

**Operation/inputs.** Run the six exact builders `workload`, `micro`, `meso`,
`mixed`, `policy-heavy`, `tensor-access` in the
[audited test](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/tests/cpp/vm_workload_determinism_tiers_test.cpp).
Preserve instruction tuples, literal kinds and policies; B0 canonical input
SHA-256 is `86787727b57b249fb6be8285043ffd957d3efba8a2728db12f70397d4142d529`.
Do not repair the historical R0-writing workload or introduce padded programs.

**Oracle.** Retain the audited state-signature comparison plus explicit semantic
checks, tags and float-handle dereferencing. Require `state.halted == true`,
expected final PC/state and no trap. At the pinned implementation,
`run_to_halt(5000)` returns success after a bounded loop even when the step budget
expires; a success result alone does not establish completion. Qualification
must check this condition without changing the measured project source.

**Boundary.** Construct/load fresh machines outside timing. Measure complete
`run_to_halt(5000)` calls, including policy, trace allocation, GC and in-run hot
trace compilation. Label it the factory's hosted VM path, not a pure interpreter
or steady-state JIT kernel. Warm-up machines are discarded; no measured machine
is reused, so in-run compilation stays part of the result.

**Denominator.** Capture context-0 R75 before/after each complete run and retain
both values. A fresh machine must start with zero; require final positive finite
integer ticks, no reset/overflow and final synchronization. For a trial let
`D = sum(R75_after - R75_before)`, `T = sum(interval_ns)`, `M = completed runs`.
Report `T/D` microseconds per 1,000 **R75 software ticks**, plus `T/(1000*M)`
microseconds per completed program. Do not use trace length, 5,000, outer
`step()` calls, hardware instructions, or expected historical ticks in place of
observed `D`.

**Counter qualification is currently unresolved.**
[VM counting](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/vm.cpp),
[trace execution](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/jit/jit_compiler.cpp)
and [R75 synchronization](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/vm/runtime_state_helpers.cpp)
show interpreter increments before dispatch and trace increments before opcode
handling, including side exits; some trace returns bypass interpreter register
synchronization. Ticks are not established as retired semantic instructions.
Before eligibility, retain a source-derived, independently checked count ledger
for each builder: executed/attempted instructions, guard retries, side exits,
Halt inclusion and the final R75 flush. Repeated equal signatures/ticks are not
that proof. Never rename ticks as instructions. If the ledger cannot establish
this software denominator consistently, normalized rates are invalid and this
subcase remains blocked. A program-latency-only alternative needs an explicit
protocol amendment; it is not a silent replacement of the requested metric.

**Batching/limits.** From correctness-only expected ticks choose a fixed whole
run count giving at least 10,000 ticks. Prepare at most 16 machines per chunk;
time the full chunk loop once, storing call outcomes for later verification.
Sum chunk intervals/counts, not averages of per-run rates. Allocation between
chunks is excluded; allocator/cache carryover is documented. This intentionally
proposes chunk intervals instead of B0's per-machine intervals to bound timer
cost for micro. All 30 trials use the same count/chunk partition. Future
qualification must show the pool fits the budget, counter ledger holds and
overhead is small; this design has not demonstrated any of them. Counter,
halt, signature or input discrepancies invalidate results; time noise is
inconclusive. Batching must not change the programs or remove JIT costs.

## WL-CANONFS-MEM-THROUGHPUT

**Operation/inputs.** Separate in-memory `write_object(RawBlock, payload)` and
`read_object_bytes(ref)` for 4,096, 65,536 and 1,048,576 bytes, all byte `0x42`.
Payload SHA-256 values respectively:

- `725bcd6c66d02acf6ebeab9c92410e010ea22e336876256aaf05a211f4ce1902`
- `fee47b1f0d7685a226fd5f2b9dd8f525038bbb05fe9d89a5d75c249edac868e3`
- `5ae9782017a68037004b2bf806c77d324db4d915ed3725d84eb3121b2ad16061`

**Oracle/boundary.** Use the
[audited memory driver](https://github.com/t81dev/t81-foundation/blob/cda9135d36144d4de2771af174e52ac7b30a83c9/fs/in_memory_driver.cpp):
assert success, repeated-write identity, independently checked expected content
hash and byte-exact read equality. Qualify CanonHash expectations with an
independent specification-compatible hash check, not SHA-256 substituted for
CanonHash. SHA-256 here identifies inputs only. Bootstrap mode has no installed
Axion hook/capability table; read verification must be explicitly enabled and
recorded. Do not claim policy-check overhead or provenance authenticity.

Each trial starts a fresh memory driver. Write is 100 repeated writes of the
same payload/key on that driver, including the first insertion; hashing,
copying, parity generation and intrinsic allocations/frees remain inside.
Store all returned references and verify afterwards. This is repeated-content
storage, not unique-object ingest. Read preloads the one object in setup;
perform 100 reads with hash verification and returned-vector allocation/copy
included. Retain all 100 outputs until after timing; no hidden per-read free.
Time the whole batch, including loop/output-store costs. State lifetime and the
first-write fraction must remain fixed if qualification changes batch count.

**Metrics.** For block size `S`, completed operations `N`, `T` ns: primary decimal
throughput `S*N*1000/T` MB/s; latency `T/(1000*N)` microseconds/block. Numerator
counts payload bytes, not parity/hash metadata or memory bus traffic.

**Confounders/qualification.** Hash verification, allocator reuse, parity work,
cache/paging effects and first-insertion fraction are part of the profile.
Retained reads increase setup/peak-memory context; include this in metadata.
Require a bounded retention plan, independent hash expectation, successful APIs
and overhead/host qualification. Disabled verification, wrong hash/bytes or
unexpected filesystem I/O invalidates the cell. No persistent filesystem,
fsync, disk cache, block driver or PR #686 policy workload enters this campaign.

## WL-RES-RUSAGE-METRICS — observer

**Operation/oracle.** Collect worker self-accounting and per-PID waited-child
accounting, outside API intervals. Validate nonnegative counters and monotonic
user/system CPU time and high-water RSS for the same process. Agreement between
self/waited-child observations within documented collection boundaries is a
consistency check, not an independent physical memory oracle.
`test_resource_monitoring` verifies VM policies; B0's orchestrator smoke check
neither measures worker resources nor qualifies this future observer.

**Identity/boundary/units.** Attach each observation to trial ID, PID, exact
binary/input hashes, accounting selector and lifecycle phase. Worker self values
cover its setup, verification, warm-up and measured work; a wait/reap observer
captures the completed child's lifetime. No worker descendants are allowed in
the initial profile. CPU time is user/system seconds, raw timeval components
also retained. Peak RSS is Linux raw KiB; macOS raw bytes divided by 1,024,
preserving fractions. Record OS/kernel and raw unit before normalization.
Linux [getrusage](https://man7.org/linux/man-pages/man2/getrusage.2.html),
[per-child wait4](https://man7.org/linux/man-pages/man2/wait4.2.html) and
[Apple's source manual](https://github.com/apple/darwin-xnu/blob/main/bsd/man/man2/getrusage.2)
provide the platform accounting references; qualification must pin the actual
host documentation. Older archived Apple manuals differ on units; do not infer
units from the word POSIX or silently normalize an unknown platform.

Use a specific waited PID; never subtract cumulative parent `RUSAGE_CHILDREN`
peaks to estimate this child's peak. Neither peak-RSS subtraction nor a parent
`RUSAGE_SELF` observation estimates incremental worker allocation. CPU deltas
between two self observations may be reported only with their exact phase
scope; they do not become isolated kernel CPU cost. No denominator or throughput
for this observer; retain distributions of process peaks/CPU seconds separately
for each subcase's 30 children, not pooled across workloads. Cgroup memory is a
different scope, not a replacement for process RSS or an enforcement proof.

Qualify per-child API availability, reaping order, no aggregate/parent confusion,
normalization fixtures and retained failures. Missing identity/units, failed
accounting, descendant ambiguity or lifetime truncation invalidates resource
results. Coarse CPU accounting may give zero for short work; retain zero and
mark CPU-resolution limits rather than fabricating precision or adding load.
Timing results may be retained diagnostically but the five-workload campaign
cannot be called complete if its required observer is invalid.

## WL-RES-ARTIFACT-SIZE — observer

**Operation/oracle/boundary.** Outside any timing interval, identify the exact
measurement executable and every linked project archive, plus supporting
correctness executables, by stat bytes and SHA-256. Preserve compiler/linker
versions, command lines, build IDs, source/runner commits, paths, dependencies,
standard library, LTO, stripping/debug and signing status. Check the file list
against the actual link commands. Do not rebuild the unneeded CLI or Google
Benchmark runner simply because Phase A names them; do not claim their sizes.
Input identity is each artifact hash plus this build profile; no random input.

Retain tool version, raw ELF or Mach-O section/segment headers, file offsets,
flags, types, memory sizes and file sizes. Distinguish whole-file disk bytes,
loadable file bytes and in-memory sections such as zero-fill/BSS; do not sum
memory sizes and call the sum disk size. Report each format's native names.
No ELF/Mach-O text/constant-section equivalence, shared-library cost attribution,
compression advantage or CAS serialized size is assumed. No cross-format ratio
is part of this campaign.

**Metrics/qualification.** Exact integer bytes per named file/section, not a
rate; one inventory per immutable build, referenced by every child. Qualify tools
and repeat clean builds with the identical registered paths/environment. If
hashes differ, retain both inventories, locate differences and withhold a claim
of bit-reproducible builds. Unknown format, failed tools or mismatched binary
identity invalidates the relevant footprint record; artifacts may still be
reported individually with limits. No 30-trial size variance is invented.

## Trial and statistical contract

There are 14 timed subcases: two codec, six VM, six CanonFS. The initial complete
campaign requires all to become eligible. Thirty trials and a uniform 5% CV
gate follow the accepted B0 proposal; Phase A's smaller per-workload trial counts
and its stricter draft VM CV threshold are not silently treated as identical.
This uniform threshold is a proposed B1 decision requiring human acceptance;
changing it later creates a new protocol/profile, not a favorable reanalysis.
Do not silently drop a blocked tier;
a smaller campaign requires a new explicit scope decision. Resources accompany
children; artifact inventory accompanies builds. Neither creates a timed kernel.

1. Plan 30 fresh-process trials per timed subcase: 420 measured children if all
   qualify. Fresh processes reduce shared-state dependence but do not prove
   statistical independence from thermal or temporal drift. Record campaign
   order and observations, never describe them as independent proof by process ID.
2. Warm up codec with 10,000 conversions; each VM tier and CanonFS operation with
   five operations on disposable state. Verify warm-up results, discard its
   objects and construct new measured state. This defines a warmed process with
   fresh workload state, not cold caches. Warm-up itself needs authorization.
3. Use the base batch counts above. Only qualification-authorized pilots may
   adjust them, with at most three pilots per cell, deterministic doubling and
   no ranking of speed to select a count. Freeze final counts/chunk partitions
   before the campaign; label and retain every pilot separately. Do not tune a
   batch mid-campaign or count pilots toward the 30 trials. If a count cannot
   satisfy timer/memory budgets, stop; amend the design rather than overrun them.
4. Use 30 blocks, each containing every subcase once. Base order is codec
   encode/decode, VM builder order above, then CanonFS size-ascending write/read.
   Rotate that list left by `block_index mod 14`. Record the full schedule before
   launching; one child at a time. Counterbalancing is imperfect with 30 blocks;
   inspect trial-index trends, do not assume drift disappears.
5. For each trial first sum elapsed intervals and operation counts, then compute
   that trial's normalized metric. Retain both raw and normalized values. Median
   is the middle sorted value or mean of the two middle values. Quartiles use
   linear interpolation at zero-based position `(n-1)*p`, `p=0.25,0.75`;
   IQR is Q3-Q1. CV is sample standard deviation (`n-1`) divided by arithmetic
   mean, multiplied by 100. Compute over 30 trial-level values, not individual
   inner operations. Latency and reciprocal throughput get separate summaries.
6. Gate the registered primary metric at CV <=5%; also expose reciprocal metric
   CV and all samples. Missing/nonpositive timed durations or counts, or a
   correctness/identity/authorization failure, invalidate the affected series.
   No CV is defined for a nonpositive mean. CV >5%, timer overhead failure or
   unexplained first-half/second-half median drift >5% is inconclusive, even
   if an alternative summary looks favorable. Passing CV alone is not accuracy.
7. Preserve outliers, failures, interrupted/incomplete sessions and host violations.
   No trimming, winsorizing, replacement trials or retry-until-green. Stop on a
   correctness or hard host-boundary failure; partial sets stay incomplete and
   do not supply an accepted 30-sample summary. Ordinary slow valid trials stay
   included. A fresh campaign after a diagnosed correction needs a new permit,
   a new ID and links to all prior outcomes; do not overwrite the first attempt.
8. Report median/IQR as descriptive spread, not confidence bounds on a population
   or significance tests. Do not pool different hosts, binaries, batch sizes,
   compilers, input distributions, allocator policies, thermal/frequency profiles,
   timer backends or output lifetimes. Record matched identities before any
   cross-run comparison; conversions/setup and assurance must match. No comparative
   radix claim follows from this baseline alone.

**Synthetic mathematical check only:** the dimensionless integers 1 through 30
have median 15.5, Q1 8.25, Q3 22.75, IQR 14.5 and sample CV approximately
56.7961834247%. Thirty equal positive fixture values have IQR/CV zero. These
are not durations, host results, pilots or benchmark samples. A separate,
explicitly synthetic unit check uses T = 2,000,000 ns and D = 10,000 ticks:
T/D = 200 microseconds per 1,000 ticks, not 0.2; no time was observed.

## Future authorization and evidence requirements

Authorization, host qualification and workload eligibility are separate gates.
A future reviewed runner must remain deny-by-default in ordinary builds and CI.
A CLI flag, environment variable, JSON `authorized` field, commit authorship or
this document cannot supply authority. No dispatcher is implemented here.

Proposed activation requires a detached Ed25519-signed permit verified against a
maintainer-approved public-key trust root held outside mutable runner inputs.
The trust-root enrollment/rotation decision is human-controlled; accepting an
arbitrary key next to a permit defeats the design. Parser, canonical signed-byte
format, signature/library review and negative tests are future engineering, not
completed security assurance. This guards activation under trusted reviewed
code and a controlled operator host; it is not protection against root replacing
the executable or trust store. Protected CMP3 signing rules are not reused or changed.

| Gate | Required future evidence |
| --- | --- |
| Authorization | Verified maintainer identity/signature, permit ID, allowed action (`qualification-only` or `campaign`), validity/revocation, source and runner/binary hashes, hashes of these three approved documents and inputs, host/boot identity, exact cells/schedule/counts, resource limits and output destination. |
| Host qualification | Static control checks plus separately authorized timer/noise/pilot evidence on this host/boot/configuration, expiring on configuration drift. A permit does not manufacture a passing host report. |
| Workload eligibility | Passing correctness/retention/denominator/accounting/footprint qualification for every named cell; matrix status alone is not evidence. |
| Execution | Reviewed gated implementation, all prior gates satisfied, unused campaign nonce and atomic run-directory reservation, remaining budgets, no protected cells or unsupported controls. |

Avoid circular qualification: a **qualification-only** permit can authorize the
bounded timer probes/pilots after static host checks, but cannot authorize a
campaign or self-approve qualification. A separate campaign permit binds the
reviewed qualification evidence digest and frozen batching plan. Unknown,
expired, revoked, replayed or mismatched permits fail closed before timer reads,
warm-up, worker launch or output overwrite. An unavailable or stale revocation status fails
closed; the approved permit format must bound that freshness. Reserve the campaign
nonce once, then consume uniquely bound trial IDs so launcher and worker checks
do not accidentally authorize duplicate trials. Authorization is checked in the
worker as well as the launcher and before each trial; revoke/expiry prevents
further trials and aborts at a reviewed safe boundary, preserving partial data.
No claim of instantaneous revocation of an already running API call is made.

Proposed ceilings for later human approval: one workload thread/child at a time,
1 GiB worker peak-RSS observation budget, 16 VM instances per chunk, 16 MiB
retained output per child, 8 GiB total evidence, 60 seconds per trial/pilot,
30 minutes total qualification, eight hours total campaign. These are ceilings,
not predicted costs or exact RSS enforcement guarantees. External watchdog and
OS/cgroup limits must have explicitly recorded, distinct scopes. Exceeding any
ceiling stops the run and retains an incomplete record; absent reliable budget
checks blocks eligibility. This design grants no resource allocation.

Retain append-only per-child records: permit and host-report digests; source,
runner and executable/archive hashes; compiler/CMake/dependency/environment data;
input and output-verification identities; UTC labels; PID/exit/signal; warm-up,
setup and batch definitions; every integer interval/count; R75/halting data;
raw/self/waited-child accounting; host observations; tool output/errors and all
failure reasons. Seal completed evidence with an indexed SHA-256 manifest and
human review. Hashes establish content identity, not truth or authentic provenance.
