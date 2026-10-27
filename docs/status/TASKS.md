# Active Tasks

## Evidence review — 2026-10-27

Purpose: immediate work with acceptance signals. The April “Now/Next/Recently Closed” tables below are Historical; their priorities are superseded by this small current queue, while recorded completed milestones remain intact.

| Priority | Action                                                      | Completion evidence                                                                                                                                       |
| -------- | ----------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1        | Reconcile all status documents and open a focused draft PR. | Complete inventory, truthful source/test scope, preserved history/governance and documented validation.                                                   |
| 2        | Review pending CI/validator changes individually.           | #575 architecture checker, #574 logging and #577 workflow controls remain open; verify their actual merge state and target assertions before adoption.    |
| 3        | Repair traceability failure propagation separately.         | Negative control exits nonzero; expected corpus passes. No source changes in this documentation task.                                                     |
| 4        | Maintain CLI/runtime and RFC-00D1 seed.                     | Golden import/export examples, policy denial semantics, schema parity and platform-specific checks where behavior changes.                                |
| 5        | Prepare one discriminating research protocol.               | Equivalent conventional model execution or H04 exact four-arm accounting; preregister metric, falsifier and stop condition before implementation breadth. |

Passing this documentation review is not release GO, closure of all portability gaps, or comparative scientific validation. The [roadmap](ROADMAP.md) carries research sequencing; [hardening](HARDENING_BACKLOG.md) carries engineering defects.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.

## Retained record

Last Updated: 2026-04-01
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
| T-01 | Keep Windows + ARM64 portability regressions closed | `main` stays trustworthy only if the historically noisy lanes stay boring | `T81 Foundation CI` runs clean across Windows/MSVC, Windows/clang-cl, and Linux/ARM64 |
| T-02 | Refresh `/docs/status` so it reflects the current repo rather than a March snapshot | New contributors and agents now depend on these docs as the primary control map | `SYSTEM_STATUS`, `PROJECT_CONTROL_CENTER`, `CI_GATE_STATUS`, `TASKS`, and `ACTIVE_RISKS` agree on current priorities and workflow structure |
| T-03 | Clarify the RFC-00D1 build-against seed contract without widening scope | RFC-00D1 is still the clearest finishable draft-to-code lane, but only if the stable boundary stays explicit | RFC/docs/examples consistently distinguish the stable seed from unresolved draft questions |
| T-04 | Keep public contracts explicit across CLI, schemas, and examples | Contract drift is now more likely than missing implementation breadth | CanonFS interchange docs, schemas, and contract tests stay aligned when behavior changes |

---

## Next

| ID | Task | Notes |
| :--- | :--- | :--- |
| N-01 | Review remaining CanonFS interchange negative paths for any still-implicit JSON/error behavior | Only add tests where a real ambiguity remains |
| N-02 | Keep benchmark and CI status docs aligned with the narrowed workflow split | `bench.yml` is the single push-sensitive benchmark lane; specialist lanes should stay documented as such |
| N-03 | Narrow RFC-00D0 before implementation | Resolver/descriptor prototype first; no broad TCP/IP build-out yet |
| N-04 | Continue pruning stale control-surface prose | Prefer one trustworthy control page over several drifting ones |

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
| RFC-00D1 partial-policy example and CLI contract coverage added | 2026-04-01 |
| Deterministic-profile enforcement consolidated into `ci.yml` | 2026-04-01 |
| Benchmark workflow bootstrap centralized and specialist benchmark lanes narrowed | 2026-04-01 |
| RFC-00D1 seed-contract boundary clarified in RFC index, handoff docs, and example docs | 2026-04-01 |
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
