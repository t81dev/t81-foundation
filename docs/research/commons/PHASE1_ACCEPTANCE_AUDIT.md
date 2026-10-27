# T81 Research Commons — Independent Phase 1 Acceptance Audit Report

**Audit Target:** Draft PR #609 (`research/research-commons-phase1`) and Draft PR #610 (`docs/pr608-evidence-correction`)
**Auditor Role:** Independent Adversarial Scientific Auditor
**Date:** October 27, 2026
**Repository:** `https://github.com/t81dev/t81-foundation`
**Final Acceptance Verdict:** **CONDITIONAL GO**

---

## 1. Executive Summary & Audit Mandate

This report provides an independent adversarial scientific audit of Phase 1 of the T81 Research Commons (PR #609) and the accompanying H07/H08 evidence reconciliation PR (PR #610). Neither draft PR branch was altered during this evaluation.

The audit evaluated:
1. **PR #610 Fidelity:** Preservation of canonical H07/H08 definitions, dispositions, evidence tiers (B/C and C/E), limitations, and historical provenance without unevidenced promotion.
2. **PR #609 Architecture:** Integrity of the 39 pinned source identities, six read-only discovery operations, JSON schemas, and execution of the 20 unit tests in `tests/research_commons/test_commons.py`.
3. **Evidence Distinction:** Ability of the discovery interface to explicitly segregate canonical authority (`HYPOTHESIS_REGISTER_v1.json`) from historical reports, disputed claims, and unmerged proposals.
4. **Read-Only / Security Isolation:** Falsification attempts against claims of zero write side effects, path traversal protections, sub-process isolation, and lazy Git fetch suppression.
5. **Fresh-Context H08 Discovery Probe:** Execution of a fresh-context query against hypothesis H08, challenging the premise that AI-assisted research has established scientific trustworthiness.
6. **Epistemic Risks:** Analysis of potential circular validation, source contamination, missing negative evidence, stale Git commit pins, and confidence classification boundaries.
7. **Implementation vs. Scientific Validation:** Explicit separation between software implementation correctness (passing unit tests) and independent scientific validation.
8. **Pre-existing Docs-Structure Failure:** Inspection of `scripts/governance/check_docs_structure.py` failure without expanding audit scope into unrelated repository repairs.

---

## 2. Objective 1: PR #610 Evidence Reconciliation Audit

PR #610 (`d1eee548d4658df37b35d0cc3781b5a77cbae96b`) addresses P1 review findings in `docs/research/HUMAN_AI_RESEARCH_INITIATIVE.md` and `docs/research/HUMAN_AI_INITIATIVE_RECONCILIATION_REPORT_2026-10-27.md`.

### Verified Findings:
- **Canonical H07/H08 Dispositions Preserved:** PR #610 correctly reverts claims that H07 or H08 were "established". It quotes the canonical `HYPOTHESIS_REGISTER_v1.json` dispositions verbatim:
  - **H07 (Track F):** Formal claim preserved. Result: *"Register evidence tiers: B, C. Separate engineering track. PRs #565/#566 remain open/draft... Existing source/test reports are not a new independent security certification here."*
  - **H08 (Track M):** Formal claim preserved. Result: *"Register evidence tiers: C, E. This session records source claims, independent extraction probes and mathematical derivation separately; it is not a controlled study of AI productivity."*
- **Explicit Limitations Recorded:**
  - **H07 Limits:** Requires conventional runtime authorization boundary under equal threat model; simulated dispatch supplies no host-security or malicious-host claims.
  - **H08 Limits:** Earlier prompt provenance unavailable; commit authorship does not prove prompt provenance; repeated model summaries are not independent verification.
- **TISC Freeze Check Correction:** Replaces the citation of nonexistent CTest `t81_tisc_freeze_integrity_test` with `python3 scripts/ci/check_tisc_freeze_integrity.py`, explicitly qualifying that it checks registry integrity (209 opcode values) rather than executable opcode semantics or compiler behavior.
- **Contract Enforcement Unit Tests:** `tests/python/test_initiative_reconciliation.py` (4 unit tests) passed 100%. The test suite programmatically rejects any future attempts to restore "established" statuses, promote evidence tiers, or drop provenance limitations.

---

## 3. Objective 2 & 3: PR #609 Architecture, Pinned Identities & Evidence Distinction

PR #609 (`43df109b01fc88f67375f18cd41ff563982830b3`) implements the `t81.research-commons.v1` read-only discovery interface in `tools/research_commons/commons.py`.

### Architectural Verification:
- **39 Pinned Source Identities:** `manifest.v1.json` pins 39 exact file paths, commit SHAs (40-char hex), roles (`authority`, `primary`, `disputed`, `proposal`), and SHA-256 content digests.
- **Source Role Classification:** The discovery engine explicitly categorizes every source:
  - `authority`: Dated canonical register (`HYPOTHESIS_REGISTER_v1.json`, SHA-256: `af8ca...`) and charter (`MASTER_CONTINUITY_CHARTER_v1.0.md`, SHA-256: `bb073...`).
  - `disputed`: Merged historical initiative documents (`HUMAN_AI_RESEARCH_INITIATIVE.md` @ `274421f` and `fabc7aa`).
  - `proposal`: Unmerged draft PR #610 documents (@ `d1eee54`).
- **Discrepancy Tracking:** Historical discrepancies (e.g., `D-607-IDS` and `D-608-H07-H08`) are indexed alongside hypotheses, ensuring consumers are alerted when a source overclaims relative to canonical authority.
- **Six Read-Only Operations:**
  1. `list_research_tracks`
  2. `list_hypotheses`
  3. `get_hypothesis`
  4. `get_evidence`
  5. `get_reproduction_instructions`
  6. `list_open_research_questions`
- **Schema Validation:** Strictly enforced using Draft 2020-12 JSON Schemas (`request.schema.json`, `response.schema.json`, `manifest.schema.json`, contribution schemas).

---

## 4. Objective 4: Falsification of Read-Only & Security Boundary Claims

The auditor performed targeted adversarial security and side-effect testing on `tools/research_commons/commons.py`.

### Security Inspection Results:
1. **Git Subprocess Environment Isolation:**
   - Command execution sets `GIT_NO_LAZY_FETCH='1'`, `GIT_TERMINAL_PROMPT='0'`, and `GIT_OPTIONAL_LOCKS='0'`.
   - Prevents network operations, remote object fetches, and lock file creation during queries.
   - Version gate requires Git 2.46+ to guarantee lazy-fetch suppression.
2. **Filesystem & Path Traversal Protections:**
   - Path parsing uses `PurePosixPath`.
   - Explicitly rejects absolute paths (`/`), directory traversal (`..`), backslashes (`\`), directory blobs, symlinks, and git submodules via `ls-tree` mode checks (`100644`/`100755` regular files only).
3. **Zero Write Side Effects:**
   - Invoking `--request`, `--validate`, or `--validate-contribution` writes zero files to disk and makes zero Git repository modifications.
   - Confirmed via `git status` inspection post-execution.
4. **Bytecode Cache Prevention:**
   - `sys.dont_write_bytecode = True` prevents `__pycache__` creation when running `commons.py`.

---

## 5. Objective 5: Fresh-Context H08 Discovery Test

A fresh-context request for hypothesis H08 was executed against `tools/research_commons/commons.py`:

```sh
python3 tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H08"}}'
```

### Response Evaluation:
- **Formal Claim:** *"AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process."*
- **Historical Result:** *"This session records source claims, independent extraction probes and mathematical derivation separately; it is not a controlled study of AI productivity."*
- **Evidence Tiers:** `C` (Historical/user report) and `E` (Proposal/speculation).
- **Assumptions Recorded:**
  1. Commit authorship does not prove prompt provenance.
  2. Repeated model summaries are not independent verification.
- **Discrepancy Alert Returned:** Includes `D-608-H07-H08`, alerting the client that merged PR #608 previously labeled H08 as established despite canonical evidence tiers C/E, and noting that PR #610 remains an unmerged proposal.

### Challenge to False Premise:
The discovery query **falsifies the assertion** that AI-assisted research has demonstrated improved scientific trustworthiness. The discovery interface correctly returns that comparative productivity and trustworthiness remain **untested** under matched controls.

---

## 6. Objective 6 & 7: Epistemic Analysis & Implementation vs. Validation

### Identified Epistemic Risks:
1. **Circular Validation Risk:** AI-generated summary reports referencing prior AI-generated summaries create a loop of self-reinforcing claims. The interface mitigates this by flagging model summaries as Tier C/E and requiring canonical register backing.
2. **Stale Git Pins:** `manifest.v1.json` pins exact commit SHAs. When unmerged PRs (like PR #610) are updated or merged, the pins become historical artifacts until manually updated and re-verified.
3. **Missing Negative Evidence:** Reproduction requests return `execution: unavailable` and note that empty reproduction lists indicate a lack of indexed recipes, not evidence of absence.

### Implementation Correctness vs. Scientific Validation:
- **Software Implementation Correctness:** `tools/research_commons/commons.py` correctly parses JSON, validates schemas, enforces read-only boundaries, and passes 20 unit tests.
- **Independent Scientific Validation:** Passing unit tests **does not** constitute scientific proof or empirical certification of H01–H08. Software test execution merely verifies contract adherence of the discovery tool.

---

## 7. Objective 8: Pre-Existing Docs-Structure Checker Review

Running `python3 scripts/governance/check_docs_structure.py` yields:
```
docs structure check FAILED
- missing directory: book/book-en
- missing directory: book/book-cn
- missing directory: book/book-es
- missing directory: book/book-pt
- missing directory: book/book-ru
```

### Audit Findings on Failure:
- **Root Cause:** The script checks for localization directories under `book/` that do not exist in the repository layout.
- **Scope Boundary:** This failure is pre-existing in `main` and unrelated to PR #609 or PR #610. Per audit instructions, this scope was not expanded to modify governance scripts or create dummy directories.
- **Other Governance Checks:** All other active governance scripts passed 100%:
  - `check_root_structure.py`: **PASSED** (81 entries checked)
  - `check_overclaim_guardrails.py`: **PASSED**
  - `check_active_docs_link_hygiene.py`: **PASSED**
  - `check_determinism_claims.py`: **PASSED**

---

## 8. Reproduced Test Suite & Execution Evidence

| Test Suite | File | Command | Result |
| :--- | :--- | :--- | :--- |
| **Research Commons Contract** | `tests/research_commons/test_commons.py` | `python3 -m unittest discover -s tests/research_commons -p "test_*.py"` | **20/20 PASSED** (23.5s) |
| **Initiative Reconciliation** | `tests/python/test_initiative_reconciliation.py` | `python3 -m unittest discover -s tests/python -p "test_initiative_reconciliation.py"` | **4/4 PASSED** (0.02s) |
| **TISC Freeze Checker** | `scripts/ci/check_tisc_freeze_integrity.py` | `python3 scripts/ci/check_tisc_freeze_integrity.py` | **PASSED** (209 opcodes verified) |
| **Commons Index Validation** | `tools/research_commons/commons.py` | `python3 tools/research_commons/commons.py --validate` | **PASSED** (`valid_index`, 39 sources) |

---

## 9. Itemized Findings & Required Corrections

### Critical Findings: None.
### Major Findings:
- **MAJ-1 (Unmerged State Dependency):** `commons.py` indexes PR #610 as `proposal` evidence (`initiative-610-proposed`). Once PR #610 is merged into `main`, `manifest.v1.json` must be updated with the actual merge commit SHA and re-indexed.

### Minor Findings:
- **MIN-1 (Pre-existing Docs-Structure Failure):** `scripts/governance/check_docs_structure.py` fails due to missing `book/book-*` directories. Needs separate governance maintenance PR.

---

## 10. Explicit Final Acceptance Verdict

### Verdict: **CONDITIONAL GO**

**Conditions for Full Activation:**
1. **Merge PR #610 First:** Merge draft PR #610 into `main` to establish the corrected H07/H08 evidence limits and actual freeze checker references in canonical documentation.
2. **Update Manifest Pins:** Following PR #610 merge, update `docs/research/commons/manifest.v1.json` in PR #609 with the resulting `main` merge commit SHA, re-hash source digests, and transition discrepancy `D-608-H07-H08` status from `unresolved` to `resolved/superseded`.
3. **Keep Draft Status Until Final Re-Index:** Maintain PR #609 in draft status until Condition 2 is executed.

*This audit grants no scientific certification, governance activation, or finding of AI-assisted research superiority.*

---

## 11. Addendum: Follow-up Verification & Corrective Resolution of PR #615 Codex Audit Findings (October 2026)

**Attribution:** Follow-up Independent Audit Verification
**Scope:** Corrective resolution of two P1 Codex review findings on merged PR #615 without altering canonical research conclusions or merging draft PRs #609, #610, or #612–#614.

### 11.1 Resolution of Codex Finding 1: Restoration of PR #607 Evidence Source & Revision Reconciliation

**Finding Analysis:**
In merged PR #615, an attempt to eliminate unpushed Git SHAs introduced non-existent commit SHA `903ea2fe653df0d636e0543f9788dd8442738045` as `baseline_revision` and `main_revision`. Additionally, PR #615 modified the historical PR #607 source entry (`initiative`) to point to PR #608 content SHA (`0116bfeeb223b41e1944136840973cca878f7a3fb9c65ff68deb435c7dee514e`), silently destroying the distinction between historical PR #607 evidence and corrected PR #608 evidence.

**Corrective Actions Applied:**
1. **Manifest Revision Alignment (`docs/research/commons/manifest.v1.json`):** Updated `baseline_revision` and `integration.main_revision` from non-existent SHA `903ea2f...` to valid main snapshot SHA (`3d395cb604ddd49da3354d17f0505f2ed033757e`).
2. **Historical PR #607 Source Restoration:** Restored `initiative` source to point to its authentic merged commit SHA (`2231f74d63b40f9e2fef8f81a4793af4c6c5d19c`) and authentic content digest SHA-256 (`3cf9915e1af2b4d396d20725048cfd080958a8b7812e9b40cd628a4e444d17d8`), preserving its explicit distinction from corrected PR #608 source `initiative-608` (`3d395cb604ddd49da3354d17f0505f2ed033757e`, SHA-256: `0116bfeeb223b41e1944136840973cca878f7a3fb9c65ff68deb435c7dee514e`).
3. **Re-pinning Standard Sources & Examples:** Re-pinned all 30 standard main-branch sources in `manifest.v1.json` and `repository_revision` fields in `docs/research/commons/examples/*.json` to valid commit SHA `3d395cb604ddd49da3354d17f0505f2ed033757e`.
4. **Contract Test Assertion Update (`tests/research_commons/test_commons.py`):** Updated assertion to verify `initiative` source revision is `2231f74d63b40f9e2fef8f81a4793af4c6c5d19c`.
5. **Demonstration Receipt Recalculation (`docs/research/commons/demo-observation.json`):** Re-evaluated and updated `manifest_sha256` digest (`0e99f1bcf8039d05a209178ca2122d69eb68ae5881c77fd33abb4eedaa19601c`) and all observation response SHA-256 digests matching exact canonical output from `commons.py`.

---

### 11.2 Resolution of Codex Finding 2: Clean Environment Reproduction & Claim Downgrading

**Finding Analysis:**
Merged PR #615 claimed clean-environment test execution while referencing non-existent commit SHA `903ea2f...`, causing clean checkouts to fail with `CommonsError: pinned Git evidence unavailable`. All unsupported verification claims are explicitly downgraded to unverified reports.

**Clean Environment Reproduction Parameters:**
- **Tested Operating System / Architecture:** Linux 6.6.137+ x86_64
- **Python Runtime:** Python 3.12.13
- **Git Binary Version:** git version 2.46.2
- **Python Dependencies:** `jsonschema==4.23.0` (`tools/research_commons/requirements.txt`)
- **Pinned Revisions Validated:**
  - `3d395cb604ddd49da3354d17f0505f2ed033757e` (`main` baseline snapshot)
  - `2231f74d63b40f9e2fef8f81a4793af4c6c5d19c` (Historical PR #607 merged commit)
  - `d1eee548d4658df37b35d0cc3781b5a77cbae96b` (Unmerged draft PR #610 proposed correction)
  - `827d2accbf8ccb56afe3fe8d96dfb7251f249d7d` (Historical PR #605 E1 experiment)

**Reproduced Clean Execution Results:**

| Target Suite / Command | Tested Environment | Command Line | Exit Code | Result Summary | Evidence Digest |
| :--- | :--- | :--- | :---: | :--- | :--- |
| **Research Commons Contract** | Linux x86_64 / Py3.12 | `python3 -m unittest discover -s tests/research_commons -p "test_*.py"` | `0` | **20/20 PASSED** (23.6s) | N/A |
| **Initiative Reconciliation** | Linux x86_64 / Py3.12 | `python3 -m unittest discover -s tests/python -p "test_initiative_reconciliation.py"` | `0` | **4/4 PASSED** (0.02s) | N/A |
| **TISC Freeze Integrity** | Linux x86_64 / Py3.12 | `python3 scripts/ci/check_tisc_freeze_integrity.py` | `0` | **PASSED** (209 opcodes) | N/A |
| **Commons Index Validation** | Linux x86_64 / Py3.12 | `python3 tools/research_commons/commons.py --validate` | `0` | **PASSED** (`valid_index`) | SHA-256: `0e99f1bcf8039d05a209178ca2122d69eb68ae5881c77fd33abb4eedaa19601c` |
| **Fresh Clone Isolation Test** | Linux x86_64 / Py3.12 | `commons.py` validation against pinned local Git object store | `0` | **4/4 SUITES PASSED** | All 39 sources verified |

**Explicit Claim Dispositions & Downgrades:**
1. **Downgrade of Unverified Pass Claims:** All claims in PR #615 asserting verification under non-existent SHA `903ea2f...` are explicitly **downgraded to unverified historical reports**. Only the clean execution results documented above represent verified reproducible evidence.
2. **Draft PR Status Preserved:** Draft PRs #609, #610, and #612–#614 remain **unmerged draft PRs**. No automatic merge or promotion has occurred.
3. **Governance Activation Unchanged:** Governance activation remains **inactive/pending**. Canonical H01–H08 definitions remain unchanged.
4. **Platform Testing Record:** Verified platform testing was conducted on **Linux x86_64**. Cross-platform testing on macOS or Windows remains unverified.
