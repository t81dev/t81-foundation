# T81 Handoff

This document is the shortest practical handoff for a new maintainer or serious contributor.

## T81 in one paragraph

T81 is a deterministic, policy-gated runtime for auditable AI inference, built on a ternary-native execution model, immutable CanonFS storage, and Axion policy enforcement. The project also includes an in-progress guest OS and bare-metal kernel direction, but the clearest usable value today is: governed execution, reproducible traces, content-addressed artifacts, and a spec-first architecture that treats determinism as a system property rather than a best-effort runtime flag.

## What is real today

- Deterministic VM/runtime with cross-platform reproducibility gates
- Axion policy engine with fail-closed behavior and audit surfaces
- CanonFS immutable object storage and provenance-oriented workflows
- Working CLI, install path, Docker path, and QEMU demos
- Large accepted RFC corpus with active CI enforcement
- CanonFS foreign file system interchange seed from RFC-00D1:
  - import/export CLI
  - core interchange module
  - schema artifacts
  - validation tooling
  - basic policy-profile support
  - stable-enough seed contract:
    `t81.canonfs-import.v1`, `t81.canonfs-export.v1`,
    `t81.canonfs-import-provenance.v1`,
    `t81.canonfs-export-provenance.v1`,
    `t81.canonfs-interchange-manifest.v1`,
    explicit interchange error `reason` / `kind` / `code`,
    explicit `provenance_schema` / `manifest_schema` result fields,
    `host-file` / `host-directory`, and the current policy-profile names

## What is still draft or in motion

- RFC-00D0: base-81-aware TCP/IP stack
- RFC-00D1: CanonFS foreign file system interchange remains draft overall, but the current v1 JSON seed contract is now stable enough for examples, tests, and adjacent tooling
- RFC-00D1 now explicitly names its current v1 candidate contract surface and promotion blockers, so contributors do not need to reconstruct that boundary from implementation details
- RFC-00D1 "stable enough to build against" should be read narrowly: use the
  current JSON/result/provenance/manifest seed contract, but do not assume that
  text output, symlink posture, archive/bundle export, or schema-catalog
  promotion questions are already settled
- Broader bare-metal / TernaryOS ambitions are real but not yet the easiest public adoption path
- Some repo narratives still over-emphasize the long-horizon OS story relative to the current runtime value

## What should not be casually rewritten

- Deterministic execution invariants
- Canonical serialization and hash identity behavior
- Axion fail-closed and pre-side-effect enforcement posture
- Accepted RFC contract language in `spec/rfcs/`
- Public API boundaries under `include/t81/` without explicit compatibility review

## What can be simplified

- Documentation sprawl and overlapping roadmap/status narratives
- Contributor entry points
- Internal duplication where CLI-only logic can move into reusable core modules
- Public framing that currently mixes runtime, OS, kernel, and future hardware stories too early

## Architectural priorities

When in doubt, prefer these in order:

1. Preserve determinism and evidence quality.
2. Preserve policy-before-side-effect behavior.
3. Preserve CanonFS object and provenance integrity.
4. Narrow and harden working surfaces before expanding scope.
5. Keep draft work explicitly draft until the boundary is truly stable.

## Current phase

The bounded AI OS-object family is now a protected subsystem, not an exploratory
lane. Treat `assess-fixed`, `route-fixed`, and `classify-fixed` as the current
reference family and prefer stability, guardrails, portability, and claim
accuracy over adding a fourth composition.

## Current best entry points

- Public overview: [README.md](../README.md)
- RFC catalog: [spec/rfcs/index.md](../spec/rfcs/index.md)
- Documentation index: [docs/README.md](README.md)
- Contributing guide: [CONTRIBUTING.md](../CONTRIBUTING.md)
- Buildable next tasks: [docs/BUILDABLE_NEXT_STEPS.md](BUILDABLE_NEXT_STEPS.md)
- Contributor-facing roadmap: [docs/ROADMAP.md](ROADMAP.md)
- Bounded family status: [docs/status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md](status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md)
- AI CLI reality check: [docs/status/AI_CLI_IMPLEMENTATION_MATRIX.md](status/AI_CLI_IMPLEMENTATION_MATRIX.md)

The short version:

- `Now`: RFC-00D1 policy-profile depth plus CI/portability boringness
- `Next`: RFC-00D1 contract-promotion follow-through plus runtime-first docs cleanup
- `Later`: RFC-00D0 resolver prototype, kept narrow

## If you only have 30 minutes

1. Read [README.md](../README.md) through `Choose your path`.
2. Skim [docs/BUILDABLE_NEXT_STEPS.md](BUILDABLE_NEXT_STEPS.md) and pick one lane.
3. Read the matching RFC before touching code.
4. Run the narrowest local verification for that lane before changing anything broad.

## Highest-leverage next tasks

1. Finish making the public story match the actual usable runtime.
2. Promote one or two narrow draft surfaces from “interesting” to “obviously finishable.”
3. Continue reducing CLI-local logic in favor of reusable core modules.
4. Keep CI boring and green; do not let portability debt accumulate.
5. Tighten the line between accepted, draft, and experimental surfaces in docs and code.

## Suggested owner mindset

Treat T81 as a serious research-grade runtime that needs product discipline, not as a greenfield invention project. The main risk is no longer “lack of architecture.” The main risk is that the architecture stays easier to admire than to extend.

## Good first contribution shape

- pick a narrow lane with an existing RFC and tests
- avoid repo-wide refactors on first contact
- keep determinism, policy, and CanonFS integrity ahead of convenience
- prefer making one draft surface clearer over adding a new subsystem
