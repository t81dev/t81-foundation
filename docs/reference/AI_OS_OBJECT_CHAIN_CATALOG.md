# AI OS-Object Chain Catalog

This document lists the current bounded deterministic AI OS-object chains in
T81.

For the newcomer-facing explanation of the current canonical runnable chain,
see
[FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](/Users/t81dev/Code/t81-foundation/docs/explanation/FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md).

These chains are:

- deterministic
- policy-gated
- typed
- CanonFS-backed
- helper-driven

They are not a workflow engine or a general orchestration layer. They are a
small bounded family of reusable compositions built on the current AI task and
artifact helper surfaces.

Confirmed invariant:

- for identical task, model, policy, and input, the current chains keep the
  same content-addressed object identity across repeated runs and across
  different CanonFS roots
- for the validated `assess-fixed` chain, the canonical final bundle also keeps
  the same content-addressed identity across different CanonFS roots
- for the validated `route-fixed` chain, the canonical final bundle also keeps
  the same content-addressed identity across different CanonFS roots

## Assess-Fixed Host Action Chain

Purpose:

- run one bounded governed assessment task
- turn the stored AI result into one fixed downstream host-action decision
- persist the completed chain as one canonical bundle object

Task schema/result family:

- task: `assess_fixed.v1`
- result artifact schema: `t81.ai.task.assess-fixed.v1`
- provenance artifact schema: `t81.ai.task.provenance.v1`

Downstream record schema:

- `t81.ai.task.assess-fixed.host-action-record.v1`

Final bundle schema:

- `t81.ai.task.assess-fixed.bundle.v1`

Object roles:

- task result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- final bundle artifact: canonical

Canonical runnable example:

- [run_assess_fixed_host_action.sh](/Users/t81dev/Code/t81-foundation/examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh)

Portable proof path:

- [ai_task_assess_fixed_composition_test.cpp](/Users/t81dev/Code/t81-foundation/tests/cpp/ai_task_assess_fixed_composition_test.cpp)
  via `./build/t81_ai_task_assess_fixed_composition_test ./build/t81`

Canonical top-level object:

- the stored bundle `t81.ai.task.assess-fixed.bundle.v1`

## Route-Fixed Path Selection Chain

Purpose:

- run one bounded governed route-selection task
- turn the stored AI route result into one fixed downstream path-selection record
- persist the completed chain as one canonical bundle object

Task schema/result family:

- task: `route_fixed.v1`
- result artifact schema: `t81.ai.task.route-fixed.v1`
- provenance artifact schema: `t81.ai.task.provenance.v1`

Downstream record schema:

- `t81.ai.task.route-fixed.path-selection-record.v1`

Final bundle schema:

- `t81.ai.task.route-fixed.bundle.v1`

Object roles:

- task result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- final bundle artifact: canonical

Canonical runnable example:

- [run_route_fixed_path_selection.sh](/Users/t81dev/Code/t81-foundation/examples/ai-and-inference/model-load-canonfs/run_route_fixed_path_selection.sh)

Portable proof path:

- [ai_task_route_fixed_composition_test.cpp](/Users/t81dev/Code/t81-foundation/tests/cpp/ai_task_route_fixed_composition_test.cpp)
  via `./build/t81_ai_task_route_fixed_composition_test ./build/t81`

Canonical top-level object:

- the stored bundle `t81.ai.task.route-fixed.bundle.v1`

## Boundary

This catalog covers the current bounded reusable composition family only.

It does not define:

- a workflow engine
- a general packaging system
- a general AI orchestration layer
- new inference behavior
