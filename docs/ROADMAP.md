# T81 Contributor Roadmap

This is the short roadmap for someone trying to build T81 forward without first absorbing the entire historical planning corpus.

## Roadmap goal

Move T81 from "ambitious and credible" toward "pick-up-able and extensible by new contributors" without weakening its determinism, governance, or provenance posture.

Use [RFC-00D2](../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)
as the target-architecture map, not as permission to widen implementation
scope ahead of proof.

## Phase 1: Make the current value obvious

Primary goal: make the runtime story clearer than the long-horizon OS story.

Current posture: the bounded AI OS-object family is now a protected subsystem,
so this phase should favor stabilization and truthful framing over additional
family breadth.

Success looks like:

- a newcomer can tell what T81 is in under a minute
- the runtime path feels complete enough to try and explain
- the docs clearly distinguish:
  - usable now
  - experimental but demoable
  - draft architectural direction

Recommended work:

- continue tightening README and docs entry points
- reduce duplicated roadmap/status material
- keep handoff documents current
- keep contributor-facing examples and smoke paths aligned with the docs that point to them

## Phase 2: Harden the CanonFS interchange lane

Primary goal: turn RFC-00D1 from a good seed into a contributor-friendly subsystem.

Success looks like:

- import/export semantics are stable enough for non-authors to build against
- policy and provenance are real, not placeholder-shaped
- interchange behavior is documented in a small number of obvious files
- the stable seed contract is named explicitly so examples, tests, and tooling
  can depend on it without pretending the whole RFC is finished

Recommended work:

- deepen policy-profile semantics
- add more negative/edge-case tests
- clarify which RFC-00D1 surfaces are draft vs implementation detail
- keep the current stable seed explicit:
  `canonfs import` / `canonfs export`,
  `t81.canonfs-import.v1`,
  `t81.canonfs-export.v1`,
  structured interchange `reason` / `kind` / `code`,
  explicit `provenance_schema` / `manifest_schema` result fields,
  provenance/manifest schemas,
  `host-file` / `host-directory`,
  and current policy-profile names
- keep the golden example plus frozen `v1/` fixture set aligned with the same
  build-against boundary
- consider promoting more of RFC-00D1 only after the deferred v1 questions stop
  moving

## Phase 3: Pick one narrow new subsystem and keep it narrow

Primary goal: avoid broad new architectural spread.

Hardware/interposer acceleration remains external to T81's current stable
runtime surface; if convergence is revisited later, it should happen only
through a narrow experimental backend seam after the current runtime and
RFC-00D1 lanes are stable.

Best candidate today:

- RFC-00D0 service descriptor and resolution prototype before full TCP/IP breadth

Success looks like:

- a small base-81-aware resolver exists
- CanonFS-backed descriptor loading works
- API and evidence behavior are testable without pretending the full stack is done

## Phase 4: Keep the repo boring to operate

Primary goal: portability, CI, and docs should not become the bottleneck.

Success looks like:

- CI failures are mostly meaningful, not housekeeping
- formatting and portability regressions are fixed quickly
- Windows/macOS/Linux lanes stay trustworthy

Recommended work:

- keep workflow drift low
- keep portable helper patterns centralized
- avoid reintroducing ad hoc platform-specific builtins or checkout gaps
- make contributor/demo scripts fail fast on missing host tools instead of failing deep in the run

## What not to do next

- do not expand scope across many draft subsystems at once
- do not rewrite stable accepted surfaces just because the repo is large
- do not make the OS/bare-metal lane the only public story
- do not let planning docs multiply faster than buildable work

## Decision rule

Prefer work that does one of these:

- makes a current surface easier to adopt
- turns draft work into something another engineer can finish
- removes project dependence on one maintainer’s memory

De-prioritize work that mainly adds conceptual breadth without making the existing system easier to use or extend.
