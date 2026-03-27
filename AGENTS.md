# AGENTS.md

Purpose: Make T81 progressively more pick-up-able and extensible by new contributors while preserving determinism, pre-side-effect policy enforcement through Axion, and immutable provenance through CanonFS.

## Project Identity

T81 Foundation is a deterministic, policy-gated runtime stack for auditable computation, with a longer-term systems direction rooted in ternary-native design.

The strongest current story is not "general-purpose operating system." It is:

- deterministic execution,
- policy enforcement before side effects,
- immutable content/provenance through CanonFS,
- and a repo with real CLI, test, schema, and CI surfaces.

When making changes, preserve that framing.

## Read This First

Start in this order:

1. [README.md](./README.md)
2. [docs/HANDOFF.md](./docs/HANDOFF.md)
3. [docs/ROADMAP.md](./docs/ROADMAP.md)
4. [docs/BUILDABLE_NEXT_STEPS.md](./docs/BUILDABLE_NEXT_STEPS.md)
5. [docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md](./docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md)

For architecture work:

6. [spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md](./spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
7. [spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md](./spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)

## Current Priority Lanes

Prefer work in these areas:

- RFC-00D1 CanonFS interchange hardening
- newcomer/handoff clarity
- deterministic runtime and CLI stability
- CI portability and boring `main`

Do not expand subsystem count casually. Favor finishing existing lanes over starting new ones.

## Stable Enough To Build Against

These areas have enough implementation and test weight to extend carefully:

- CLI/runtime path
- core deterministic VM/data surfaces
- Axion policy concepts
- CanonFS interchange contracts and schemas
- accepted RFC surfaces already referenced by tests, docs, and CI

## Draft Or Design-Led Areas

These need more restraint:

- RFC-00D1 is draft but partially implemented
- RFC-00D0 is still primarily architectural
- broader TernaryOS ambitions remain real but earlier-maturity

Do not treat draft RFC text as license to overbuild. Narrow the implementation seed first.

## Contribution Rules

- Keep the public story narrow: deterministic, governed runtime first.
- Do not casually reposition T81 as an OS-first project.
- Prefer explicit contracts, schemas, and tests over implied behavior.
- Preserve provenance and policy semantics when adding import/export or execution behavior.
- Avoid broad refactors unless they clearly reduce duplication or improve transferability.
- If a change touches portability or platform assumptions, verify Linux, macOS, Windows, and ARM64 implications before opening a PR.

## CI Expectations

`main` should stay operationally boring.

Before concluding work, prefer to verify the narrowest meaningful slice locally:

- targeted `cmake` configure/build
- relevant tests
- relevant scripts under `scripts/ci/`

If CI is red, fix the concrete failure before starting more breadth work.

## RFC Guidance

### RFC-00D1

This is the best current draft-to-code bridge in the repo.

Implemented or partly implemented areas include:

- `canonfs import`
- `canonfs export`
- interchange/provenance/result schemas
- runtime and CI JSON validation
- core interchange modules
- policy-profile recording

If choosing a practical lane, choose this one first.

### RFC-00D0

Treat this as design-led for now.

The intended posture is:

- standard TCP/IP wire compatibility
- Base-81 used for identity, representation, service discovery, and observability
- deterministic internal replay from recorded event/timer traces, not deterministic real-world network outcomes

Do not jump straight into a full stack implementation without narrowing the resolver/descriptor surface first.

## First Good Tasks

Good tasks for a new agent or contributor:

- tighten CanonFS interchange error semantics
- improve CanonFS interchange examples and docs
- keep schema/docs/CLI behavior aligned
- fix portability or CI regressions
- tighten newcomer-path docs without inflating scope

Less good tasks:

- speculative subsystem rewrites
- premature TCP/IP implementation breadth
- broad terminology churn across the repo

More concrete high-value tasks include:

- tighten CanonFS interchange error messages and add matching negative tests
- add or improve one golden CanonFS import/export example under `examples/`
- ensure documented CLI commands have matching contract-test coverage
- fix current CI warnings or portability regressions across Linux, macOS, Windows, and ARM64
- update `docs/BUILDABLE_NEXT_STEPS.md` with the next smallest finishable items

## Success Metric

A good contribution usually makes at least one of these noticeably better without increasing overall architectural surface area:

- newcomer onboarding time
- test coverage in a stable lane
- clarity of a public contract, schema, or workflow
- CI reliability and portability confidence

## Working Style

- Be explicit about what is real today versus aspirational.
- Prefer finishable increments.
- Leave code, docs, and tests in a state another engineer can pick up.
- When in doubt, reduce ambiguity rather than add surface area.

For autonomous agents: reference the current priority lanes and stable surfaces when proposing or making changes. Prefer edits that reduce maintainer memory load. If a change would expand a draft area, narrow the scope first instead of broadening the implementation opportunistically.
