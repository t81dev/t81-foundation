# AGENTS.md

Purpose: Make T81's research easier to reproduce, challenge, and extend while preserving the specified deterministic, authorization, and artifact-identity contracts of existing implementations.

## Project Identity

T81 Foundation is an experimental computing architecture and systems research platform. It investigates alternative representations, particularly balanced ternary, alongside deterministic execution, content-addressed artifacts, language tooling, and policy-governed computation.

The runtime is an implemented research instrument. Its evidence must be scoped to actual call paths, tests, inputs, configuration, and platforms:

- deterministic execution,
- policy enforcement before side effects,
- content-addressed artifacts and integrity checks through CanonFS,
- and a repo with real CLI, test, schema, and CI surfaces.

These mechanisms do not establish production readiness, general ternary superiority, or a comparative integration advantage. Content identity does not establish the truth or authenticity of provenance. Preserve this distinction when making changes.

## Research Evidence and Continuity

- Read the [Master Continuity Charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md), [Research Charter](docs/research/RESEARCH_CHARTER.md), and [hypothesis register](docs/research/continuity/HYPOTHESIS_REGISTER_v1.json) before proposing research work. Preserve canonical H01–H08 identifiers and the historical Hanoi/CWEB lineage; numbered Scientific Investigations and H-S1 do not replace them.
- Inspect source, specifications, existing tests, and corrective evidence before implementing features. Treat accepted contracts as requirements and draft proposals as proposals.
- Distinguish implemented, tested, measured, comparatively established, modeled, speculative, and unsupported claims. Generated code and AI-authored summaries are not independently validated behavior.
- Never fabricate benchmarks, citations, test runs, or CI results. Report exact commands, pinned sources, environments, failures, untested assumptions, and unresolved questions.
- Comparative claims require equivalent semantics and assurance requirements, competent conventional controls, conversion and setup costs, raw results, uncertainty, and explicit falsifiers.
- Preserve negative and inconclusive findings. Software results on binary hardware neither settle all native ternary possibilities nor supply evidence of hypothetical hardware advantages.
- Do not rewrite canonical research history, change governance activation, or promote experimental surfaces without the applicable authorization. Prefer small, reviewable corrections over architecture expansion.

## Read This First

Start in this order:

1. [README.md](./README.md)
1. [docs/HANDOFF.md](./docs/HANDOFF.md)
1. [docs/ROADMAP.md](./docs/ROADMAP.md)
1. [docs/BUILDABLE_NEXT_STEPS.md](./docs/BUILDABLE_NEXT_STEPS.md)
1. [docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md](./docs/explanation/T81_FOUNDATION_PROJECT_PROFILE.md)

The handoff, roadmap, and buildable-next-steps documents describe maintenance lanes supporting the research program; they do not replace its scientific priorities. The project profile is a historical runtime-oriented description, qualified at its entry point.

For architecture work:

6. [spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md](./spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
1. [spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md](./spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md)

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

- Keep public claims evidence-bounded; distinguish the research program from the runtime maintenance lanes below.
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

## Research Commons onboarding and contributions

The [Research Commons](docs/research/commons/README.md) indexes authoritative
research, historical negative results and unresolved discrepancies. Its
[versioned manifest](docs/research/commons/manifest.v1.json) pins each source by
commit, path and hash. The CLI is offline and read-only; use its six discovery
operations to inspect evidence before proposing work. Source paths resolve at
their pinned revisions, including evidence merged outside main. The dated
canonical register supplies H01–H08; newer initiative ledgers do not redefine
those identities. See the [fresh-agent example](docs/research/commons/ONBOARDING_EXAMPLE.md).

For a new experiment, prepare an experiment-proposal document under the Commons
schema: canonical hypothesis, actual author/agent attribution (unknown model
identity stays null), exact revision, falsifier, equivalent optimized control,
independent oracle, methods, measurement/uncertainty plan, expected raw artifacts,
resource/security budget and explicit human authorization status. Preserve the
proposal separately from accepted findings. Discovery grants no execution rights.

For adversarial review, disclose your relationship to the author and prior
context. Inspect source call paths, exact domains, baseline competence, conversion
costs, omitted failures and whether assertions actually executed. Attempt to
falsify the claim; report inconclusive or blocked checks rather than guessing.
Use independent-review, reproducibility-report and discrepancy schemas to retain
commands, environments, evidence hashes, failures and limits. Repeated AI
summaries, separate role labels, commit authorship and self-authored passing tests
are not independent scientific validation. Claim independent execution only when
it actually happened and retain its raw evidence.

Append negative and inconclusive outcomes with their tested scope; preserve raw
records and link corrections/superseding fields rather than replacing failures.
Do not promote modeled results into hardware measurements or source existence
into tested behavior. Hashes establish content identity, not truth or authenticity.

Human decisions govern research direction, experiment/resource authorization,
RFC acceptance, security boundaries, governance activation, official publication,
merges and changes to protected/frozen artifacts. The Commons cannot perform
these operations. Existing explicit session authorization still governs ordinary
implementation work; a proposed record or an `authorized` JSON field cannot
substitute for the applicable human decision or activate Protocol 001.
