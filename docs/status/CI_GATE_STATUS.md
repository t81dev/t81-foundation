# CI Gate Status

Status: Active
Last Updated: 2026-03-06
Owner: @t81dev
Reference Candidate: `aab6c719` (origin/main, 2026-03-06)
Current Main Head: `aab6c719` (origin/main, 2026-03-06; CI red at last refresh)

## Purpose

Provide a standing, day-to-day view of CI gate health across all workflows:
required gates, informational gates, known failures, flaky-test inventory, and
the benchmark guardrail signal. This document is not release-specific; it tracks
the health of the CI surface as a whole.

For release-specific gate evidence against a candidate SHA, see
`docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md`.

## Branch Protection: Required Contexts

These contexts must be `completed` + `success` on any candidate SHA before a
GO decision can be stamped.

| Context | Workflow | Required | Last Known Status |
| :--- | :--- | :--- | :--- |
| `quality gate / required` | `ci.yml` | **Yes** | completed / failure ❌ (`aab6c719`) |
| `Analyze (cpp)` | `codeql.yml` | **Yes** | in progress ⏳ (`aab6c719`) |

Verification command:

```
gh api repos/t81dev/t81-foundation/branches/main/protection \
  --jq '.required_status_checks.contexts'

gh api repos/t81dev/t81-foundation/commits/<sha>/check-runs
```

## Informational Gates (Non-Required)

These run on every push/PR but are not branch-protection blocking. Failures are
tracked here and must be addressed unless explicitly deferred.

| Gate | Workflow | Last Known Status | Notes |
| :--- | :--- | :--- | :--- |
| `gate / t81lang cross-arch bit-identity` | `ci.yml` | success ✅ | T81Lang compiler output bit-identical across x86_64 and arm64 |
| `gate / t3k cross-arch bit-identity` | `ci.yml` | success ✅ | T3K workload bit-identical across architectures |
| `gate / tritwise-determinism / no-simd` | `ci.yml` | success ✅ | Tritwise ops deterministic without SIMD |
| `gate / tritwise-determinism / avx2-asan` | `ci.yml` | success ✅ | Tritwise ops deterministic with AVX2 + ASAN |
| `gate / determinism slice / linux-x86_64 / clang` | `ci.yml` | success ✅ | Full determinism slice on Linux/x86_64/clang |
| `build / macos-x86_64 / gcc` | `ci.yml` | success ✅ | |
| `build / macos-arm64 / clang` | `ci.yml` | success ✅ | |
| `build / linux-x86_64 / gcc` | `ci.yml` | success ✅ | |
| `build / linux-x86_64 / clang` | `ci.yml` | success ✅ | |
| `build / linux-arm64 / clang` | `ci.yml` | success ✅ | |
| `build / windows-x86_64 / msvc` | `ci.yml` | failure ❌ | Configure failed due sparse checkout omission of `benchmarks/` + `spec/conformance/` |
| `build / windows-x86_64 / clang-cl` | `ci.yml` | failure ❌ | Configure failed due sparse checkout omission of `benchmarks/` + `spec/conformance/` |
| `build / sanitizers` | `ci.yml` | success ✅ | ASAN + UBSAN build |
| `static analysis / clang-tidy` | `ci.yml` | success ✅ | |
| `fuzzing / frontend` | `ci.yml` | success ✅ | Frontend fuzz harness |
| `formal verification / ternary logic` | `ci.yml` | success ✅ | |
| `benchmark / linux-x86_64` | `bench.yml` | success ✅ | |
| `benchmark / vm workload gate` | `bench.yml` | success ✅ | Guardrail: VM workload dispatch/native ratio |
| `cxx-std / linux-x86_64 / clang / ON` | `ci.yml` | success ✅ | C++23 feature set |
| `cxx-std / linux-x86_64 / clang / OFF` | `ci.yml` | success ✅ | C++20 feature set |
| `governance-metrics` | `ci.yml` | success ✅ | Determinism claim registry check |
| `lint / spec & docs` | `ci.yml` | success ✅ | Markdown links + spec structure |
| `contract-sync` | `runtime-contract.yml` | success ✅ | Runtime contract alignment |
| `format / clang-format` | `format.yml` | success ✅ | |
| `architecture / invariants (informational)` | `ci.yml` | success ✅ | |
| `product / dcp integrity (informational)` | `ci.yml` | success ✅ | |
| `build` (GitHub Pages / Jekyll) | `documentation.yml` | **failure ⚠️** | **Mitigating** — root `_config.yml` third_party exclusion patch queued; awaiting next run |

## Operational Notes (2026-03-06)

- Added CI gate `gate / determinism repeatability / linux-x86_64 / clang` to
  enforce same-machine bit-identity for
  `vm_workload_determinism_signatures.log` across repeated runs.
- Promoted `gate / t3k cross-arch bit-identity` and
  `gate / t81lang cross-arch bit-identity` to required quality-gate
  dependencies.
- Added architecture coherence invariant check:
  `scripts/ci/check_architecture_coherence.py`.

## Known Failures

### Jekyll Pages Build — Mitigating

| Field | Value |
| :--- | :--- |
| **Workflow** | `documentation.yml` |
| **Gate** | `build` (GitHub Pages / Jekyll) |
| **Run** | `22514402961` |
| **Root Cause** | Jekyll renders `third_party/llama.cpp/examples/model-conversion/scripts/embedding/modelcard.template` — Liquid `{{ }}` syntax causes a Jekyll Liquid rendering error |
| **Classification** | Mitigating — non-required workflow; no impact on C++ release artifact, determinism, or DCP guarantees |
| **Decision** | DEC-005 in `docs/status/DECISION_LOG.md` |
| **Resolution Path** | Root `_config.yml` now excludes `third_party/`; verify on next Pages/Jekyll run |
| **Blocking Release** | No |

### 2026-03-05 CI Incident — Resolved

| Field | Value |
| :--- | :--- |
| **Affected SHA(s)** | `401a049e`, `b20934be` pre-fix window |
| **Observed Symptoms** | `T81 Foundation CI` matrix failures across clang/sanitizers/fuzz/clang-tidy; `Format Check` fallback scanning full tree |
| **Primary Root Cause** | Corrupted `include/t81/support/expected.hpp` fallback implementation (malformed class body) |
| **Secondary Root Cause** | `format.yml` checkout depth too shallow for `HEAD^..HEAD` diff on push |
| **Fixes Applied** | `57f1a96c` (restore `expected.hpp` fallback), `b20934be` (`format.yml` `fetch-depth: 2`) |
| **Validation** | Local clean build + `ctest` 285/285 pass; `T81 Foundation CI` success on run `22722029938`; `CodeQL` success on run `22722029925` |

## Benchmark Guardrail Signal

| Gate | Current Threshold | Last Status | Notes |
| :--- | :--- | :--- | :--- |
| `benchmark / vm workload gate` — dispatch/native ratio | Enforced (threshold in `bench.yml`) | Pass ✅ | Guardrail verifies that VM workload dispatch does not regress below the established native-execution ratio. Alert threshold: >5% regression vs. baseline |

**Variability note:** Benchmark results are sensitive to runner environment.
If two consecutive runs on identical environments diverge by >5%, revisit the
guardrail threshold. See `R-06` in `docs/status/ACTIVE_RISKS.md`.

## Flaky Test Inventory

No known flaky tests as of 2026-02-28. If a flaky test is identified:

1. Open a tracking issue referencing the test name, failure mode, and
   reproduction frequency.
2. Add an entry to this section with the issue link.
3. If flakiness is environment-specific, note the environment constraint.

| Test | Failure Mode | Frequency | Environment | Issue | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| (none) | — | — | — | — | — |

## Repro Ledger

Long-running reproducibility evidence is managed by the `repro-ledger.yml`
workflow. The repro ledger records canonical output hashes for key workloads
across platforms.

| Workload | Last Ledger Update | Status |
| :--- | :--- | :--- |
| T81Lang compiler repro gate | 2026-02-28 | Pass ✅ |
| T3K cross-arch bit-identity | 2026-02-28 | Pass ✅ |
| Tritwise determinism slices | 2026-02-28 | Pass ✅ |

Repro hash refresh is automated via `t81lang-repro-hash-refresh.yml` and
`scripts/ci/t81lang_repro_gate.py`.

## Stdlib Surface Baseline Gate

The stdlib surface baseline CI gate (`check_stdlib_surface_baseline.py`) is
wired into `ci.yml` and the governance hygiene check. It enforces that the
module surface of `lang/stdlib/std/` matches the declared baseline.

| Gate | Last Status |
| :--- | :--- |
| `check_stdlib_surface_baseline.py` | Pass ✅ |
| `check_stdlib_promotion_snapshot.py` | Pass ✅ |

## Governance and Hygiene Gates

| Gate | Script | Last Status |
| :--- | :--- | :--- |
| Governance policy structure / license / artifact | `check_root_structure.py`, `check_readme_naming.py` | Pass ✅ |
| Docs governance hygiene | `check_docs_governance_hygiene.py` | Pass ✅ |
| Public API semver lock | `check_public_api_semver.py` | Pass ✅ (lock refreshed 2026-03-05) |
| RFC lifecycle hygiene | `check_rfc_lifecycle_hygiene.py` | Pass ✅ |
| TISC v1.1.0 freeze integrity | `check_tisc_freeze_integrity.py` | Pass ✅ |
| Architecture target table vs. CMake | `check_architecture_targets.py` | Pass ✅ |
| Workflow action pinning policy | `audit_workflow_actions.py` | Pass ✅ |
| Workflow permissions policy | `audit_workflow_permissions.py` | Pass ✅ |
| C2 month-close check | `c2_month_close_check.py` | Pass ✅ |
| C2 month-close preflight | `c2_month_close_preflight.py` | Pass ✅ |

## Cadence

- Update this document when any gate transitions from pass to failure (or vice
  versa) on `main`.
- Refresh the reference candidate SHA and known-failure entries at each release
  packet cycle.
- Benchmark guardrail and stdlib baseline gates reviewed at C2 month-close.

## Cross-References

- `docs/records/audits/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/records/audits/RECENT_COMMIT_AUDIT_2026-03-05.md`
- `docs/status/DECISION_LOG.md` (DEC-005 — Jekyll deferred)
- `docs/status/ACTIVE_RISKS.md` (`R-06` — benchmark variability; `R-07` — Jekyll)
- `.github/workflows/ci.yml`
- `.github/workflows/codeql.yml`
- `.github/workflows/bench.yml`
- `.github/workflows/repro-ledger.yml`

## Versioning Statement

This document is an operational CI health artifact. It does not override `/spec`,
freeze policy, or determinism registry boundaries.
