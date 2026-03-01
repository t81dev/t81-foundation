# Structural Integrity Report

Date: 2026-02-25
Scope: Phase 10 structural invariants enforcement

## Dependency Firewall Status

- Script: `scripts/architecture/check_dependency_firewall.py`
- Result: PASS (no unwaived violations)
- Waivers: 1
  - `CORE_DEPENDS_EXPERIMENTAL` at `core/vm/vm.cpp:24` for include `t81/experimental/cog/promotion.hpp`
  - Rationale tracked in `scripts/architecture/dependency_firewall_waivers.tsv`

## Public API Boundary Confirmation

- Public header rule enforced: `include/t81/**` must not include `internal/**`.
- Relative traversal from public headers into internal headers is forbidden.
- Scanner result: no unwaived violations.

## Experimental Containment Confirmation

- Rule enforced: `core/**` must not include `experimental/**`.
- Current state: one explicit waived include in VM (`core/vm/vm.cpp`), no additional violations.

## Legacy Path References Confirmation

- Script: `scripts/architecture/check_legacy_paths.sh`
- Result: PASS
- Legacy path patterns (for active scope) are absent, with historical exceptions explicitly excluded:
  - `docs/records/archive/architecture/REPO_RESTRUCTURE_MASTERPLAN.md`
  - `docs/records/archive/architecture/RESTRUCTURE_PHASE1_PREFLIGHT.md`
  - `scripts/restructure/phase1_scan.sh`
  - audit/history directories

## Validation Summary

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`: PASS
- `cmake --build build --parallel`: PASS
- `ctest --test-dir build --output-on-failure`: PASS (247/247)
- `scripts/architecture/check_dependency_firewall.py`: PASS (1 waived)
- `scripts/architecture/check_legacy_paths.sh`: PASS
