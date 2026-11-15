# BASE-01 Phase B0 final acceptance — November 2, 2026

Disposition: **READY FOR INTEGRATION**, scoped to the non-measuring Lane A B0
validator and the platform limits below. No B1 readiness or authorization follows.
This is a bounded acceptance review by Codex, the implementation author with the
prior session context; it is not independent scientific validation of its own
apparatus. No Lane B implementation was accessed or coordinated.

Review input: PR #688 at `f53afbd162b0b9f935d3418c781e4320312205e7`.
Historical implementation: `54c1a92ffe5c19871c3b1a68c71083ac87fc7f0c`.
The acceptance correction commit is a descendant on the same PR branch. The
original manifest, receipts, original seven-test log and frozen artifacts are
unchanged; new review evidence is in `evidence/acceptance/`.

## Phase A dependency

The bundled `phase-a-manifest.json` is byte-identical to
`docs/research/base01/BASE01_MANIFEST.json` at PR #687 head
`ecd66137bacbce50e605cfc1466ec7d36131ba9e`. SHA-256:
`1367b021f393594e4aab3bc4b52a7b76ca94ee70a49b8f0bda62aea96d46e4ea`.
Protocol `BASE-01`, version `0.1.0-draft`, phase `A`, status `DRAFT_AUDIT_ONLY`,
and `measurement_authorized: false` are preserved.

All 11 classifications match the pinned source:

| ID | Classification | Authorization status |
| --- | --- | --- |
| WL-ARITH-CODEC-I64 | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-ARITH-LIMB-MUL | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-ARITH-SWAR-TRITWISE | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-VM-TISC-DETERMINISM-TIERS | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-VM-HANOI-COMMAND-SURFACE | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-VM-CMP3-DECISION | EXCLUDED | STRICTLY_BLOCKED |
| WL-CANONFS-MEM-THROUGHPUT | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-CANONFS-PERSISTENT-IO | CONDITIONAL | REQUIRES_DEDICATED_FILESYSTEM_QUALIFICATION |
| WL-CANONFS-BLOCK-BACKED-POLICY | EXCLUDED | UNMERGED_DRAFT_PR |
| WL-RES-RUSAGE-METRICS | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |
| WL-RES-ARTIFACT-SIZE | ELIGIBLE | PENDING_PHASE_B_AUTHORIZATION |

Totals: eight eligible, one conditional, two excluded. The exact five minimal
IDs remain codec, TISC determinism tiers, in-memory CanonFS, rusage and artifact
size, in the original order. Audited source remains
`cda9135d36144d4de2771af174e52ac7b30a83c9`; the separate PR #686 reference remains
`ac7397aa5f8751d34adcb6ce2d38e3141e65b36c`, classified draft/unmerged in the snapshot.
CMP3 and H04-R03 Phase C research boundaries retain `FROZEN` and
`measurement_authorized: false`. No historical classification or pin was updated.

At this review PR #687 is open/unmerged at the supplied head. **PR #688 has no
technical prerequisite that PR #687 merge first.** The validator reads its own
bundled snapshot; it does not require `docs/research/base01/` on `main`, fetch
PR #687 at runtime, or derive build sources from its branch. The separate historical identity test
requires pinned Git objects for read-only comparison (README documents an explicit
setup fetch if absent); that is not a dependency on merging PR #687. It can integrate
independently as a snapshot-pinned B0 apparatus. Human review may choose another
merge order. Integrating B0 does not accept all claims in Phase A's prose or
change either phase's measurement authorization.

Material correction: the old runtime checked only the five-ID list, so a changed
protocol, source pin, authorization or classification could pass that check.
`verify_manifest()` now requires the exact full-snapshot hash before parsing it.
The receipt hashes those same checked bytes. Negative tests retain the five IDs
while mutating other fields and confirm rejection. Historical bytes are untouched.

## Resource-oracle reconciliation

| Evidence | Actual scope |
| --- | --- |
| `test_resource_monitoring` | VM policy regressions: tensor count, tensor element budget and symbolic-node limits. It does not call POSIX `getrusage`. |
| `lane_a.resource_validation()` | Actual B0 validation for WL-RES-RUSAGE-METRICS: two orchestrator `getrusage(RUSAGE_SELF)` observations, nonnegative/nondecreasing peak RSS and user/system CPU counters, and platform RSS normalization. |
| Historical receipt `resource_observer` | Correctly states `orchestrator RUSAGE_SELF smoke check only`; status is validated, RSS units KiB, CPU samples not retained. |

Phase A's association of its POSIX oracle with `test_resource_monitoring` is an
attribution error. This addendum and README explicitly supersede that association;
the immutable historical manifest is preserved. The VM executable remains one
of four supporting correctness regressions, not the rusage oracle. The actual
smoke check verifies availability and basic counter invariants; it is not an
independent accuracy oracle, child-workload resource attribution, a monotonic
wall-clock test, RSS enforcement or a performance measurement. New tests execute
the smoke check and reject deliberately decreasing fixture counters. No measured
CPU/RSS samples or invented independent oracle are introduced.

## Measurement isolation

Python `main()` unconditionally rejects `--measure` before `validate()`, source
access, subprocess work or resource collection. No measurement authorization
environment variable is read; the existing environment override negative test
continues to pass. Argparse rejects unsupported selectors/authorization flags.
The only native entry point accepts exactly one argument, `--validate-only`;
every other argument vector exits 2 before setup. New native tests cover no
arguments, measurement/trial/warm-up selectors, protected experiment selectors,
extra arguments and purported environment authorization. All exit 2, emit no
stdout and leave the scratch directory empty.

`base01_interval` has no caller or template instantiation in B0. Inspection of
`runner.cpp`, `kernels.cpp`, `measurement.hpp` and the native object confirms no
interval instantiation or steady-clock reference in the runner object. The
codec batches run once for correctness, without a clock; VM repeated execution
checks signatures. These are not warm-up or trial campaigns. No alternate
measurement dispatcher, PMU collector, SPICE path or protected experiment call
is present. The fixed selected programs cannot select CMP3 or H04-R03 Phase C.
No automatic workflow references Lane A or its measurement path; even a future
invocation of its `--measure` would encounter the unconditional gate.

## Reproducibility and retained evidence

`evidence/acceptance/integrity.json` records verified identities, including:

- Audited source SHA and recomputed tracked-byte digest
  `1c77b63826cd0adb7e5709a750fd6be3e7295bc3bbae9d94726e8c8cf2fa2c77`.
- All 11 historical runner-file hashes checked against Git at the historical
  implementation commit, not against the modified acceptance working tree.
- The manifest byte equality/hash, all nine files in the final evidence index,
  49 archived development files and 11 locally available binary/archive artifacts.
- Retained CMake caches and compile commands byte-equal to the available builds;
  source/runner paths and required build configuration checked. Release/C++23,
  same `/usr/bin/c++`, profiling/benchmarks/llama/TUI/DPE/TernaryOS disabled.
  Current compiler version equals the receipt: Apple clang 21.0.0.
- Four historical correctness exit codes 0; native data for 1,031 codec inputs,
  six VM tiers and three CanonFS sizes. The independent Python positional codec
  check and exact VM input digest match the receipt. Original seven contract
  results are retained; all seven were rerun in acceptance.
- `measurement_authorized: false` and exactly empty `performance_samples`.

Fresh correctness-only acceptance checks, run in a new temporary directory,
re-executed the same four binaries and native `--validate-only`. Their stdout,
stderr and VM signature log match historical output byte-for-byte. New results
are in `rechecks.json`; native raw output references the verified, identical
historical file. No historical receipt was regenerated or relabeled current.
The native source/binary is unchanged by this acceptance patch. Python changes
are limited to full-manifest verification and hashing the checked bytes.

Acceptance suite: **11 tests passed, zero skipped** (the original seven plus
four focused acceptance checks). Exact test command:

```sh
BASE01_ACCEPTANCE_NATIVE=/private/tmp/t81-base01-native-build/base01_lane_a \
  python3 -B -m unittest discover -s experiments/base01_lane_a -p 'test_*.py' -v
```

Other exact commands checked are recorded in `rechecks.json`; the interval
inspection used `nm -C` on the runner object. Hash verification is read-only;
source identity uses `verify_source()`, CMake consistency uses `verify_builds()`,
and retained manifest/receipt identities are covered by
`test_retained_evidence_and_historical_implementation_identity`.
Content identity and successful self-authored checks do not establish provenance
authenticity, independent scientific validation, comparative advantage or CI health.

## Platform and CI scope

macOS ARM64 is validated and was rechecked here with the available original
builds. Linux implementation paths were inspected and RSS conversion remains
unit-tested; Linux execution is **unverified**, with no suitable Linux execution
environment used. Windows is **unsupported**. B1 host qualification is
**incomplete**: macOS affinity unsupported, frequency/boost uncontrolled, original
host load above Phase A's threshold. None is a claim of additional platform coverage.

Read-only GitHub checks confirm original head Format Check succeeded. Integration,
CodeQL and cross-platform-determinism runs on this branch were cancelled, not
passed. The two integration runs' benchmark jobs were cancelled; the older run's
execution steps are explicitly `skipped`, not measured. Raw observations are in
`ci-runs-before.json` and the two `ci-*-benchmark-status.json` files.

Safe bounded checks are local unit tests, selected correctness executables,
native `--validate-only`, identity checks, compiler metadata and `git diff --check`.
The standalone Format Check workflow is non-measuring but does not format files
under this experiments path; its old success is not substantive validation of
this code. No workflow was triggered during this review. Do not dispatch/rerun
`ci.yml` as a substitute: it contains general benchmark and tritwise smoke jobs.
The correction commit uses `[skip ci]` to avoid automatic PR benchmarking. No CI
or governance workflow was modified and no overall CI pass is claimed.

## Acceptance and remaining limits

There are **no remaining B0 acceptance blockers** within the stated scope.
Full manifest identity is enforced, resource attribution is reconciled, historical
evidence is intact and reproducible locally, and both entry points remain
non-measuring. Ready for human integration consideration; no merge is performed.

B1 authorization, a qualified host, campaign activation and timer/denominator
qualification remain outside B0. They are not requirements to integrate a
non-measuring validator. No performance measurement, PMU collection, SPICE,
CMP3/H04-R03 execution, Phase B1 implementation/activation, frozen-protocol edit,
unauthorized CI benchmarking or merge occurred in this review.

Exact files changed by this acceptance increment (relative to
`experiments/base01_lane_a/`):

- `lane_a.py`
- `test_b0_acceptance.py`
- `README.md`
- `B0_ACCEPTANCE.md`
- `evidence/acceptance/contract-tests.log`
- `evidence/acceptance/integrity.json`
- `evidence/acceptance/rechecks.json`
- `evidence/acceptance/ci-runs-before.json`
- `evidence/acceptance/ci-head-benchmark-status.json`
- `evidence/acceptance/ci-older-benchmark-status.json`
- `evidence/acceptance/sha256.json`
