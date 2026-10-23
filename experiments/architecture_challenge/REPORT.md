# Architecture decision: inventory replenishment under revocation

2026-10-22. Experimental comparison based on
`0005ab675d2729b2795200e8c78ded43d4520b0d` (latest main after merged #553).
PR #536 was inspected as open at `3e52b6b5247b578e5f6a861a1b9a971b6e93c7cd`;
no merge, patch dependency, numerical expansion or frozen ISA modification.

Decision: **neither arm establishes authenticated occurrence or freshness under
a malicious producer. No architectural winner is justified.** Honest-host
revocation enforcement is comparable; no custom-VM advantage is demonstrated by
this bounded integer workload. The smallest enforcement mechanism in both arms
is the host-owned serialized authorization/write boundary. T81's native hook is
useful admission machinery, but the live spending limit and transaction binding
come from the experiment adapter, not a new Axion language feature.

## Reconciliation with existing research

- [Research charter](../../docs/research/RESEARCH_CHARTER.md): treat the architecture
  as falsifiable. Its identical internal-bundle criterion is narrowed here to
  protocol outcomes; native instruction traces/addresses retain their differences.
  Its 10x complexity/latency threshold is a hypothesis, not a prior measurement.
- [Experiment framework](../../docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md):
  reuse inventory replenishment, mutable authority, detached audit and adversaries.
  Its three-way design is sequenced as A/B first; no Python/OPA/database arm yet.
  Timing cannot choose revocation order. Duplicate, malformed, interrupted and
  malicious-producer cases are additional first-class observations.
- [Baseline-X proposal](../../docs/research/BASELINE_X_PROTOTYPE.md): use a standard
  Wasm runtime and a host capability boundary; do not build another VM, CanonFS or
  policy language. Its ~500 LOC, latency and determinism table contains targets
  and assertions, not measured controls. SQLite, Sigstore and JSON-LD are not
  necessary to test this local write boundary. Omitting a signature means origin
  remains deliberately unauthenticated, not implicitly supplied by content hashes.
- [Ternary closure](../../docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md):
  retain the CPU ternary optimization stop. Candidate 3 motivates provenance
  work but supplies no benchmark result for this transaction. Neither its math
  overhead target nor numerical conformance is tested here.
- [Deterministic provenance v2](../deterministic_provenance/README.md): reuse its
  production interpreter/CanonFS seams, separate canonical evidence from timing,
  and independently recompute identities. Its instruction-specific oracle,
  unsigned evidence and explicitly absent transaction/revocation claim do not
  substitute for this boundary comparison. Preserve v1 failures and all existing
  numerical exclusions; no float API is exercised here.
- [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md): its six
  primitives inform the protocol: request/effect, hash evidence, fixed method,
  live authority, responsible host and independent challenge. This experiment is
  **not full RFC conformance**: no signature, timestamp or asserted signer; a
  verified denial must be distinguishable from an allowed act (the draft's
  blanket ALLOW-only verifier does not model that). Rehashing all six fields
  cannot make an invented history factual. A future protocol needs a pinned
  trust policy and authenticated execution/commit witness, not a self-declared key.
- CanonFS [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md),
  [identity/persistence RFC-0054](../../spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md)
  and `include/t81/canonfs/canon_driver.hpp`: object identity is preserved;
  externally serialized callbacks are not a concurrent or transactional authority
  service. This experiment neither promotes interchange schema maturity nor
  claims import/export provenance proves factual correctness.

## Security boundary and comparison

The [machine-readable contract](workload-contract.json) fixes the request, TCB,
observable side effect, threat model and acceptance criteria for both arms.
The local PO payload is the protected effect. There is no supplier/payment
integration, model inference, numerical-performance or real-world inventory claim.

| Boundary | Arm A | Arm B |
| --- | --- | --- |
| Calculation | Existing T81 interpreter, four TISC instructions, production trace, static instruction budget | Wasmtime integer multiplication, 100 fuel units, single host import, no WASI |
| Final authorization | Persistent CanonFS invokes production Axion hook immediately before write; host binds actual amount, latest limit and one-shot session state to an allow/deny policy | Host `submit` binds actual amount, latest limit and one-shot session state immediately before file creation |
| Policy update | Explicit serialized transition after compute, after preliminary check, or after commit | Same external transition order inside host callback |
| Storage | Production SHA3-512/256 + Base81 CanonFS object paths | Standard SHA-256 named ordinary files |
| Evidence | Common canonical protocol envelope archived through native CanonFS, separate from PO effect | Same envelope archived through ordinary SHA-256 CAS |
| Native detail | Real VM PC/opcode trace retained in corpus | Native API/fuel mode retained; internal instruction tracing off |

Both hosts own authority and the write path. No external actor mutates authority
between final check and write in the serial executor. This defines a linearized
bounded execution schedule, not a thread-safety test. The existing concurrent
revocation experiment remains the separate real-thread test; there is no claim
that this adapter makes arbitrary Driver callback replacement thread-safe.
The `revoke-after-check` case permits the preliminary check but denies in the
actual storage hook. `revoke-after-commit` permits the already completed write.
The intentionally broken `snapshot-control` uses the earlier authority and is
caught by the external oracle in both arms. No sleeps/race probabilities select
any outcome.

The guest can invoke only its supported calculation and the single bound effect.
Wasm forged amounts, negative values and missing-capability imports are tested.
Arm A's program is constructed from validated bounded integers, not attacker
supplied TISC. Host memory corruption, privileged filesystem bypass, untrusted
compiler/OS and malicious replacement of the trusted observer are outside the
honest-host enforcement boundary, but **producer-controlled evidence is inside
the evidence threat model**. These are distinct claims.

## Adversarial result and missing primitives

Normal cases: approved, initially over-limit, malformed request, malformed policy,
same-session repeated invocation, abrupt process interruption before/after the
write, limit reduction after computation/check/commit. Both producers abruptly
exit 75 in interruption cases before protocol archival; the parent reconstructs
an explicitly observer-produced record from diagnostics and live storage. An
after-write interruption leaves a PO without producer protocol evidence. Recovery
is observable, not an atomic effect/evidence commit or a power-loss guarantee.

Tampering with stored payloads, input/result/limit/transaction fields or stale
policy stories is rejected, including after the attacker recomputes all hashes.
Missing/extra fields, duplicate JSON keys and booleans substituted for integers
are rejected. The verifier uses a caller-pinned case, never the producer's claim
about which schedule applied. A stateless bounded oracle is not a general VM.

The preserved attack corpus demonstrates two remaining failures in both arms:

1. Without invoking either runtime or writing a PO, an attacker constructs the
   correct expected story and all hashes. Detached verification accepts it.
   A genuinely independent live store observation rejects it; a producer-invented
   copy of that observation is accepted. The missing primitive is an authenticated
   witness of execution/commit tied to an externally trusted authority/event log.
   A producer signature alone only authenticates who made the potentially false claim.
2. An old valid envelope passes verification repeatedly. The host's in-session
   consumed flag blocks a second invocation, but no durable shared transaction
   registry or trusted nonce/epoch checkpoint survives independent sessions.
   The missing primitive is durable one-shot transaction/freshness state bound
   atomically to the effect and authority version. Neither native content addressing
   nor Wasm isolation supplies it.

Cryptographic integrity detects changed bytes; authorization is relative to the
pinned policy schedule; bounded reproducibility recomputes the fixed integer
method; factual correctness requires outside evidence about occurrence and source
facts. Passing the first three cannot establish the fourth. This prevents an
unsigned evidence checker from being reported as a proof of actual execution.

## Measurement and counting rules

Each sample starts a new producer process and fresh store; no warm-up is hidden.
Cases rotate and arms alternate. Steady clocks record nanoseconds. All raw values,
stdout/stderr, exit codes, native traces, file byte inventories and expected
negative-control failures are preserved; output directories are never reused.

| Metric | Timed interval / limitation |
| --- | --- |
| `setup_ns` | Request parsing, store setup, program/runtime creation; Wasm includes compilation/instantiation |
| `execution_ns` | T81 `run_to_halt`, including intrinsic trace/policy work; Wasm invocation minus host callback. Different native observation overhead is retained |
| `authorization_ns` | Host validation and verdict/event construction at the write boundary; A includes policy text construction/parsing and real Axion evaluation |
| `storage_ns` | Protected driver/file write minus the measured authorization interval; A includes CanonFS hashing, no fsync guarantee |
| `evidence_ns` | Common canonical encoding, payload digest and whole-record SHA-256; excludes evidence staging/archive writes |
| `staging_ns` | Common temporary evidence-file creation |
| `evidence_storage_ns` | Native archive write; A includes driver setup and admission inside archival process |
| `archive_process_ns` | A's extra archive process (startup included); B's in-process archive interval |
| `observation_ns` | Parent reads actual protected-file inventory; separate from verification |
| `verification_ns` | In-process bounded replay on captured inventory |
| `verification_process_ns` | Independent CLI startup, live-store inspection and verification |
| `process_ns` | Whole producer subprocess, including startup/stdout/native trace serialization |

Intervals do not form an additive end-to-end total: producer execution and
process intervals overlap; remaining native serialization falls inside process
time. VM intrinsic authorization/trace costs cannot be disentangled through its
stable API; only **side-effect authorization** is timed separately. Tiny workload
measurements include Python FFI overhead and cold-start costs; there is no claim
about general VM throughput, warmed inference, power or a 10x improvement.

`count.py` counts physical/nonblank lines and uncompressed bytes, including
comments, for each adapter, shared protocol, verifier/harness and validation code.
It inventories actual native compile sources and repository header dependencies
from Ninja, plus Python package files and native library bytes from installed
wheel metadata. Full Wasmtime implementation LOC is absent from wheels. Python,
the compiler and system dependencies are explicit uncounted obligations. Adapter
LOC is therefore comparable by a defined rule; full-stack dependency LOC and TCB
size are **not** claimed comparable. No earlier report's performance or source-size
estimate is used as a benchmark result.

Maintenance obligations differ: A retains custom ISA/interpreter, Axion parser,
driver/hash/serialization and their compatibility tests; B retains Wasmtime
updates and sandbox/capability ABI review. Both retain host boundary correctness,
protocol schema/oracle, canonicalization, digest upgrades, replay/freshness state,
trusted witness/key management and interrupted-write recovery. A smaller local
adapter does not by itself establish lower total dependency maintenance cost.

## Decision and smallest next increment

Do not claim that T81VM/Axion/CanonFS is necessary for this honest-host write
property: the conventional capability boundary matches it in the tested domain.
Do not declare the conventional stack a full adversarial winner either. Extracting
the bounded accountable-evidence protocol remains a reasonable candidate, but its
security contract must state exactly which external witness/authority/freshness
anchors are required. First add one durable transaction/authority-version witness
shared across restart, test producer fabrication against it, and atomically couple
its receipt to the protected effect or explicitly record recovery ambiguity.
Only then repeat the same A/B adjudication and consider a third OPA/database arm.

## Preserved native observations

Measured source: `481c6923c7b1f2c0c5d9438add87375a86889b2f`, clean tracked
and untracked working tree. Native macOS ARM64, Python 3.14.3, Apple C++ toolchain,
Release/strict-float configuration; pinned Wasmtime 38.0.0. The
[raw corpus](observed-macos-arm64/corpus.jsonl),
[producer/archive stdout and exit codes](observed-macos-arm64/raw.jsonl),
[verifier outputs](observed-macos-arm64/verification.jsonl),
[all measurements and metadata](observed-macos-arm64/report.json),
[defined source/dependency counts](observed-macos-arm64/counts.json), and
[attack observations](observed-macos-arm64/attacks.json) are committed.
The [initial run](observed-macos-arm64-initial/report.json), adapter compilation
error and corrected build/test logs remain preserved separately as development observations; they are not counted as runtime results.
`sha256.json` detects artifact changes but is not an authenticated manifest.

- 660 producer observations, 30 repetitions per arm/case; no unexpected run,
  verification, within-arm repeatability or paired-protocol differences.
- 600 normal-case observations accepted with actual file-inventory checks;
  60 deliberately unauthorized stale-policy controls rejected with actual PO
  writes preserved. These 60 are counterexamples, not successful protections.
- The 240 normal over-limit/malformed-policy/revocation observations created no
  protected file. All 60 same-session second invocations were denied.
- All 120 controlled before/after interruption processes exited 75. After-write
  interruptions retained the PO; before-write interruptions did not.
- 14 evidence attacks preserved: six rejected; eight accepted instances exposing
  fabricated-history/observer and repeated-envelope gaps (four per arm).

Approved samples, mean ± sample standard deviation in microseconds:

| Interval | A: T81 | B: Wasmtime host |
| --- | ---: | ---: |
| Setup | 345.92 ± 58.93 | 2948.60 ± 6110.57 |
| Execution, native instrumentation retained | 37.82 ± 52.85 | 232.17 ± 45.98 |
| Final side-effect authorization | 10.08 ± 9.97 | 3.01 ± 1.48 |
| Protected storage | 154.93 ± 48.84 | 354.51 ± 257.66 |
| Protocol evidence generation | 148.22 ± 15.84 | 164.68 ± 13.86 |
| Native evidence archive | 511.58 ± 94.52 | 246.66 ± 45.89 |
| In-process verification of captured inventory | 152.44 ± 10.58 | 123.03 ± 10.34 |
| Separate verifier process including live inspection | 67557.11 ± 1084.46 | 67728.81 ± 1384.18 |
| Whole producer process | 6746.45 ± 606.24 | 77934.81 ± 7851.26 |

The cold-start Wasm/Python path is slower here; do not hide that observation
behind the proposal's lower-latency target. High variance, tiny workloads,
different implementation languages and intrinsic tracing prevent a general
runtime performance conclusion. Raw medians/extrema and every sample remain in
the report; no ratio is promoted as evidence that the integrated architecture
is necessary for enforcement.

Final adapter/harness inventory (the measured-source inventory is preserved separately):
nonblank physical lines: A **159**, B **101**; shared runner/verifier/auditor
**368**, validation/counting/attacks **221**. Actual native build closure: **180
source/header files, 36,606 nonblank lines** (includes the A adapter), executable
**1,423,624 bytes**. Installed dependency files: Wasmtime **22,470,881 bytes**,
importlib_resources **285,984 bytes**. These scope-defined inventories are not a
claim that a 99-line host replaces every T81 subsystem or that the external
Wasmtime implementation has no maintenance cost.

Five adversarial unittest methods passed, including 44 further real-arm
observations; detached stdlib corpus audit and determinism-claim guard passed.
The local run alone establishes no cross-platform identity. The separately
inspected Linux/macOS native evidence below extends that bounded claim; Windows
remains subject to its own native observations.

The first native macOS CI run also passed 220 measurements, its five-test suite
and independent corpus audit, then failed uploading raw CanonFS filenames:
GitHub artifacts reject Base81 names containing `<`/`>` (NTFS portability rules).
The [full first-job log](observed-macos-arm64-initial/first-ci-macos-job.json) is preserved.
The workflow now packages the entire native directory tree in a tarball, preserving
names and bytes instead of renaming/skipping objects. A local tar round-trip
compared every archived file with the original bytes. Subsequent native Linux/macOS publication passed after this packaging fix.

The primary preserved run uses explicitly named `host.submit` linking. A
same-signature unsupported-import test ensures import names are enforced, not
merely positional function types. The earlier 660-observation run remains in
`observed-macos-arm64-initial/`; both runs had zero unexpected differences.
Subsequent harness-only portability edits resolve `.exe` for Windows metadata
hashing and retain failing unittest directories. They do not change the measured
calculation, authorization or storage path on macOS.

The first Windows native build succeeded but its suite failed in metadata
collection because an extensionless executable path was passed to `read_bytes`;
the [first Windows job log](observed-macos-arm64-initial/first-ci-windows-job.json)
preserves this failure. Execution via Windows process lookup and direct file
hashing have different suffix requirements. The harness now resolves the actual
`.exe` and emits a failure report even if the runner is absent. Test directories
and full measurements are packaged even after test failure. This fixes the
observed harness issue; it does not assert Windows CanonFS compatibility.

## Native Linux/macOS archive comparison

[Workflow run 39876732879](https://github.com/t81dev/t81-foundation/actions/runs/39876732879)
passed the Linux x86_64 and macOS ARM64 experiment jobs, including artifact
publication. Both executed the five-test suite and 220 measurements with no
unexpected differences; both rejected all 20 stale-policy controls. Downloaded
ZIP SHA-256 digests match the published artifact digests. The committed
[Linux tarball](observed-ci/linux.tar.gz), [macOS tarball](observed-ci/macos.tar.gz),
[download/check summary](observed-ci/summary.json) and
[independent offline audit](observed-ci/offline-audit.json) preserve the native
files and measurements before artifact retention expires.

`audit_archives.py` independently checked every protected/evidence file's actual
tar payload against the recorded byte inventory. All 220 corresponding protocol
envelopes match exactly between native Linux x86_64 and macOS ARM64, including
the intentionally failing controls. Internal native traces and measurements were
not normalized or compared as though they represented identical execution.
The measured CI revision is GitHub's synthetic merge
`decd97b2c29d30104f1de32335b1df901b69ec1c`, for head
`481c6923c7b1f2c0c5d9438add87375a86889b2f` against the requested base. This extends
reproducibility only for the admitted integer contract; it does not establish
occurrence authentication, freshness, numerical conformance or Windows support.

## Preserved Windows portability failure

The corrected native Windows run at head
`1c2fc827655efbeb88b9f05369f03f5c58d3d871` built successfully, then produced
**100 Arm A protected/evidence-storage failures** across 110 attempted A trials.
Only ten A trials completed the full path. All 110 B trials completed: 100 normal
records independently verify, and ten stale-policy controls are rejected.
The suite, measurements and full-corpus audit remain red on Windows; they are
not skipped, allowlisted or reported as passing. The
[complete failure archive](observed-ci/windows-failure.tar.gz),
[job log](observed-ci/windows-job.json) and
[case/path analysis](observed-ci/windows-analysis.json) are committed, including
raw stdout/stderr, failed directories and partial measurements.

All 100 failures align exactly with cases whose protected/evidence Base81 object
basename contains reserved ASCII filename characters. The PO itself resolves to
`2I≈Ewmi5τc<−JTzBE−aV32YVsbλ≠DwfσT9bvRt≈2y.blk`.
The production persistent driver's `object_path` directly appends Base81 text and
its Windows `_open` branch maps failed opens to `DecodeError`.
[Microsoft's filename rules](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
reserve `<`/`>` for these APIs. The exact correlation and source path support the
filename diagnosis; the adapter does not expose errno, so no more specific
encoding/OS error is claimed. Some evidence basenames avoid forbidden characters,
which explains the ten successful A trials. This is a concrete storage-layout
portability disadvantage for A in the tested domain, not an arithmetic failure
or proof that a different CanonFS backend cannot work on Windows.

Repairing production CanonFS filename mapping requires its own identity/persistence
compatibility review and migration tests. It is outside this isolated experiment;
changing the native layout here would erase the observation. The focused PR
therefore remains draft with this real platform failure visible. The architectural
security verdict remains **neither passes the full malicious-producer/freshness
requirements**; B's observed portability benefit does not authenticate its evidence.
