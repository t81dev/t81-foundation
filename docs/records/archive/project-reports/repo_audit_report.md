# Repository Classification Audit Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Repository Classification Audit Report](#repository-classification-audit-report)
  - [1. Root Item Classification](#1-root-item-classification)
  - [2. Risk Assessment Summary](#2-risk-assessment-summary)
  - [3. Remediation Actions](#3-remediation-actions)

<!-- T81-TOC:END -->


## 1. Root Item Classification

| Item | Classification | Risk Assessment | Remediation |
| :--- | :--- | :--- | :--- |
| `.devcontainer/` | Build/Config | Low | No |
| `.git/` | Version Control | Low | No |
| `.github/` | CI/Governance | Medium (Critical Workflows) | Review Actions |
| `.t81_canonfs/` | Generated Artifact | High (Should not be in repo) | Add to .gitignore |
| `benchmarks/` | Canonical Source | Low | Move results to docs |
| `docs/developer-guide/book/` | Canonical Documentation | Medium (Multilingual drift) | Implement policy |
| `cmake/` | Build/Config | Low | No |
| `contracts/` | Canonical Source | Medium | Verify relevance |
| `docs/` | Canonical Documentation | High (Sprawl) | Restructure |
| `examples/` | Canonical Documentation | Low | No |
| `include/` | Canonical Source | Medium (Public API) | No |
| `legacy/` | Historical Archive | Low | No |
| `logs/` | Generated Artifact | Medium (Should not be in repo) | Add to .gitignore |
| `notebooks/` | Experimental | Low | No |
| `pdf/` | Generated Artifact | Low | No |
| `docs/governance/archive/policy/` | Governance | Medium | Merge into docs/governance |
| `scripts/` | Canonical Source | Medium (CI/Build) | No |
| `spec/` | Canonical Specification | High (Source of Truth) | No |
| `src/` | Transitional Source | Medium (Remaining modules pending migration) | Track in restructure checklist |
| `tests/` | Canonical Source | High (Quality Gate) | No |
| `tools/` | Canonical Source | Low | No |
| `.clang-format` | Build/Config | Low | No |
| `.clang-tidy` | Build/Config | Low | No |
| `.cursorrules` | Build/Config | Low | No |
| `.editorconfig` | Build/Config | Low | No |
| `.gitattributes` | Version Control | Low | No |
| `.gitignore` | Version Control | Medium | Update required |
| `.pre-commit-config.yaml` | Build/Config | Low | No |
| `CMakeLists.txt` | Build/Config | High (Build System) | No |
| `CMakePresets.json` | Build/Config | Low | No |
| `CODEOWNERS` | Governance | High | No |
| `CODE_OF_CONDUCT.md` | Governance | Medium | Move to docs/governance? |
| `CONTRIBUTING.md` | Governance | Medium | Move to docs/governance? |
| `docs/spec/vm/opcode_reference.md` | Canonical Documentation | Low | Move to docs/spec |
| `LICENSE` | Legal | High | No |
| `README.es.md` | Canonical Documentation | Medium (Translation) | Apply Policy |
| `README.md` | Canonical Documentation | High (Entry Point) | Keep |
| `README.pt-BR.md` | Canonical Documentation | Medium (Translation) | Apply Policy |
| `README.ru.md` | Canonical Documentation | Medium (Translation) | Apply Policy |
| `README.zh-CN.md` | Canonical Documentation | Medium (Translation) | Apply Policy |
| `SECURITY.md` | Governance | High | Move to docs/governance? |
| `benchmark_raw.txt` | Generated Artifact | Medium | Move to benchmarks/results |
| `benchmark_results_*.txt` | Generated Artifact | Medium | Move to benchmarks/results |
| `dummy.gguf` | Generated Artifact | Low | Add to .gitignore |
| `dummy.safetensors` | Generated Artifact | Low | Add to .gitignore |
| `dummy.t81w` | Generated Artifact | Low | Add to .gitignore |
| `generate_dummy_safetensors*` | Build Output | High | Add to .gitignore |
| `generate_dummy_safetensors.cpp` | Experimental | Low | Move to tools/ or delete |
| `package-lock.json` | Build/Config | Low | No |
| `package.json` | Build/Config | Low | No |
| `pyproject.toml` | Build/Config | Low | No |
| `sanitizer_audit.log` | Generated Artifact | Low | Add to .gitignore |

## 2. Risk Assessment Summary

*   **Artifact Leakage**: Multiple benchmark results and binary/generated files (`.t81_canonfs`, `dummy.*`, `generate_dummy_*`) are present in the root. These should be gitignored.
*   **Documentation Sprawl**: `docs/` is disorganized. `policy/` exists separately from `docs/governance`.
*   **Multilingual Drift**: Multiple README translations exist at root without a clear synchronization mechanism.

## 3. Remediation Actions

*   **Immediate**: Update `.gitignore` to exclude generated artifacts.
*   **Immediate**: Structure `docs/` according to the new governance plan.
*   **Immediate**: Move benchmark results to `benchmarks/results/`.
