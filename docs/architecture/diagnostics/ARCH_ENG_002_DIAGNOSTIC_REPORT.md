# T81-ARCH-ENG-002 Phase 1: Architecture-Guided Engineering Diagnostics Report

**Document ID:** `T81-ARCH-ENG-002-DIAGNOSTIC-REPORT-001`
**Authoritative Baseline:** `9a3e307ad21135749d35a4f1a6a0d5820d0cc698` (`origin/main`)
**Timestamp:** 2026-11-10
**Status:** READY FOR INDEPENDENT REVIEW

---

## 1. Executive Summary

This report establishes Phase 1 of the **Architecture-Guided Engineering Diagnostics System** (T81-ARCH-ENG-002), extending the visualization datasets introduced in T81-ARCH-VIZ-001 and T81-ARCH-VIZ-002 into an automated diagnostic and drift detection foundation.

The objectives achieved in this phase include:
1. **Deterministic Architecture Drift Detection:** Implemented automated drift analysis in `scripts/ci/validate_architecture_visualization.py` verifying source file existence, line bounds, reference type compliance, and referential integrity against the pinned repository baseline (`9a3e307ad21135749d35a4f1a6a0d5820d0cc698`).
2. **Machine-Readable Component-to-Test Evidence Index:** Created `docs/architecture/visualization/component_evidence_index.json` covering all 35 architectural nodes, mapping components to concrete test files, CMake targets, and documented continuity receipts with four mutually exclusive relationship classifications: `VERIFIED`, `STATICALLY_SUPPORTED`, `UNVERIFIED`, and `NOT_MAPPED`.
3. **QEMU Case Study Audit:** Conducted an in-depth empirical audit of the TernaryOS QEMU verification harness scripts (`drivers/qemu/scripts/verify_production.sh`, `drivers/qemu/scripts/automated_verification.sh`, `ternaryos/production_qemu_setup.sh`, and `ternaryos/automated_verification.sh`), documenting critical defects including the `((COMPLETED_TESTS++))` arithmetic trap under `set -e`, directory mismatches, missing script dependencies, and invalid verification assertions. A corrective blueprint is provided for separate implementation.

---

## 2. Architecture Drift Detection Audit

Drift detection was executed across all components of the architecture dataset:

| Category | Checked Entities | Detected Anomalies | Disposition |
| :--- | :--- | :--- | :--- |
| **Layers** | 7 Layers | 0 | PASSED |
| **Nodes** | 35 Nodes (71 source paths) | 0 missing paths | PASSED |
| **Edges** | 32 Edges (source references & bounds) | 0 invalid bounds, 0 unknown types | PASSED |
| **Interfaces** | 6 Interfaces (provider/consumer nodes) | 0 dangling references | PASSED |
| **Execution Paths** | 9 Pathways (33 steps) | 0 stale lines, 0 missing files | PASSED |
| **Architecture Gaps** | 4 Gaps | 0 stale references | PASSED |
| **Coverage Manifest** | 35 Top-Level Dirs (18 mapped, 17 unmapped) | 0 census mismatches | PASSED |
| **Evidence Index** | 35 Components mapped | 0 schema or reference errors | PASSED |

**Total Drift Anomalies Detected:** `0`. All references resolve deterministically against the pinned tree.

---

## 3. Component Evidence Index Distribution

From `docs/architecture/visualization/component_evidence_index.json`, the 35 architectural components are partitioned strictly into four mutually exclusive evidence relationship categories:

```
┌─────────────────────────────────────────────────────────────┐
│ Component Evidence Classification (Total: 35 Components)    │
├───────────────────────────────┬──────────────┬──────────────┤
│ Classification                │ Count        │ Percentage   │
├───────────────────────────────┼──────────────┼──────────────┤
│ VERIFIED                      │ 9            │ 25.71%       │
│ STATICALLY_SUPPORTED          │ 22           │ 62.86%       │
│ UNVERIFIED                    │ 3            │ 8.57%        │
│ NOT_MAPPED                    │ 1            │ 2.86%        │
└───────────────────────────────┴──────────────┴──────────────┘
```

### Classification Criteria & Governance Rules:
1. **`VERIFIED` (9 Components):**
   - Components certified by formal, independent mathematical oracles and signed continuity receipts.
   - *Components:*
     - `sub.layer.2_exec.freestanding_runtime` (CPA-004 cross-profile conformance receipt)
     - `sub.layer.3_num.balanced_ternary_limbs` (SCG-07 Phases 3, 4, 5, 6 qualification receipts)
     - `sub.layer.3_num.fixed_width_integers` (SCG-03 and SCG-07 Phase 6 receipts)
     - `sub.layer.3_num.multi_precision_bigint` (SCG-07 Phase 2 and Phase 6 receipts)
     - `sub.layer.3_num.rational_fractions` (SCG-07 Phase 1 and Phase 6 receipts)
     - `sub.layer.5_os.ternaryos_kernel` (CPA-004 freestanding qualification receipt)
     - `mod.ternaryos.scheduler` (CPA-004 timer schedule qualification receipt)
     - `mod.ternaryos.mmu` (CPA-004 TTBR0 page-table qualification receipt)
     - `sub.layer.6_hw.qemu_drivers` (CPA-004 PL011 and GICv3 qualification receipt)

2. **`STATICALLY_SUPPORTED` (22 Components):**
   - Implemented production code supported by compiled executables and dedicated unit/integration test suites in `tests/cpp/` or `ternaryos/tests/`, but lacking an independent qualification receipt.
   - *Components:* Frontend (`sub.layer.1_lang.frontend`, lexer, parser, semantic analyzer, IR generator), CLI (`sub.layer.1_lang.cli`, weights tool), Execution Core (`sub.layer.2_exec.tisc_isa`, `t81vm`, interpreter core, system registers), Numeric & Service subsystems (`ternary_tensors`, `soft_math`, `axion_kernel`, `canonfs`, `codecs`, `crypto`, `simd_primitives`), and Assurance modules (`hypothesis_register`, `scg_continuity`, `cpp_test_suite`, `ci_governance`).

3. **`UNVERIFIED` (3 Components):**
   - Experimental prototypes, bridges, or hardware prototypes with smoke-only or simulation tests, lacking dedicated verification suites.
   - *Components:* `sub.layer.1_lang.toolchain_bridges` (smoke tests only), `sub.layer.2_exec.jit_engine` (smoke-level policy tests), `sub.layer.6_hw.rtl_feasibility` (Verilog simulation testbench only, no hardware measurements).

4. **`NOT_MAPPED` (1 Component):**
   - Unimplemented RFCs or conceptual proposals with zero code or tests in the tree.
   - *Component:* `sub.layer.5_os.daios_proposal` (RFC-00D2 design document only).

---

## 4. QEMU Verification Harness Case Study

As mandated by T81-ARCH-ENG-002, the TernaryOS QEMU verification harness was subjected to a systematic engineering diagnostic audit. Four primary files were examined:
- `drivers/qemu/scripts/verify_production.sh`
- `drivers/qemu/scripts/automated_verification.sh`
- `ternaryos/production_qemu_setup.sh`
- `ternaryos/automated_verification.sh`

### 4.1. Defect Catalog

#### Defect 1: Arithmetic Evaluation Failure Under `set -e` (`((COMPLETED_TESTS++))`)
- **Location:** `drivers/qemu/scripts/automated_verification.sh:297-301` and `ternaryos/automated_verification.sh:253-257`.
- **Code:**
  ```bash
  TOTAL_TESTS=5
  COMPLETED_TESTS=0

  [ -f "$OUTPUT_DIR/monitor_test.log" ] && ((COMPLETED_TESTS++))
  [ -f "$OUTPUT_DIR/memory_analysis.log" ] && ((COMPLETED_TESTS++))
  [ -f "$OUTPUT_DIR/boot_debug.log" ] && ((COMPLETED_TESTS++))
  [ -f "$OUTPUT_DIR/hardware_test.log" ] && ((COMPLETED_TESTS++))
  [ -f "$OUTPUT_DIR/integration_monitor.log" ] && ((COMPLETED_TESTS++))
  ```
- **Root Cause:** In Bash, `((expr))` evaluates the arithmetic expression and returns exit status `1` (failure) if the evaluated expression is `0`. With `COMPLETED_TESTS=0`, post-increment `((COMPLETED_TESTS++))` evaluates to `0` before incrementing. Under `set -e` (and `set -euo pipefail`), Bash treats exit status `1` as an unhandled error and immediately terminates the script. As a result, the script crashes on the very first log file check!
- **Demonstration:**
  ```bash
  bash -c 'set -e; TOTAL=5; COMPLETED=0; touch /tmp/exists; [ -f /tmp/exists ] && ((COMPLETED++)); echo "done"'
  # Result: Exited with code 1, does NOT reach "done"
  ```

#### Defect 2: Inadequate Verification Assertions (Equating Log Existence with Verification Success)
- **Location:** `drivers/qemu/scripts/automated_verification.sh:260-311`
- **Issue:** The script assesses test completion solely by checking whether the log file was created (`[ -f "$OUTPUT_DIR/test.log" ]`), rather than checking the exit code of QEMU or verifying explicit assertions.
- **Consequence:** If QEMU crashes, hangs, or encounters fatal boot errors, the log file is still created on disk (even if empty or containing error traces), causing the harness to mark the test as `✅ COMPLETED` and increment `COMPLETED_TESTS`.

#### Defect 3: Mismatched Output Directories Between Calling and Execution Scripts
- **Location:** `drivers/qemu/scripts/verify_production.sh:9, 26` vs `drivers/qemu/scripts/automated_verification.sh:8`
- **Issue:**
  - `verify_production.sh` defines:
    ```bash
    OUTPUT_DIR="${OUTPUT_DIR:-${TMPDIR:-/tmp}/ternaryos_verification}"
    ```
    and later checks for results at:
    ```bash
    if grep -q "VERIFICATION SUCCESSFUL" "$OUTPUT_DIR/verification_summary.txt"; then
    ```
  - However, `automated_verification.sh` sets:
    ```bash
    OUTPUT_DIR="${OUTPUT_DIR:-$(mktemp -d "${TMPDIR:-/tmp}/ternaryos_verification.XXXXXX")}"
    ```
  - Because `verify_production.sh` does *not* export `OUTPUT_DIR` (`export OUTPUT_DIR`), `automated_verification.sh` runs with `OUTPUT_DIR` unset, generates a random temporary directory (e.g. `/tmp/ternaryos_verification.Ab3X9z`), and writes `verification_summary.txt` there.
  - `verify_production.sh` then looks in `/tmp/ternaryos_verification/verification_summary.txt`, fails to find it (or finds stale artifacts from a prior manual run), and reports `❌ Production verification FAILED`.

#### Defect 4: Invalid Working-Directory and Relative Path Assumptions
- **Location:** `drivers/qemu/scripts/automated_verification.sh:85, 106` and `ternaryos/production_qemu_setup.sh:145, 342`
- **Issue:**
  - `automated_verification.sh` invokes:
    ```bash
    ./experimental/ternaryos/qemu_monitor_test.sh > "$OUTPUT_DIR/monitor_test.log" 2>&1
    ./experimental/ternaryos/memory_analyzer.sh ...
    ```
  - The path `experimental/ternaryos/` does *not exist* in the repository. The scripts were moved or historically placed at `drivers/qemu/scripts/` (e.g., `drivers/qemu/scripts/qemu_monitor_test.sh` and `drivers/qemu/scripts/memory_analyzer.sh`).
  - When executed, these commands immediately fail with `No such file or directory`.

#### Defect 5: Host-Specific Hardcoded Path Dependencies
- **Location:** `ternaryos/automated_verification.sh:9` and `ternaryos/production_qemu_setup.sh:176, 203`
- **Issue:**
  - Hardcoded macOS Homebrew firmware path: `BIOS="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"`
  - Hardcoded absolute paths: `PRODUCTION_DIR="/opt/ternaryos-qemu"`
  - Linux environments require `/usr/share/AAVMF/AAVMF_CODE.fd` or `/usr/share/qemu/edk2-aarch64-code.fd`.

---

## 5. Corrective Recommendation Blueprint (For Follow-On Repair PR)

In accordance with T81-ARCH-ENG-002 constraints, no production or runtime harness changes are made in this diagnostic PR. The following corrective blueprint is provided for the dedicated repair task:

1. **Arithmetic Increment Repair:**
   Replace the arithmetic post-increment idiom:
   ```bash
   # BROKEN:
   [ -f "$file" ] && ((COMPLETED_TESTS++))

   # CORRECT:
   if [ -f "$file" ]; then
       COMPLETED_TESTS=$((COMPLETED_TESTS + 1))
   fi
   ```
2. **Explicit Verification Assertions:**
   Record test command exit codes and inspect specific success sentinel strings in log files rather than bare file existence:
   ```bash
   if [ -s "$OUTPUT_DIR/boot_debug.log" ] && grep -q "qemu_bare_kernel_entry" "$OUTPUT_DIR/boot_debug.log"; then
       PASS_COUNT=$((PASS_COUNT + 1))
   fi
   ```
3. **Environment & Directory Unification:**
   In `verify_production.sh`, properly export `OUTPUT_DIR`:
   ```bash
   export OUTPUT_DIR="${OUTPUT_DIR:-${TMPDIR:-/tmp}/ternaryos_verification}"
   mkdir -p "$OUTPUT_DIR"
   ```
   In `automated_verification.sh`, honor the exported `OUTPUT_DIR` without overwriting it with `mktemp -d` when already provided.
4. **Script Path Resolution:**
   Resolve auxiliary scripts dynamically relative to `BASH_SOURCE[0]`:
   ```bash
   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
   "$SCRIPT_DIR/qemu_monitor_test.sh" > "$OUTPUT_DIR/monitor_test.log" 2>&1
   ```
5. **Cross-Platform Firmware Discovery:**
   Retain the multi-candidate firmware search loop from `drivers/qemu/scripts/automated_verification.sh` across all QEMU setup and launch scripts.

---

## 6. Limitations & Governance Boundaries

1. **Qualification Boundary Preservation:**
   - No qualification levels were elevated or altered in this task.
   - `component_evidence_index.json` respects existing formal boundaries: only components with documented, signed continuity receipts (SCG-07, CPA-004) are classified as `VERIFIED`. All other compiled modules remain strictly `STATICALLY_SUPPORTED`, `UNVERIFIED`, or `NOT_MAPPED`.
2. **Read-Only QEMU Harness Audit:**
   - QEMU script repairs are strictly deferred to a dedicated subsequent PR.
3. **Trace and Runtime Invariance:**
   - No production runtime code, compiler logic, or normative specifications were modified.
