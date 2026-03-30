# Cross-Document Conflict Register

| Documents Involved | Nature of Contradiction | Probable Authoritative Source | Recommended Resolution |
|---|---|---|---|
| `docs/architecture/OVERVIEW.md` vs Repo Directory Root (`isa/`, `vm/`, `fs/`) | The overview maps sub-systems to `core/isa/`, `core/vm/`, `src/canonfs/`, but the actual directory structure is flattened (`isa/`, `vm/`, `fs/`). | Repo Directory Root | Update `docs/architecture/OVERVIEW.md` to reflect flattened repository structure (`isa/`, `vm/`, `fs/`, `include/t81/types/`). |
| `docs/ROADMAP.md` vs `docs/status/ROADMAP.md` | Duplicate roadmap documents. The `docs/status/` version is ~7x larger and likely the canonical tracking document, whereas `docs/ROADMAP.md` is shorter and likely an outdated summary or symlink that drifted. | `docs/status/ROADMAP.md` | Replace `docs/ROADMAP.md` with a direct relative link (markdown link or single-line redirect text) to `docs/status/ROADMAP.md`, or delete it completely if link hygiene allows. |
| `docs/HANDOFF.md` vs Directory Structure | Mentions architecture subsystems but doesn't explicitly misname paths like `OVERVIEW.md`, but relies on `OVERVIEW.md` for exact paths. It also links to `docs/ROADMAP.md`. | Directory Structure | Fix `OVERVIEW.md` first. Update `docs/ROADMAP.md` links to `docs/status/ROADMAP.md`. |
| `docs/BUILDABLE_NEXT_STEPS.md`, `docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md` | Outdated roadmap link. | Directory Structure | Update `docs/ROADMAP.md` links to `docs/status/ROADMAP.md`. |
