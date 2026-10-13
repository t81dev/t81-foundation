# Spec Drift Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Spec Drift Policy](#spec-drift-policy)
  - [1. Definition of Drift](#1-definition-of-drift)
  - [2. Detection Mechanism](#2-detection-mechanism)
  - [3. Remediation Policy](#3-remediation-policy)
    - [A. If Implementation is Ahead (Orphaned Code)](#a-if-implementation-is-ahead-orphaned-code)
    - [B. If Spec is Ahead (Ghost Spec)](#b-if-spec-is-ahead-ghost-spec)
    - [C. If Conflict (Semantic Mismatch)](#c-if-conflict-semantic-mismatch)
  - [4. Acceptable Drift](#4-acceptable-drift)
  - [5. Freeze Implications](#5-freeze-implications)

<!-- T81-TOC:END -->


**Status:** Active
**Enforcement:** Soft-Fail (Scanner available, CI blocked on Critical Drift)

## 1. Definition of Drift

Spec Drift occurs when the implementation diverges from the authoritative specification. This compromises the "Institutional Grade" guarantee of the system.

Drift is classified into three categories:

1.  **Orphaned Implementation:** Public API surfaces (headers, opcodes) exist in code but are not documented in `/spec`.
2.  **Ghost Spec:** The specification defines behavior or symbols that do not exist in the implementation.
3.  **Semantic Mismatch:** The behavior of the code contradicts the normative language of the spec (e.g., fault conditions, binary formats).

## 2. Detection Mechanism

The project uses `scripts/governance/spec_impl_drift_check.py` to scan for:
*   Unreferenced public headers.
*   Opcode enum vs spec mismatches.
*   Missing implementation directories for spec files.

## 3. Remediation Policy

When drift is detected, the following remediation steps are mandatory:

### A. If Implementation is Ahead (Orphaned Code)
*   **Action:** Update the relevant spec file to include the new feature.
*   **Deadline:** Before the feature is released in a stable version.
*   **Freeze Implication:** If the surface is frozen, this requires a Spec Update RFC.

### B. If Spec is Ahead (Ghost Spec)
*   **Action:** Mark the spec section as "Planned" or "Experimental", or implement the missing feature.
*   **Constraint:** Stable specs MUST NOT reference nonexistent features as normative requirements.

### C. If Conflict (Semantic Mismatch)
*   **Authority:** The `/spec` is the **Source of Truth**.
*   **Action:** Fix the implementation to match the spec.
*   **Exception:** If the spec is demonstrably wrong (e.g., physically impossible to implement), follow the **Break Procedure** in `FREEZE_ENFORCEMENT.md` to update the spec.

## 4. Acceptable Drift

Drift is permissible **only** under these conditions:

1.  **Experimental Features:** Code in `src/experimental/` or headers marked `[[experimental]]` may lead the spec.
2.  **Internal Implementation Details:** Helper classes/functions not part of the public API do not need spec coverage.
3.  **Draft Specs:** Specs marked "Status: Draft" may describe future states not yet implemented.

## 5. Freeze Implications

*   **Verified Surfaces:** Any drift in a verified surface (e.g., TISC ISA, Data Types) is a **Critical Defect**.
*   **CI Gate:** Future integration will block PRs that introduce new drift in Frozen surfaces.
*   **Release Gate:** No Major/Minor release can occur with Critical Drift in Verified surfaces.
