# cognitive-tiers/ — Conformance Programs for spec/cognitive-tiers.md

Programs in this directory encode normative invariants from `spec/cognitive-tiers.md`.

## Planned Programs (SE-M6)

| Program | Normative Ref | Invariant |
| :--- | :--- | :--- |
| `tier-annotation-enforcement.t81` | §1 | `@tier(N)` blocks N+1 tier opcodes |

This program is planned for SE-M6 (target: 2026-06-01), after `@tier` enforcement
is implemented in the T81Lang compiler and VM.
