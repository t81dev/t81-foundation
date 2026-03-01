# t81vm/ — Conformance Programs for spec/t81vm-spec.md

Programs in this directory encode normative invariants from `spec/t81vm-spec.md`.

## Planned Programs (SE-M3)

| Program | Normative Ref | Invariant |
| :--- | :--- | :--- |
| `determinism-profile.t81` | §1 | Tier A: identical input → identical output |
| `axion-log-completeness.t81` | §5 | Every privileged op emits an AxionEvent |

These programs are planned for SE-M3 (target: 2026-04-30).
