# Output Presentation Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Output Presentation Contract](#output-presentation-contract)
  - [Core Pattern](#core-pattern)
  - [AI Inference JSON](#ai-inference-json)
  - [Tensor-Backed CLI Success](#tensor-backed-cli-success)
  - [Governance Demo Output](#governance-demo-output)
  - [Drift Rules](#drift-rules)
  - [Enforcement](#enforcement)

<!-- T81-TOC:END -->


Purpose: keep user-facing results in a stable "answer first, evidence second"
shape across CLI, JSON, and example wrappers.

This contract does not change execution behavior or remove evidence. It only
defines the presentation order that result-producing surfaces should follow.

## Core Pattern

Use this order whenever a command returns a result rather than just performing
an administrative action:

1. `result_summary`
   One sentence answering what happened.
2. `result_class`
   A stable classification of the result shape such as `tensor_handle`,
   `bounded_inference_text`, `probe_output`, or `governed_execution`.
3. `result_meaning`
   One sentence answering what the result actually is.
4. `next_step_hint`
   One sentence naming the most immediate supported next action.
5. `termination_reason`
   Why the run stopped, when the command exposes a termination condition.
6. User-visible output
   A handle, preview, text snippet, or explicit statement of the visible result.
7. Identifiers
   Model ids, hashes, manifest refs, provenance refs, and similar stable handles.
8. Full evidence
   Trace data, architecture-state blocks, logits, policy reasons, or other deep
   runtime material.

## AI Inference JSON

For `t81 ai inference run`, the top-level JSON header should present these keys
before deep evidence sections like `decode_trace`, `hidden_tensor_summary`,
`kv_state_summary`, or `architecture_state_summary`:

1. `result_summary`
   Explains the outcome in one sentence.
2. `result_class`
   Names the class of result exposed to the user.
3. `result_meaning`
   Explains what the result actually represents.
4. `next_step_hint`
   Names the most direct supported next action.
5. `termination_reason`
   States why the bounded run ended.
6. `output_kind`
   Names the low-level output representation.
7. `output_preview`
   Gives the first user-facing preview of the output.

Deep evidence must remain intact below these fields.

## Tensor-Backed CLI Success

For `t81 code run` when the printed output is only a tensor handle, present:

1. compile success
2. `result kind: tensor_handle`
3. one short operation line explaining what executed
4. the existing handle caveat
5. one short `next:` line pointing to the supported inspection path
6. the printed handle and normal termination line

This keeps the command honest without pretending a handle is materialized data.

## Governance Demo Output

For the governance wrapper:

1. show the underlying CLI result in runtime order
2. add one short result-class line on success
3. add at most one short next-step line on success
4. keep deny-side trap evidence unchanged

The wrapper should explain what passed or failed without obscuring the native
CLI output underneath.

## Drift Rules

Future changes should avoid:

- inserting new high-detail evidence fields above `result_summary`
- reintroducing duplicate top-level `termination_reason` or `output_kind`
- inventing one-off `result_class` names when an existing class already fits
- omitting `result_meaning` or `next_step_hint` when the result is otherwise likely
  to be correct but ambiguous
- making wrappers present results in a different order than the native CLI

## Enforcement

The primary guardrail lives in `tests/cpp/cli_contract_test.cpp`, which should
assert the relative ordering of the top-level AI JSON fields and the stable
ordering of trap and success surfaces.
