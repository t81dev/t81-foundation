# Active Tasks

Last Updated: 2026-03-26
Owner: @t81dev

Immediate, actionable items only. Structural hardening items belong in
`HARDENING_BACKLOG.md`. Long-horizon planning belongs in `ROADMAP.md`.

---

## Current Priority

T81 is in a handoff-and-hardening phase, not a breadth-expansion phase.

The main near-term goal is simple:

- keep `main` boring,
- keep newcomer-facing docs aligned with reality,
- and keep RFC-00D1 as the clearest draft-to-code lane in the repo.

---

## Now

| ID | Task | Why It Matters | Acceptance Signal |
| :--- | :--- | :--- | :--- |
| T-01 | Clear current `main` CI on `069963ea` | Handoff quality is weak if branch health is uncertain | Required contexts complete successfully on `main` |
| T-02 | Keep Windows + ARM64 portability regressions closed | Recent CI churn has concentrated here | `T81 Foundation CI` runs clean across Windows/MSVC, Windows/clang-cl, and Linux/ARM64 |
| T-03 | Harden RFC-00D1 CanonFS interchange error semantics and examples | RFC-00D1 is the best current buildable lane for another engineer | CLI/docs/schemas/tests stay aligned; negative-path behavior is explicit and tested |
| T-04 | Keep status / handoff docs current | New contributors and agents now depend on these docs as the primary map | `README`, `HANDOFF`, `BUILDABLE_NEXT_STEPS`, `AGENTS`, and `/docs/status` agree on current priorities |

---

## Next

| ID | Task | Notes |
| :--- | :--- | :--- |
| N-01 | Add one golden CanonFS import/export example under `examples/` | Prefer a real artifact plus contract-test coverage |
| N-02 | Tighten CanonFS interchange error messages and negative tests | Focus on finishable RFC-00D1 hardening, not new surface area |
| N-03 | Keep public contracts explicit | Schemas, CLI output, and provenance artifacts should evolve together |
| N-04 | Narrow RFC-00D0 before implementation | Resolver/descriptor prototype first; no broad TCP/IP build-out yet |

---

## Explicitly Deferred

- broad TCP/IP implementation work beyond RFC-00D0 narrowing
- subsystem-count expansion for its own sake
- major terminology churn across docs and specs
- speculative rewrites of working CLI / CanonFS interchange code

---

## Recently Closed

| Item | Closed |
| :--- | :--- |
| RFC-00D1 implementation seed: `canonfs import` / `canonfs export` CLI landed | 2026-03-26 |
| RFC-00D1 schemas, runtime validation, and CI JSON contract checks landed | 2026-03-26 |
| RFC-00D1 core interchange modules and non-CLI ops API landed | 2026-03-26 |
| RFC-00D1 policy profiles (`permissive`, `import-only`, `export-only`, `deny-all`) landed | 2026-03-26 |
| Handoff docs and newcomer path tightened (`HANDOFF`, `ROADMAP`, `BUILDABLE_NEXT_STEPS`, README updates) | 2026-03-26 |
| `AGENTS.md` added and refined to guide future AI contributors | 2026-03-26 |

---

## Rule

If a task does not improve one of these, it is probably not the right next task:

- CI reliability
- newcomer legibility
- RFC-00D1 finishability
- portability across supported lanes
