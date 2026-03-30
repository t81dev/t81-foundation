# Prioritized Remediation Plan

## Priority 0: Critical Corrections Required Immediately

1. **Target**: `docs/architecture/OVERVIEW.md`
   - **Why**: Directly misleads contributors on codebase structure, breaking onboarding.
   - **Action**: Update the "Architecture File Style Guide", "Current State by Layer", and "Evidence" sections. Change paths from `core/isa/` to `isa/`, `core/vm/` to `vm/`, `core/types/` to `include/t81/types/`, and `src/canonfs/` to `fs/`.
   - **Evidence Required**: `ls -l isa/ vm/ fs/ include/t81/types/` confirms these directories exist at the root level.

2. **Target**: `docs/ROADMAP.md`
   - **Why**: Duplicate document causing split-brain confusion. The authoritative status roadmap is `docs/status/ROADMAP.md` (~21KB vs ~3KB).
   - **Action**: Replace `docs/ROADMAP.md` with a stub pointing to `docs/status/ROADMAP.md` (or delete if link hygiene allows, but a stub prevents broken links).
   - **Evidence Required**: `ls -l docs/ROADMAP.md docs/status/ROADMAP.md`

3. **Target**: `scripts/governance/check_docs_structure.py`
   - **Why**: Broken script causing CI/governance failures.
   - **Action**: Fix the `BOOK_DIRS` array to point to `docs/book/book-*`.

## Priority 1: Root Docs and Onboarding

1. **Target**: `docs/HANDOFF.md`, `docs/BUILDABLE_NEXT_STEPS.md`, `docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md`
   - **Why**: The entry points for serious contributors. Make sure they point to the correct paths and status documents.
   - **Action**: Review references to architecture and status docs, ensure they align with the corrected paths from Priority 0 (`docs/status/ROADMAP.md`).
   - **Evidence Required**: Run `check_active_docs_link_hygiene.py`.
