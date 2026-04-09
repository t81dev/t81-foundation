# Dependency Health

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Dependency Health](#dependency-health)
  - [Purpose](#purpose)
  - [Dependency Inventory](#dependency-inventory)
    - [Build Tooling](#build-tooling)
    - [Vendored Dependencies](#vendored-dependencies)
    - [FetchContent Dependencies](#fetchcontent-dependencies)
    - [GitHub Actions (CI Tooling)](#github-actions-ci-tooling)
  - [Dependency Detail](#dependency-detail)
    - [llama.cpp (Vendored)](#llamacpp-vendored)
    - [Asio — Standalone (FetchContent)](#asio-—-standalone-fetchcontent)
    - [CMake](#cmake)
    - [Python (CI / Governance Scripts)](#python-ci--governance-scripts)
  - [Known CVEs and Security Advisories](#known-cves-and-security-advisories)
  - [Upgrade Review Cadence](#upgrade-review-cadence)
  - [Determinism Impact Assessment](#determinism-impact-assessment)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-03-14
Owner: @t81dev
Version: 1.0.0

## Purpose

Track the health of external dependencies: versions pinned or vendored, upgrade
policy, known CVEs or breakages, and the review cadence. Critical for a
deterministic system where dependency version drift can break bit-exact
reproducibility.

## Dependency Inventory

### Build Tooling

| Dependency | Minimum Required | Recommended | Pinned / Enforced | Source | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CMake** | 3.16 | Latest 3.x stable | `cmake_minimum_required(VERSION 3.16)` | System install | CMakePresets.json used for preset-based builds |
| **C++ Standard** | C++20 | C++23 (`T81_USE_CXX23=ON`) | Enforced via `target_compile_features` | — | C++20 required; C++23 optional |
| **AppleClang** | 17+ | Latest | No explicit lock | System Xcode | macOS primary dev compiler |
| **Clang** | 18+ | Latest | No explicit lock | System / CI `ubuntu-latest` | Linux CI primary |
| **GCC** | 14+ | Latest | No explicit lock | System / CI | Used in Linux build matrix |
| **MSVC** | Recent | — | No explicit lock | System | Windows builds; `/W4` flag enforced |
| **Python** | 3.12 | 3.12 | `python-version: "3.12"` in CI | GitHub Actions `setup-python` | Governance scripts, CI gates |

### Vendored Dependencies

| Dependency | Vendored Path | Pinned Commit / Tag | Upstream | DCP Scope | Upgrade Policy |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **llama.cpp** | `third_party/llama.cpp` | `3769fe6eb` (HEAD ~2026-02) | https://github.com/ggml-org/llama.cpp | **Non-DCP** (governed non-DCP) | Manual; requires governing AGI pipeline review before upgrade |

### FetchContent Dependencies

| Dependency | FetchContent Tag | Upstream | DCP Scope | Upgrade Policy |
| :--- | :--- | :--- | :--- | :--- |
| **Asio** (standalone) | `asio-1-30-2` | https://github.com/chriskohlhoff/asio | Non-DCP (network / distributed only) | Manual; upgrade when network/distributed surfaces move beyond experimental |

### GitHub Actions (CI Tooling)

All Actions are pinned to full commit SHAs, enforced by `scripts/ci/audit_workflow_actions.py`
with `--max-tagged 0 --max-unknown 0`. No tagged-only or floating references are
permitted.

| Action | Pin Policy | Last Audit Status |
| :--- | :--- | :--- |
| `actions/checkout` | Full SHA (`de0fac2e...`) | Pass ✅ |
| `actions/setup-python` | Full SHA (`a309ff8b...`) | Pass ✅ |
| `lycheeverse/lychee-action` | Full SHA (`a8c4c7cb...`) | Pass ✅ |
| All other actions | Full SHA required | Enforced by `audit_workflow_actions.py` |

## Dependency Detail

### llama.cpp (Vendored)

| Field | Value |
| :--- | :--- |
| **Path** | `third_party/llama.cpp` |
| **Pinned Commit** | `3769fe6eb` |
| **Upstream** | https://github.com/ggml-org/llama.cpp |
| **Integration Surface** | `tooling/model/llama_cpp_adapter.cpp`, `t81_llama_adapter` static library, CLI `llama-run` |
| **DCP Classification** | **Governed non-DCP** (see DEC-003 in `docs/status/DECISION_LOG.md`) |
| **CMake Flag** | `T81_ENABLE_LLAMA_CPP=OFF` (default disabled) |
| **Export Status** | Internal / build-only; `T81_EXPORT_LLAMA_ADAPTER` guard active (see DEC-004) |
| **Known Issues** | Modelcard templates use Liquid `{{ }}` syntax, breaking Jekyll Pages build (RISK-007, mitigation patch queued; pending CI verification) |
| **Upgrade Trigger** | Security advisory, API breaking change, or governed AGI promotion pipeline advancement |
| **Upgrade Process** | 1. Update submodule/clone commit. 2. Run `llama_kernels_test` and `llama_cpp_governed_demo`. 3. Re-run repro gate (`scripts/ci/llama_cpp_repro_gate.py`). 4. Log decision in `docs/status/DECISION_LOG.md`. 5. Update this document. |
| **CVEs / Advisories** | None tracked as of 2026-02-28. Monitor upstream releases. |

### Asio — Standalone (FetchContent)

| Field | Value |
| :--- | :--- |
| **FetchContent Tag** | `asio-1-30-2` |
| **Upstream** | https://github.com/chriskohlhoff/asio |
| **Usage Surface** | `experimental/distributed/`, `examples/complex_demo.cpp`, `t81_network_test` |
| **DCP Scope** | Non-DCP — network/distributed paths only |
| **Build Flag** | Pulled automatically when distributed target is built |
| **Known Issues** | None tracked as of 2026-02-28 |
| **Upgrade Policy** | Upgrade when distributed compute moves beyond experimental, or on security advisory. Test `t81_network_test` after upgrade. |

### CMake

| Field | Value |
| :--- | :--- |
| **Minimum** | 3.16 |
| **Recommended** | Latest 3.x stable |
| **Enforcement** | `cmake_minimum_required(VERSION 3.16)` in root `CMakeLists.txt` |
| **Known Issues** | None |
| **Upgrade Policy** | No proactive upgrade required. If a new CMake feature is used, bump minimum version in `CMakeLists.txt`. |

### Python (CI / Governance Scripts)

| Field | Value |
| :--- | :--- |
| **CI Version** | 3.12 |
| **Local Requirement** | 3.10+ recommended for governance scripts |
| **Libraries** | `mdformat`, `mdformat-gfm`, `mdformat-frontmatter`, `mdformat-toc` (CI); governance scripts use stdlib only |
| **Known Issues** | None |
| **Upgrade Policy** | Bump CI Python version in workflows when 3.12 reaches EOL or a critical vulnerability is found. Test all `scripts/governance/` and `scripts/ci/` scripts after bump. |

## Known CVEs and Security Advisories

| Dependency | CVE / Advisory | Severity | Status | Resolution |
| :--- | :--- | :--- | :--- | :--- |
| (none tracked) | — | — | — | — |

*Monitor upstream repositories for new advisories. llama.cpp in particular has
an active release cadence.*

## Upgrade Review Cadence

| Dependency | Review Frequency | Trigger |
| :--- | :--- | :--- |
| llama.cpp | Monthly (at C2 close) or on advisory | Security advisory, API break, or AGI pipeline promotion |
| Asio | Quarterly or on advisory | Security advisory or distributed maturation |
| CMake | On-demand | New language feature required or advisory |
| Python | On-demand | EOL or critical CVE |
| GitHub Actions | Every PR via `audit_workflow_actions.py` | Any workflow change |
| Compilers | On-demand | New C++ standard adoption or advisory |

## Determinism Impact Assessment

For a deterministic system, dependency upgrades carry bit-exact reproducibility
risk. Required checks after any dependency upgrade:

1. Run `gate / tritwise-determinism / no-simd` and `/ avx2-asan`.
2. Run `gate / t81lang cross-arch bit-identity` and `gate / t3k cross-arch bit-identity`.
3. Run `gate / determinism slice / linux-x86_64 / clang`.
4. Run `formal verification / ternary logic`.
5. If reproducibility hashes change: refresh `repro-ledger.yml` and update
   the T81Lang repro hash baseline (`t81lang-repro-hash-refresh.yml`).
6. If DCP-scoped behavior changes: require governance review and spec update.

## Cross-References

- `docs/status/CI_GATE_STATUS.md`
- `docs/status/DECISION_LOG.md` (DEC-003, DEC-004)
- `docs/status/RISK_REGISTER.md` (RISK-007)
- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `CMakeLists.txt`
- `scripts/ci/audit_workflow_actions.py`
- `scripts/ci/llama_cpp_repro_gate.py`

## Versioning Statement

This document is an operational dependency health artifact. It does not override
`/spec`, freeze policy, or determinism registry boundaries.
