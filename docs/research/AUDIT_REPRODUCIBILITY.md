# T81 Foundation Architecture — Audit Reproducibility Record

## Executive Overview

This document records the exact environment, pinned repository revision, commands, tool versions, and test execution outcomes for the T81 Foundation RFC Capability and Evidence Audit.

---

## 1. Audit Environment & Revision Pinning

- **Repository**: `t81dev/t81-foundation`
- **Git Commit Hash**: `c930b8c2ee7478782bf3c6d231bdd59984595ef2`
- **Git Branch**: `jules-3950160242379340213-da40b81d`
- **Architecture & OS**: Linux x86_64
- **C++ Compiler**: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`
- **CMake Version**: `3.28.3`
- **CTest Version**: `3.28.3`
- **Python Version**: `Python 3.12.13`
- **Build Mode**: `-DT81_BUILD_ARCHITECTURE_CHALLENGE=ON -DT81_BUILD_TESTS=ON`

---

## 2. Verification Commands & Execution Protocols

To reproduce this audit and verify all evidence packets, execute the following commands from the repository root:

### 2.1 Toolchain Build Protocol
```bash
# Configure CMake with Architecture Challenge flags enabled
cmake -B build -S . -DT81_BUILD_ARCHITECTURE_CHALLENGE=ON -DT81_BUILD_TESTS=ON

# Compile full target suite
cmake --build build -j16
```

### 2.2 Governance & Integrity Verification Checks
```bash
# 1. Verify TISC v1.1.0 ISA Freeze Integrity (209 opcodes)
python3 scripts/ci/check_tisc_freeze_integrity.py

# 2. Verify Root Structure Whitelist
python3 scripts/governance/check_root_structure.py

# 3. Verify Determinism Surface Claim Registry Alignment
python3 scripts/governance/check_determinism_claims.py

# 4. Verify Public C++ API SemVer Lockfile
python3 scripts/governance/check_public_api_semver.py
```

### 2.3 Automated Test Suite Protocol
```bash
# Run full CTest suite in parallel
ctest --test-dir build -j 8 --output-on-failure
```

### 2.4 Independent Scientific Oracle Verification Scripts
```bash
# 1. Verify Checked Floating-Point Conversion Reference Contract
python3 experiments/checked_float_contract/test_reference.py

# 2. Verify Exact Rational Conversion Oracle
python3 experiments/float_conversion_audit/test_oracle.py

# 3. Verify Durable Witness Security Attack Scenarios
PYTHONPATH=experiments/durable_witness python3 experiments/durable_witness/test_durable_witness.py
PYTHONPATH=experiments/durable_witness python3 experiments/durable_witness/attacks.py
```

---

## 3. Direct Test Execution Results

During this audit assignment, all automated test protocols were executed directly in the sandbox environment:

| Test Protocol / Suite | Total Executed | Passed | Failed | Status |
| :--- | :--- | :--- | :--- | :--- |
| **TISC Freeze Integrity Check** | 209 Opcodes | 209 | 0 | **PASSED** |
| **CTest C++ Executable Suite** | 408 Tests | 408 | 0 | **PASSED** (100% Pass) |
| **Spec Conformance Suite** | 33 Tests | 33 | 0 | **PASSED** |
| **Checked Float Contract Oracle** | Reference Suite | All | 0 | **PASSED** |
| **Float Conversion Audit Oracle** | Exact Oracle | All | 0 | **PASSED** |
| **Durable Witness Attack Suite** | Security Attacks | All | 0 | **PASSED** |
| **CLI Contract Suite (`t81_cli_contract_test_runner`)** | E2E CLI Matrix | All | 0 | **PASSED** |

---

## 4. Audit Boundaries & Constraints

The findings recorded in this research packet adhered strictly to the following assignment constraints:
1. **No ISA Changes**: TISC v1.1.0 opcode registry remains frozen at 209 opcodes (0-215 range with reserved holes).
2. **No T81Lang Semantic Changes**: Grammar, lexer, parser, and lowering semantics were unmodifed.
3. **No Refactoring**: Runtime execution and VM code paths were unmodifed.
4. **No RFC Alterations**: Existing accepted RFCs under `/spec/rfcs/` were preserved without edits.
5. **Separation of Evidence Levels**: Observed source facts are explicitly distinguished from prose documentation claims.

---

## 5. Artifact Deliverables Inventory

This audit produced a 5-deliverable research packet under `docs/research/`:

1. `/docs/research/CAPABILITY_BASELINE.md`: Executive inventory across 12 major subsystems with explicit evidence levels.
2. `/docs/research/RFC_EVIDENCE_MATRIX.csv`: Machine-readable mapping of all 113 RFCs to catalog status, frontmatter status, capability IDs, and source/test paths.
3. `/docs/research/TISC_T81LANG_COVERAGE.md`: Opcode-by-opcode TISC coverage, VM dispatch locations, T81Lang construct coverage, and compiler reachability gaps.
4. `/docs/research/RESEARCH_GAPS.md`: Systematic breakdown of Implementation, Verification, Measurement, and Architectural gaps and prior Scientific Investigations 01-08.
5. `/docs/research/AUDIT_REPRODUCIBILITY.md`: This document.
