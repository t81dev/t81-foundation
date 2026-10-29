# Governance Authority Decision Matrix & Reconciliation Report

**Document ID:** `GOVERNANCE_AUTHORITY_DECISION_MATRIX.md`
**Author:** Jules (Independent Evidence Reviewer)
**Repository:** `t81dev/t81-foundation`
**Date:** October 2026
**Status:** Read-only evidence review and maintainer decision preparation

---

## Executive Summary

Codex independently reproduced the original governance validation results and prepared a local integration candidate (`37dfb1bd708758d00d3b1c7206412839715f33f0`) that increased governance validator passes from 16/27 (on current `main`) to 23/27. The remaining 4/27 failing validators are blocked by governance authority decisions rather than implementation defects alone.

This report establishes the authoritative evidence, root causes, available options, recommended decisions, affected files, and verification criteria for resolving those four governance blockers without weakening existing contracts or CI safety guarantees.

---

## Blockers Decision Matrix

### Block 1: Public API Version and Lock

#### 1. Current Observed State
- `include/t81/config.hpp` declares `T81_VERSION_MAJOR 1`, `T81_VERSION_MINOR 9`, `T81_VERSION_PATCH 1` (`1.9.1`).
- `CMakeLists.txt` sets `T81_VERSION_MAJOR 1`, `T81_VERSION_MINOR 9`, `T81_VERSION_PATCH 1` (`1.9.1`).
- `docs/governance/PUBLIC_API_LOCK.json` records `"version": "1.9.0"` and `"surface_digest_sha256": "fa406791328c8ce64c03f25aa794b20b069d0b68a1755213585f90aee2d02c93"`.
- The current SHA-256 surface digest computed over `include/t81/**/*.hpp` is `0bddb4b28083f7d564d72c2133ff968f34f77c8641af45def224adb1398295a8`.
- `python3 scripts/governance/check_public_api_semver.py` fails with version mismatch (`config=1.9.1, lock=1.9.0`) and digest mismatch.

#### 2. Historical Evidence
- On 2026-03-21, version 1.9.1 was introduced to record the completion of RFC-0034 native dispatch hotpath SIMD optimizations in C++ headers (`include/t81/isa/opcodes.hpp`, `include/t81/tensor/matmul.hpp`, `include/t81/simd/simd.hpp`), as documented in `docs/status/SYSTEM_STATUS.md` and `docs/records/status-history/RFC_0034_HOTPATH_SIMD_EVIDENCE_2026-03-21.md`.
- The version macro was updated in `include/t81/config.hpp` and `CMakeLists.txt`, but `docs/governance/PUBLIC_API_LOCK.json` was left at 1.9.0 without running `--write-lock`.

#### 3. Authoritative Governing Requirement
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` and `scripts/governance/check_public_api_semver.py` dictate that any public API header change or version increment requires explicit maintainer authorization before updating `PUBLIC_API_LOCK.json`.
- Semantic versioning rules mandate a patch bump (`1.9.0` -> `1.9.1`) for backward-compatible internal hotpath optimizations and non-breaking header additions.

#### 4. Root Cause
- The RFC-0034 SIMD optimization pass updated `include/t81/config.hpp` to version 1.9.1 and added SIMD helper routines in public headers, but omitted the required lock file update (`check_public_api_semver.py --write-lock`).

#### 5. Available Resolution Options
- **Option A.1 (Recommended):** Obtain maintainer authorization to update `docs/governance/PUBLIC_API_LOCK.json` to version `1.9.1` and surface digest `0bddb4b28083f7d564d72c2133ff968f34f77c8641af45def224adb1398295a8` via `--write-lock`.
- **Option A.2:** Revert `include/t81/config.hpp` and `CMakeLists.txt` to `1.9.0` and revert header additions to restore digest `fa406...`. (*Rejected:* Reverts accepted RFC-0034 SIMD performance gains).
- **Option A.3:** Bump version to `1.10.0` or `2.0.0`. (*Rejected:* Inspection confirms header changes were backwards-compatible SIMD additions; a patch bump `1.9.1` is correct under SemVer).

#### 6. Recommended Resolution
Option A.1: Maintainer explicitly approves updating `docs/governance/PUBLIC_API_LOCK.json` to version `1.9.1` and digest `0bddb4b28083f7d564d72c2133ff968f34f77c8641af45def224adb1398295a8`.

#### 7. Risks and Compatibility Consequences
- Re-locking establishes an authoritative baseline for version 1.9.1. All public header changes are backwards-compatible additions; no API breaking changes exist.

#### 8. Exact Maintainer Decision Required
- Authorize updating `docs/governance/PUBLIC_API_LOCK.json` to version `1.9.1` and SHA-256 digest `0bddb4b28083f7d564d72c2133ff968f34f77c8641af45def224adb1398295a8`.

#### 9. Files That Would Change After Approval
- `docs/governance/PUBLIC_API_LOCK.json`

#### 10. Tests That Must Pass Afterward
- `python3 scripts/governance/check_public_api_semver.py`

---

### Block 2: Component Maturity Labels

#### 1. Current Observed State
- `python3 scripts/governance/check_status_label_coherence.py` fails with:
  - `README maturity mismatch for t81lang: expected=stable got=parsing, intermediate representation...`
  - `README maturity mismatch for t81vm: expected=stable got=interpreter for tisc programs...`
  - `README maturity mismatch for axion: expected=stable got=policy evaluation and execution authorization`
- `docs/governance/MATURITY_CANONICAL_STATUS.json` defines maturity as `"Stable"` for `t81lang`, `t81vm`, and `axion`.
- `docs/status/IMPLEMENTATION_MATRIX.md` and `docs/status/SYSTEM_STATUS.md` report `t81lang`, `t81vm`, and `axion` as `"Stable"`.

#### 2. Historical Evidence
- On March 15–16, 2026, `T81VM`, `Axion`, and `T81Lang` completed formal stable promotion reviews (`T81VM_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`, `AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`, `T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md`).
- `MATURITY_CANONICAL_STATUS.json`, `IMPLEMENTATION_MATRIX.md`, and `SYSTEM_STATUS.md` were correctly updated to reflect `Stable` maturity for all three components.
- However, `README.md` §3 ("Implemented Research Components") formatted its table with columns `Component | Role | Representative evidence | Important limit`. Column 1 (`row[1]`) contained `Role` text rather than a `Maturity` label.

#### 3. Authoritative Governing Requirement
- `MATURITY_CANONICAL_STATUS.json` is the canonical authority for component maturity classifications (`Stable` for T81VM, T81Lang, Axion).
- `check_status_label_coherence.py` enforces cross-document coherence across `MATURITY_CANONICAL_STATUS.json`, `IMPLEMENTATION_MATRIX.md`, `SYSTEM_STATUS.md`, and `README.md`.

#### 4. Root Cause
- Schema and parser defect: `check_status_label_coherence.py` assumes table column index 1 (`row[1]`) in `README.md` contains the component's maturity label. `README.md` §3 changed its table column 1 from `Maturity` to `Role`, causing the script to read role descriptions as maturity labels and compare them against `"stable"`.

#### 5. Available Resolution Options
- **Option B.1 (Recommended):** Add an explicit `Maturity` column (`Stable`) to `README.md` §3 component table, and update `check_status_label_coherence.py` to parse table columns by header name rather than hardcoded column index 1.
- **Option B.2:** Demote component maturity in `MATURITY_CANONICAL_STATUS.json`, `IMPLEMENTATION_MATRIX.md`, and `SYSTEM_STATUS.md`. (*Rejected:* Violates governance rule against demoting components with valid promotion evidence).

#### 6. Recommended Resolution
Option B.1: Maintainer approves adding an explicit `Maturity` column (`Stable`) to `README.md` §3 component table for `T81VM`, `T81Lang`, and `Axion`, and updating `check_status_label_coherence.py` to parse the `Maturity` column header dynamically.

#### 7. Risks and Compatibility Consequences
- Restores cross-document alignment without modifying component maturity levels or making unbacked stability claims.

#### 8. Exact Maintainer Decision Required
- Authorize adding the `Maturity` column (`Stable`) to `README.md` §3 component table and updating `check_status_label_coherence.py` column parsing logic.

#### 9. Files That Would Change After Approval
- `README.md`
- `scripts/governance/check_status_label_coherence.py`

#### 10. Tests That Must Pass Afterward
- `python3 scripts/governance/check_status_label_coherence.py`

---

### Block 3: RFC Lifecycle (RFC-0055 and RFC-0056)

#### 1. Current Observed State
- `python3 scripts/governance/check_rfc_lifecycle_hygiene.py` reports:
  - `RFC-0055-hardware-target-profile-template.md: status mismatch index=accepted file=draft`
  - `RFC-0056-google-axion-hardware-profile.md: status mismatch index=accepted file=draft`
- `spec/rfcs/index.md` lists `RFC-0055` as `accepted` and `RFC-0056` as `accepted`.
- Main contract files `spec/rfcs/RFC-0055-native-ternary-hardware-target-and-interop-contract.md` and `spec/rfcs/RFC-0056-google-axion-t81-integration.md` BOTH record `**Status:** accepted`.

#### 2. Historical Evidence
- RFC-0055 (Native Ternary Hardware Target and Interop Contract) was accepted with deliverables including the main contract spec and a companion target profile template (`RFC-0055-hardware-target-profile-template.md`).
- RFC-0056 (Google Axion-T81 Integration Contract) was accepted on 2026-03-22 with deliverables including the main contract spec and a companion hardware profile (`RFC-0056-google-axion-hardware-profile.md`).
- The companion profile/template files retain `Status: draft` as instance profile templates created during RFC specification.

#### 3. Authoritative Governing Requirement
- `spec/rfcs/index.md` and the primary contract specs (`RFC-0055-native-ternary...md` and `RFC-0056-google-axion-t81-integration.md`) are the authoritative records for RFC status (**Accepted**).
- `check_rfc_lifecycle_hygiene.py` must validate the status of primary RFC contract specifications against `index.md`.

#### 4. Root Cause
- Script file-matching bug in `check_rfc_lifecycle_hygiene.py`: when searching for `f"{rfc_id}-*.md"`, the script selects `matches[0]`. Because `"hardware"` and `"google-axion-h..."` sort alphabetically before `"native"` and `"google-axion-t..."`, the script erroneously selected companion template/profile files (`Status: draft`) instead of the primary RFC contract documents (`Status: accepted`).

#### 5. Available Resolution Options
- **Option C.1 (Recommended):** Update file-selection logic in `check_rfc_lifecycle_hygiene.py` to target primary RFC contract specifications by filtering out companion `-template.md` and `-profile.md` files.
- **Option C.2:** Change `Status:` in template/profile files to `accepted`. (*Rejected:* Templates/profiles are instance forms, not normative RFC specs).
- **Option C.3:** Demote RFC-0055 and RFC-0056 to `draft` in `index.md`. (*Rejected:* Contradicts accepted governance decisions).

#### 6. Recommended Resolution
Option C.1: Maintainer approves updating `check_rfc_lifecycle_hygiene.py` file-matching logic to target primary RFC contract documents.

#### 7. Risks and Compatibility Consequences
- Eliminates false-positive validator failures without changing RFC statuses or governance records.

#### 8. Exact Maintainer Decision Required
- Authorize updating `check_rfc_lifecycle_hygiene.py` to filter companion profile/template files and match primary RFC specifications.

#### 9. Files That Would Change After Approval
- `scripts/governance/check_rfc_lifecycle_hygiene.py`

#### 10. Tests That Must Pass Afterward
- `python3 scripts/governance/check_rfc_lifecycle_hygiene.py`

---

### Block 4: Aggregate Documentation Requirements

#### 1. Current Observed State
- `scripts/governance/check_docs_governance_hygiene.py` fails reporting 9 missing required README files:
  1. `core/README.md`
  2. `runtime/README.md`
  3. `runtime/jit/README.md`
  4. `internal/README.md`
  5. `internal/axion/README.md`
  6. `internal/tooling/README.md`
  7. `tooling/README.md`
  8. `examples/consumer_cmake/README.md`
  9. `examples/system-integration/README.md`
- In addition, `scripts/governance/check_docs_structure.py` fails reporting 5 missing `book/book-*` directories.

#### 2. Historical Evidence
- During repository refactoring (February–March 2026), subdirectories were reorganized and consolidated:
  - `core/` -> `include/t81/` and `src/core/`
  - `runtime/` and `runtime/jit/` -> `vm/` and `src/vm/`
  - `internal/` and `internal/axion/` -> `kernel/axion/` and `src/`
  - `tooling/` and `internal/tooling/` -> `tools/`
  - `examples/consumer_cmake/` -> `examples/compiler-and-ffi/consumer_cmake/`
  - `examples/system-integration/` -> `examples/systems-and-agents/system-integration/`
  - `book/` -> `docs/book/`
- Equivalent, active README documentation survives at all relocated locations (`tools/README.md`, `vm/README.md`, `kernel/README.md`, `examples/compiler-and-ffi/consumer_cmake/README.md`, `examples/systems-and-agents/system-integration/README.md`, `docs/book/book-*`).

#### 3. Authoritative Governing Requirement
- Governance hygiene policy requires active README coverage for all top-level and major subsystem directories.
- Retiring obsolete expected paths in `REQUIRED_READMES` requires explicit maintainer authorization to reflect historical directory consolidations.

#### 4. Root Cause
- `REQUIRED_READMES` in `check_docs_governance_hygiene.py` contains 9 obsolete paths that were relocated during refactoring, and `check_docs_structure.py` retains outdated relative paths (`book/` instead of `docs/book/`).

#### 5. Available Resolution Options
- **Option D.1 (Recommended):** Maintainer approves updating `REQUIRED_READMES` in `check_docs_governance_hygiene.py` to point to current valid directory locations (`examples/compiler-and-ffi/consumer_cmake/README.md`, `examples/systems-and-agents/system-integration/README.md`, `include/t81/README.md`, `vm/README.md`, `tools/README.md`) and retiring deleted/consolidated legacy paths (`core/`, `runtime/`, `internal/`, `tooling/`). Also update `check_docs_structure.py` `BOOK_DIRS` to `docs/book/book-*`.
- **Option D.2:** Re-create empty stub directories and README files at the 9 deleted legacy paths. (*Rejected:* Introduces dead directory clutter at repository root).

#### 6. Recommended Resolution
Option D.1: Maintainer approves updating `check_docs_governance_hygiene.py`, `check_docs_structure.py`, and `check_spec_code_alignment_baseline.py` path registries to reflect current canonical directory locations.

#### 7. Risks and Compatibility Consequences
- Removes stale path expectations and aligns governance validators with actual repository structure without losing documentation coverage.

#### 8. Exact Maintainer Decision Required
- Authorize updating `REQUIRED_READMES` in `check_docs_governance_hygiene.py`, `BOOK_DIRS` in `check_docs_structure.py`, and mapped source paths in `check_spec_code_alignment_baseline.py` to match current canonical locations.

#### 9. Files That Would Change After Approval
- `scripts/governance/check_docs_governance_hygiene.py`
- `scripts/governance/check_docs_structure.py`
- `scripts/governance/check_spec_code_alignment_baseline.py`
- `scripts/governance/check_translation_metadata.py`

#### 10. Tests That Must Pass Afterward
- `python3 scripts/governance/check_docs_governance_hygiene.py`
- `python3 scripts/governance/check_docs_structure.py`
- `python3 scripts/governance/check_spec_code_alignment_baseline.py`
- `python3 scripts/governance/check_translation_metadata.py`

---

## Single Coordinated Correction Plan for Codex

Upon receiving explicit maintainer approval for decisions A.1, B.1, C.1, and D.1, Codex should apply the following single coordinated patch:

1. **Public API Lock Update:**
   - Run `python3 scripts/governance/check_public_api_semver.py --write-lock` to write `docs/governance/PUBLIC_API_LOCK.json` with version `1.9.1` and digest `0bddb4b28083f7d564d72c2133ff968f34f77c8641af45def224adb1398295a8`.

2. **README Component Maturity Alignment:**
   - Update `README.md` §3 component table to include a `Maturity` column (`Stable` for T81VM, T81Lang, Axion).
   - Update `scripts/governance/check_status_label_coherence.py` to parse table columns by header name (`Maturity`).

3. **RFC Lifecycle Validator Correction:**
   - Update `scripts/governance/check_rfc_lifecycle_hygiene.py` file-matching logic to ignore companion `-template.md` and `-profile.md` files, selecting primary RFC specifications (`RFC-0055-native-ternary...md` and `RFC-0056-google-axion-t81-integration.md`).

4. **Directory Structure & Required README Registry Update:**
   - Update `REQUIRED_READMES` in `scripts/governance/check_docs_governance_hygiene.py` to reference active paths (`examples/compiler-and-ffi/consumer_cmake/README.md`, `examples/systems-and-agents/system-integration/README.md`, `include/t81/README.md`, `vm/README.md`, `tools/README.md`) and remove deleted legacy paths.
   - Update `BOOK_DIRS` in `scripts/governance/check_docs_structure.py` to `docs/book/book-*`.
   - Update `check_spec_code_alignment_baseline.py` mapped paths (`vm/vm.cpp` and `isa/encoding.cpp`).
   - Update `check_translation_metadata.py` language link tokens if needed.

5. **Validation Replay:**
   - Run all 27 governance validators (`for f in scripts/governance/*.py scripts/ci/*.py; do python3 "$f"; done`).
   - Confirm 27/27 passing status.
