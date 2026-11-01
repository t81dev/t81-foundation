# Repository Archaeology Methodology

**Mission ID:** T81-RA-001
**Baseline SHA:** `f775680aba7db200691c0f4e2a3174b0e7e51581`
**Date:** 2026-10-30

## Principles
1. **Source-Grounded Baseline:** All 4,080 tracked files are extracted from `git ls-tree -r -z HEAD` at the pinned baseline commit.
2. **Bounded Batching:** Extraction and classification occur in bounded batches of 40 files to avoid unbounded prompt growth or memory exhaustion.
3. **Strict Epistemic Taxonomy:**
   - **Lifecycle:** `CURRENT`, `DRAFT`, `HISTORICAL`, `SUPERSEDED`, `ARCHIVED`, `GENERATED`, `UNKNOWN`.
   - **Authority:** `NORMATIVE_CONTRACT`, `SPECIFICATION`, `IMPLEMENTATION`, `TEST_ASSERTION`, `EXPERIMENTAL_EVIDENCE`, `GOVERNANCE_VALIDATOR`, `DESCRIPTIVE`, `UTILITY`, `DRAFT_PROPOSAL`.
   - **Evidence Tier:** Tier A (Reproduced on Pinned Source), Tier B (Inspected Source Code), Tier C (Historical/External Report), Tier D (Mathematical/Model Derivation), Tier E (Proposal/Speculation).
4. **Relocation Without Mutation:** Relocation candidates are analyzed for governance impact, root allowlists (`check_root_structure.py`), and test dependencies without modifying source paths.
5. **Stratified Sampling:** Auditing performs stratified sampling across content categories to independently verify classification validity.
