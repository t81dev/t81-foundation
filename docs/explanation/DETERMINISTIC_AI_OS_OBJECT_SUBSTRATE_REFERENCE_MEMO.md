# T81 Deterministic AI OS-Object Substrate Reference Memo

## Executive Summary

T81's strongest current technical story is not "general AI operating system."
It is a deterministic, policy-gated execution substrate with immutable CanonFS
artifacts and a small bounded family of AI OS-object chains.

Today that family includes:

- `assess-fixed`
- `route-fixed`

Both chains run through the same typed helper pipeline, and both explicitly
satisfy the current Canonical Identity Invariant: for identical task, model,
policy, and input, object identity at every validated layer is content-derived
and independent of CanonFS root or execution location.

The relevant public and contract surfaces are aligned in:

- [README.md](../../README.md)
- [FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)
- [AI_TASK_SHARED_FRAMEWORK_CONTRACT.md](../reference/AI_TASK_SHARED_FRAMEWORK_CONTRACT.md)
- [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)

## Current Truthful Thesis

The most defensible current thesis is:

T81 is a deterministic governed runtime stack with pre-side-effect policy
enforcement through Axion, immutable content-addressed persistence through
CanonFS, and a bounded AI task pipeline that materializes canonical result
objects rather than leaving AI execution at the level of probes or runtime
handles.

That thesis is visible in:

- [README.md](../../README.md)
- [AGENTS.md](../../AGENTS.md)
- [FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)

It does not justify claiming:

- general-purpose OS completeness
- broad AI serving
- orchestration capabilities

## What Is Real and Proven Today

Implemented and build-against surfaces include:

- the shared AI task runner in
  [ai_cli_shared.cpp](../../tools/cli/ai/ai_cli_shared.cpp)
- the typed artifact helper surface in
  [main.cpp](../../tools/cli/main.cpp)
- the bounded chain catalog in
  [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
- the runnable examples in
  [examples README](../../examples/ai-and-inference/model-load-canonfs/README.md)

Tested and enforced surfaces include:

- the canonical `assess-fixed` composition proof in
  [ai_task_assess_fixed_composition_test.cpp](../../tests/cpp/ai_task_assess_fixed_composition_test.cpp)
- the `route-fixed` composition proof in
  [ai_task_route_fixed_composition_test.cpp](../../tests/cpp/ai_task_route_fixed_composition_test.cpp)
- the helper and CLI contract coverage in
  [cli_contract_test.cpp](../../tests/cpp/cli_contract_test.cpp)
- required CI execution in
  [ci.yml](../../.github/workflows/ci.yml)

The current validated object model is explicit and narrow:

- result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- bundle: canonical top-level object

## Deterministic AI OS-Object Pipeline

The current pipeline is not just "AI execution."

A bounded task such as `assess-fixed` or `route-fixed`:

1. runs in the strict deterministic lane
2. stores a canonical result artifact and a provenance artifact in CanonFS
3. drives a typed downstream record through the artifact helpers
4. ends with a bundle stored as the canonical object for the completed chain

That flow is:

- described for newcomers in
  [FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)
- cataloged for implementers in
  [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
- normed in
  [AI_TASK_SHARED_FRAMEWORK_CONTRACT.md](../reference/AI_TASK_SHARED_FRAMEWORK_CONTRACT.md)

The helpers involved remain narrow and typed:

- `t81 ai task ...`
- `t81 ai task read-field`
- `t81 artifact write-store-record`
- `t81 artifact store-bundle`
- `t81 artifact read-field`
- `t81 canonfs get`

## What Is Proven About Identity and Determinism

The strongest current proof is no longer just task-level repeatability.

For both `assess-fixed` and `route-fixed`, the composition tests explicitly
prove that identical task, model, policy, and input yield identical:

- `result_ref`
- `provenance_ref`
- `record_ref`
- `bundle_ref`

and identical fetched bytes for the stored objects, even across different
CanonFS roots.

That family-level invariant is now stated in:

- [FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)
- [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)

The important consequence is that the canonical top-level object is
content-addressed rather than environment-addressed.

The following are not part of identity for the currently validated family:

- storage location
- temporary paths
- execution locality

## Stable vs Draft Surfaces

Stable enough to build against:

- the bounded AI task family and its shared runner/materializer/descriptor
  contract in
  [AI_TASK_SHARED_FRAMEWORK_CONTRACT.md](../reference/AI_TASK_SHARED_FRAMEWORK_CONTRACT.md)
- the typed downstream record and bundle helpers in
  [main.cpp](../../tools/cli/main.cpp)
- the current compositions and their schema families in
  [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
- the contract and CI guardrails in
  [cli_contract_test.cpp](../../tests/cpp/cli_contract_test.cpp) and
  [ci.yml](../../.github/workflows/ci.yml)

Draft or restraint-required areas remain:

- broader OS direction
- broader AI inference surfaces outside the bounded strict lane
- RFC-led future architecture not yet anchored in equivalent tests, contracts,
  and helpers

## What an Outside Reader Could Still Misunderstand

A skim reader could still mistake the presence of "OS-object" language for a
claim of general OS completeness.

They could also confuse the existence of `ai inference run` and probe surfaces
with a mature general inference product surface.

The repo's own docs push against that reading, but the distinction matters:
the strongest implemented AI story today is the bounded typed object pipeline,
not general inference breadth.

Another likely misunderstanding is to treat the current family as evidence that
any future composition automatically inherits the same invariant.

The docs correctly do not make that claim; the invariant is explicitly limited
to the currently validated `assess-fixed` and `route-fixed` chains.

## Planning Ahead Without Breaking Discipline

The right forward plan is not architectural expansion. It is disciplined
extension from the current validated base.

The next smallest credible step is to treat the current bounded family as the
standard every future composition must meet:

- same shared runner
- same artifact-first object model
- same typed helper path
- same bundle-as-canonical rule
- same explicit cross-root proof burden before the composition is considered
  stable

The next milestone should therefore be one of two things only:

- another bounded composition that is structurally different but still
  closed-world and schema-bound
- deeper validation or documentation hardening of the current family

The wrong next steps remain:

- chat
- agents
- orchestration frameworks
- broad schema generalization
- treating the current AI lane as generalized serving

The repo already has a usable release and tag process in:

- [release.yml](../../.github/workflows/release.yml)
- [python-wheels.yml](../../.github/workflows/python-wheels.yml)
- [docker.yml](../../.github/workflows/docker.yml)

The important planning posture is to keep future changes inside the same
evidentiary discipline, not to widen the runtime.

## Final Assessment

The current repository justifies a strong but narrow claim:

T81 now has a small validated family of deterministic, policy-gated AI
OS-object chains whose top-level canonical objects are content-addressed,
typed, persistent, and root-independent for identical inputs.

That is materially more than "we can run deterministic AI probes," and
materially less than "we have a complete AI operating system."

This memo is strongest when it stays on that line.
