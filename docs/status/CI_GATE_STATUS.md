# CI Gate Status

Status: Active
Last Updated: 2026-03-19
Owner: @t81dev
Reference Candidate: `61c2edf6` (origin/main, 2026-03-19)
Current Main Head: `61c2edf6` (origin/main, 2026-03-19; Axion epoch parity and VM memory/state determinism proofs are wired into CI and governance docs)

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
| `quality gate / required` | `ci.yml` | **Yes** | completed / success ✅ (`03112f6c`) |
| `Analyze (cpp)` | `codeql.yml` | **Yes** | success ✅ (`03112f6c`) |

Verification command:

```sh
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
| `gate / determinism repeatability / linux-x86_64 / clang` | `ci.yml` | unknown ⏳ | Same-machine bit-identity for VM workload signatures; new gate added in PR #448 |
| `gate / determinism slice / linux-x86_64 / clang` | `ci.yml` | success ✅ | Full determinism slice on Linux/x86_64/clang |
| `gate / axion epoch determinism / linux-x86_64 / clang` | `ci.yml` | success ✅ | Experimental TernaryOS + DPE lane proving pooled-vs-unbounded kernel scheduler/audit parity |
| `cross-compile / linux-armv9 / gcc (informational)` | `ci.yml` | unknown ⏳ | ARMv9 cross-compile build; new gate added in PR #448; `continue-on-error: true` |
| `experimental architectures / group anchor` | `ci.yml` | skipped (nightly only) | Schedule-only anchor job for nightly experimental gates |
| `experimental / oneapi sycl sanity` | `ci.yml` | skipped (nightly only) | Intel oneAPI SYCL hello-world compile check; `continue-on-error: true` |
| `experimental / ascend cann sanity` | `ci.yml` | skipped (nightly only) | Ascend CANN toolchain check; requires `T81_ENABLE_ASCEND_CI == '1'`; `continue-on-error: true` |
| `build / macos-x86_64 / gcc` | `ci.yml` | success ✅ | |
| `build / macos-arm64 / clang` | `ci.yml` | success ✅ | |
| `build / linux-x86_64 / gcc` | `ci.yml` | success ✅ | |
| `build / linux-x86_64 / clang` | `ci.yml` | success ✅ | |
| `build / linux-arm64 / clang` | `ci.yml` | success ✅ | |
| `build / windows-x86_64 / msvc` | `ci.yml` | unknown ⏳ | Sparse checkout now includes `benchmarks/` and `spec/`; awaiting first clean run post-PR #448 |
| `build / windows-x86_64 / clang-cl` | `ci.yml` | unknown ⏳ | Sparse checkout now includes `benchmarks/` and `spec/`; awaiting first clean run post-PR #448 |
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

## Operational Notes (2026-03-19)

- **Axion epoch scheduler/audit parity promoted into CI** — new
  `gate / axion epoch determinism / linux-x86_64 / clang` lane builds the
  TernaryOS + DPE epoch tests and hard-fails on pooled-vs-unbounded divergence
  for execution and audit outcomes.
- **Governance alignment refreshed** — the determinism registry, enforcement
  matrix, project control center, and system-status pages now reference the new
  epoch proof lane as part of the governed non-DCP enforcement story for the
  experimental Axion/TernaryOS boundary.

## Operational Notes (2026-03-15)

- **RFC-0031 + RFC-0032 accepted** — All 5 AI subsystem promotion phases complete.
  New test targets added to CMake: `backend_adapter_test` (Phase 4), `t81_determinism_evidence_test` (Phase 5).
- **AI conformance suite expanded to 27 programs** — Added `spec/conformance/ai/`: `attn-determinism.t81`, `qmatmul-scale-order.t81`, `embed-bounds-check.t81`.
- **Axion event registry created** — `spec/supplemental/axion-event-registry.md` is now the normative source for `model_load`, `attn_guard`, `qmatmul_guard`, `ai_exec_gate` identifiers.
- **ai-opcode-phase1-conformance.md**: `phase_status` advanced to `spec_conformant`.
- **Test count**: 344/344 tests passing after build system fix (stale CMake generator mismatch in asio/googlebenchmark/ftxui subbuilds cleared; Ninja reconfigure). R-18 closed.
- **RFC-0002 accepted** — DEC §11 (Conformance Tests) fulfilled; stub replaced with concrete program references.
- **RFC-00A series finalized** — 00A0/A1/A5/A8 superseded; 00A3/A4/A6 accepted.

## Operational Notes (2026-03-14)

- **PR #448 merged** — Restored canonical `ci.yml` from `restore-to-main-293223a8` with three targeted fixes:
  - lychee pinned to v0.18.1; `--root-dir` set to absolute `${{ github.workspace }}`; `github.com/ggerganov/*` excluded (GitHub rate-limits CI runners); `docs/policies/AGENTS.md` removed from inputs (covered by glob)
  - CLI manual path corrected: `docs/guides/cli-user-manual.md` → `docs/user-guide/reference/cli-user-manual.md`
  - `timeout-minutes` added to all jobs that were missing them (prevents Homebrew/runner hangs blocking the queue)
- **New gates in canonical workflow**: `gate / determinism repeatability`, `cross-compile / linux-armv9 / gcc`, nightly experimental gates (`oneapi-sycl-sanity`, `ascend-cann-sanity`)
- **`T81String::serialize_canonical()` added** (PR #446 / `bda2f089`) — fixed `CanonHash<T81String>` falling to raw-bytes SSO-buffer fallback, causing non-deterministic hashes; resolved `t81_determinism_containers_test` failure; test count now 338/338

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

### TLOADHASH Tests — Closed (2026-03-15)

| Field | Value |
| :--- | :--- |
| **Tests affected** | `t81_vm_tloadhash_conformance_test`, `t81_vm_tloadhash_canonical_fixed_test`, `t81_vm_tloadhash_decodefault_determinism_matrix_test` |
| **Symptom** | Tests appeared as SEGFAULTs; root cause was stale CMake generator mismatch (Unix Makefiles vs Ninja) in asio-subbuild, googlebenchmark-subbuild, and ftxui-subbuild caches |
| **Classification** | Closed — not a code defect; build environment issue only |
| **Resolution** | Cleared stale subbuild caches; reconfigured with default Ninja preset; all 3 tests pass. 344/344 passing. |
| **Blocking Release** | No — resolved |
| **Tracking** | R-18 closed in `docs/status/ACTIVE_RISKS.md` |

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

### 2026-03-14 CI Incident — Resolved

| Field | Value |
| :--- | :--- |
| **Affected SHA(s)** | Multiple pre-PR #448 runs on `fix-ci-broken-links-lychee` |
| **Observed Symptoms** | (1) lychee v0.23.0 tokio channel panic (exit 101, `SendError`); (2) `--root-dir .` rejected as non-absolute path in lychee v0.18.1; (3) `docs/policies/AGENTS.md` treated as URL; (4) GitHub 429 rate-limit on `github.com/ggerganov/*` links; (5) CLI docs steps failing with `missing CLI manual: docs/guides/cli-user-manual.md`; (6) `t81_determinism_containers_test` failing — `CanonHash<T81String>` falling to SSO raw-bytes hash; (7) macOS x86_64 GCC benchmark job hanging 26+ min on `brew install` |
| **Root Causes** | lychee version not pinned (defaulted to v0.23.0 which has a tokio panic bug); relative `--root-dir`; CLI manual moved but path not updated; `T81String` had no `serialize_canonical()`, causing non-deterministic hash via uninitialized SSO bytes; no `timeout-minutes` on benchmark and other jobs |
| **Fixes Applied** | `bda2f089` (T81String::serialize_canonical() + CI fixes); PR #448 (canonical ci.yml restore with lychee v0.18.1 pin, absolute root-dir, ggerganov exclude, correct CLI path, all job timeouts) |
| **Validation** | PR #448 merged to main as `b566bff8`; 338/338 tests passing |

### 2026-03-05 CI Incident — Resolved

| Field | Value |
| :--- | :--- |
| **Affected SHA(s)** | `401a049e`, `b20934be` pre-fix window |
| **Observed Symptoms** | `T81 Foundation CI` matrix failures across clang/sanitizers/fuzz/clang-tidy; `Format Check` fallback scanning full tree |
| **Primary Root Cause** | Corrupted `include/t81/support/expected.hpp` fallback implementation (malformed class body) |
| **Secondary Root Cause** | `format.yml` checkout depth too shallow for `HEAD^..HEAD` diff on push |
| **Fixes Applied** | `57f1a96c` (restore `expected.hpp` fallback), `b20934be` (`format.yml` `fetch-depth: 2`) |
| **Validation** | Local clean build + `ctest` 338/338 pass; `T81 Foundation CI` success on run `22722029938`; `CodeQL` success on run `22722029925` |

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
