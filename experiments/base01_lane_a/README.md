# BASE-01 Independent Baseline Lane A — Phase B0

Codex authored this runner independently for the maintainer's Lane A assignment.
No Lane B runner was read, copied, discussed, or coordinated. This is agent-authored
validation apparatus, not independent scientific certification of its own code.
It supports the stable deterministic VM and CanonFS surfaces; it adds no runtime
subsystem or research finding. Canonical H01–H08 and Hanoi/CWEB history are unchanged.

**Measurement is disabled.** Both executables reject `--measure` (exit 2).
There is no authorization flag, environment variable, CI job, PMU collector,
benchmark-runner invocation, arbitrary workload selector, or path to CMP3 or
H04-R03 Phase C. Subsequent explicit BASE-01 B1 authorization and a reviewed
activation patch are required. A JSON field is not authorization.

## Source and workload contract

The workload source is exactly `cda9135d36144d4de2771af174e52ac7b30a83c9`.
The runner is in a different worktree and build directory; it links archives
built by that source's existing CMake targets. It never patches the audited tree.
`phase-a-manifest.json` is a byte copy from PR #687's commit
`ecd66137bacbce50e605cfc1466ec7d36131ba9e`; it is a historical input, not a new
acceptance or authorization. Its full inventory is retained for identity, but
only the following exact five IDs are used:

| ID | Validation and planned timing boundary |
| --- | --- |
| WL-ARITH-CODEC-I64 | 1,031 exact signed integers: extrema/neighbors, zero, ±1, and 1,024 unsigned-LCG-derived values. Native roundtrip/canonicality plus an independent Python arbitrary-precision positional oracle. Separate encode/decode batches. |
| WL-VM-TISC-DETERMINISM-TIERS | All six exact builders from the audited test: workload, micro, meso, mixed, policy-heavy, tensor-access. Reuse audited signature oracle; independently apply its state contract to the runner-owned VM, plus explicit semantic output checks. |
| WL-CANONFS-MEM-THROUGHPUT | Exact 0x42-filled 4,096, 65,536 and 1,048,576 byte buffers; content identity, repeated-write identity and byte-exact verified read. Separate write/read cases. |
| WL-RES-RUSAGE-METRICS | Observer, not a timed kernel. B0 checks availability and non-decreasing RUSAGE_SELF CPU/RSS counters without retaining timing samples. |
| WL-RES-ARTIFACT-SIZE | Observer: exact stat bytes, SHA-256, and native section report for the actual runner; archives and correctness executables are individually identified. No claim these sizes describe an unbuilt CLI or Google Benchmark executable. |

The LCG uses seed `0x424153453031`, multiplier `6364136223846793005`, increment
`1442695040888963407`, modulo 2^64. Low 63 bits supply magnitude, high bit sign.
Receipt input hashes use sorted-key compact ASCII JSON for integers and VM
instruction tuples/policies; CanonFS uses SHA-256 of payload bytes.
No distribution provided by the other lane is assumed. Cross-lane numerical
agreement requires checking exact input identities and measurement boundaries.

## Reproduce validation only

Prerequisites: Git, CMake >=3.20, Ninja, a compiler supporting the audited
project's C++23 mode, Python >=3.9. The runner itself has no Python dependencies.
The project's tests fetch Asio tag `asio-1-30-2`; build logs record its resolved
commit. Optional llama.cpp, benchmarks, TUI, DPE and TernaryOS are disabled.
Do not run `ctest` without a filter or invoke `benchmark_runner`.

From the Lane A branch checkout, use separate absolute paths (examples below):

```sh
git worktree add --detach /tmp/base01-source cda9135d36144d4de2771af174e52ac7b30a83c9
cmake -S /tmp/base01-source -B /tmp/base01-source-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_TUI=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_FUZZ_TESTS=OFF \
  -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_DPE=OFF
cmake --build /tmp/base01-source-build --target \
  t81_ternary_arith_test t81_vm_workload_determinism_tiers_test \
  t81_canonfs_driver_test test_resource_monitoring -j 4
cmake -S experiments/base01_lane_a -B /tmp/base01-native-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_CXX_COMPILER=/usr/bin/c++ \
  -DBASE01_SOURCE=/tmp/base01-source -DBASE01_BUILD=/tmp/base01-source-build
cmake --build /tmp/base01-native-build
python3 -B -m unittest discover -s experiments/base01_lane_a -p 'test_*.py' -v
python3 -B experiments/base01_lane_a/lane_a.py --validate-only \
  --source /tmp/base01-source --source-build /tmp/base01-source-build \
  --native-build /tmp/base01-native-build --output /tmp/base01-validation-new
```

Use the **same compiler and standard library** for both builds. `/usr/bin/c++`
is the tested macOS compiler; select the corresponding compiler on Linux. Keep
LTO disabled. The runner rejects explicit global `-flto` flags. Never reuse
archives from another source revision/configuration. Source identity validation
compares every tracked blob's bytes against Git before and after validation,
not just HEAD or Git's cached stat data. The disabled llama.cpp gitlink is
recorded by identity; its contents are not inspected or built.

Use a new output path each time. Outputs are never overwritten. A failure retains
partial records and exits nonzero without a success receipt. The receipt records
the runner checkout's HEAD plus per-file hashes; commit code before obtaining a
publication receipt. Evidence-only commits can follow that implementation commit.
Each artifact is hashed, including raw native outputs, correctness stdout/stderr,
compiler commands and CMake caches. Absolute paths are retained, not anonymized.
SHA-256 establishes content identity, not truth, authorship or authorization.

## B1 measurement specification — prepared, unexecuted

This specification supersedes the draft's smaller repetition counts with the
maintainer's **30 independent trials per timed subcase**: codec encode/decode,
six VM tiers, and CanonFS write/read at each of three sizes. Observers accompany
trials, not separate timing campaigns. No trials or warm-ups have been run.

1. Each trial must be a fresh child process. Run the correctness checks before
   any timing. Stop on error; retain the failed trial, never replace or exclude
   it silently. Use deterministic subcase order; record it and any later changes.
2. Allocate outer buffers, construct/load VM and preload read objects before
   timing. Warm up codec with 10,000 conversions and other subcases with five
   operations on **separate disposable state**. Build fresh measured VM/store state
   after warm-up. Warm-up count and fresh-state semantics must be retained.
3. Codec measurement: 100,000 inputs, cycling the exact 1,031-input corpus.
   Separate encode and decode, including encode's intrinsic vector allocation.
   `kernels.cpp` defines external batch functions; LTO is off, and caller-owned
   output arrays escape the batch and are completely checked after the interval.
   No per-conversion volatile checksum or barrier is needed. The production
   `encode_i64` returns an allocating vector: Phase A's "zero dynamic allocations"
   description does not match its call path. Do not replace it with a new codec.
4. VM measurement: time `run_to_halt(5000)` on preloaded fresh machines. Five
   thousand is a step limit, not an executed-instruction count. Batch complete
   fresh runs until at least 10,000 **R75 instruction ticks**, with bounded
   preallocated pools and individually delimited intervals; sum raw intervals,
   counts and timer boundaries. Record finite timer-resolution/interval overhead,
   especially for micro. Do not pad programs or count trace entries as instructions.
   The factory includes hot-trace compilation; include its execution-time costs
   and label it accurately. New settings to disable that path need review.
5. CanonFS: 100 operations per trial at each size, separately write/read. The
   write case characterizes repeated same-content writes (hashing, copy and parity
   regeneration), not unique-object growth. Create driver/payload before timing;
   preload the exact object for reads. Reads include hash verification and the
   API's returned-vector allocation/copy; writes include intrinsic map/parity
   allocations. Destroy results after each interval and verify outside it.
   Bootstrap mode matches audited BM_CanonFS: no installed Axion hook or capability
   table; do not label it a governed-policy overhead measurement.
6. `measurement.hpp` prepares a steady-clock interval primitive with compiler
   fences at the interval boundary. It is **never instantiated/called in B0**.
   Timer resolution and the complete dispatcher/child-process campaign must be
   qualified in a reviewed B1 activation patch; no usable campaign exists in B0.
7. Retain all 30 raw durations in integer nanoseconds, counts, per-child user and
   system CPU seconds, process-lifetime peak RSS, start/end UTC labels, pre/post
   observations, input IDs, binary/archive hashes and full host/toolchain metadata.
   CPU time and RSS describe the whole child, including verification/warm-up;
   they are not isolated kernel costs. RSS is **not** a pre/post subtraction:
   Linux ru_maxrss is KiB; macOS is bytes divided by 1024, preserving fractions.
   It is a high-water mark, not an enforcement guarantee.
8. Normalize codec to ns/conversion and conversions/sec, VM to microseconds per
   1,000 R75 ticks, CanonFS to decimal MB/s and microseconds/block. Retain aggregate
   intervals and denominators. Report median, inclusive-linear quartile IQR
   (`statistics.quantiles(..., method="inclusive")`), and sample standard
   deviation / mean CV. Flag CV >5% unstable/inconclusive; do not rerun until green.
   `summarize()` tests use labeled mathematical fixtures, not invented results.

## Environment readiness and limits

This packet validates macOS ARM64 only. Linux code paths were reviewed and RSS
normalization is unit-tested; Linux binaries have not been executed here.
Windows is explicitly unsupported by the POSIX observer and runner. There is no
ARM-specific intrinsics requirement; native ARM64 compilation was performed.
No Linux/Windows/x86_64 CI pass is claimed.

Host inspection reports affinity as unsupported on macOS and observed-only on
Linux; the runner never applies pinning. Frequency/boost controls are uncontrolled
and never silently asserted. Kernel/cache/memory/CPU data and load are retained
where available; denied/missing sysctl keys retain their errors. A shared Mac
with no verified core isolation or frequency control is **not B1-qualified**.
Do not treat low load alone as proof of isolation. B1 needs explicit acceptance
of measured host limitations or a suitably controlled Linux host, reviewed
campaign activation, timer/denominator qualification, and subsequent maintainer
authorization. Nothing here changes CMP3, H04-R03, Protocol 001, RFC acceptance,
frozen artifacts, official publication or merge authority.

See `VALIDATION.md` and `evidence/` for commands, failures and actual raw evidence.

## B0 final acceptance addendum

See [B0_ACCEPTANCE.md](B0_ACCEPTANCE.md) for the bounded final integration review.
PR #687 need not merge first: Lane A bundles its exact pinned manifest snapshot
and does not load Phase A files from `main`. This is a technical dependency
statement, not acceptance of every historical Phase A assertion. Runtime
validation now checks the full snapshot SHA-256, including all classifications,
source pins, frozen exclusions and `measurement_authorized: false`.

The historical manifest associates `test_resource_monitoring` with POSIX
accounting. That attribution is incorrect: the executable checks VM resource
policies and remains supporting regression evidence. The actual B0 validation
for `WL-RES-RUSAGE-METRICS` is `lane_a.resource_validation()`, which checks two
`getrusage(RUSAGE_SELF)` observations for nonnegative/nondecreasing CPU and peak
RSS counters and applies Linux/macOS RSS normalization. It is an API smoke check,
not an independent accounting oracle, workload-child resource attribution, RSS
enforcement, or a timing campaign. The retained receipt already scopes this as
`orchestrator RUSAGE_SELF smoke check only`; historical bytes remain unchanged.

The original seven contract-test results remain historical evidence. Acceptance
adds four focused checks, including native rejection tests enabled with:

```sh
BASE01_ACCEPTANCE_NATIVE=/tmp/base01-native-build/base01_lane_a \
  python3 -B -m unittest discover -s experiments/base01_lane_a -p 'test_*.py' -v
```

The historical identity test needs the pinned Git objects. They are already
present in this review environment; a fresh clone may need an explicit read-only
setup fetch before running that test:

```sh
git fetch origin ecd66137bacbce50e605cfc1466ec7d36131ba9e
```

This retrieves historical evidence; it does not merge PR #687, change the source
pin, or authorize execution. The validator itself never fetches it.
Without the explicit native executable path the native check reports a skip. Do not
rerun the general integration workflow: it contains unrelated benchmark jobs.
This correction uses a `[skip ci]` commit to avoid starting those workflows;
local B0 checks supply the acceptance evidence, not an overall CI pass.
