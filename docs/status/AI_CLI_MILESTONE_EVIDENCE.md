# AI CLI Milestone Evidence

## Evidence review — 2026-10-27

Last Updated: 2026-10-27

Purpose: bounded native milestone evidence, not a general inference completion claim. The October 13 account below is **Historical** where it describes missing reusable extraction: [DecodeState / IntermediateDecodeState](../../include/t81/vm/decode_state.hpp) and [decoder implementation](../../vm/decoder.cpp) now exist outside the CLI. The four-step example remains a bounded example; its token count is not a universal current runtime ceiling.

The readiness scripts are present but were not rerun with downloaded models. Refer to the [CLI matrix](AI_CLI_IMPLEMENTATION_MATRIX.md) for current source-backed behavior. No configured workflow compiles a Markdown document: AI CI checks freshness/contracts and conditionally builds code. The legacy final sentence about deterministic compilation of this document is **Unsupported**. Fixture/JSON parity is distinct from model quality, exhaustive greedy decoding and independent comparative assurance.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.
Current decoder API includes `NarrowGreedyLlamaDenseV1` and full-vocabulary argmax; the retained “true greedy decode missing” bullet is superseded for that narrow path. This is not evidence of general model quality or arbitrary architecture support.

## Retained record

Status: Active
Last Updated: 2026-10-13
Authority: `tools/cli/ai/ai_cli_shared.cpp`, `tests/cpp/cli_contract_test.cpp`, `examples/model-load-canonfs/`

## Purpose

This document records the current milestone evidence for the `t81 ai` command
family, with emphasis on the strict deterministic native inference lane.

## Current Milestone

The old milestone of proving that `t81 ai inference run` can execute a real
native path is complete for the strict deterministic `t81_reference_vm` lane.

What is real today:

- a bounded native inference lane exists for `t81 ai inference run`
- the lane is tokenizer-aware for the current narrow Llama-shaped path
- the lane now carries hidden-tensor, q/k, forward-state, and combined
  architecture-state evidence across decode steps
- the checked-in bounded horizon now reaches 4 decode steps
- the fourth step is a distinct deep architecture-state mode, not just a longer
  repeat of the earlier steps
- the payload now exposes explicit readiness/evidence posture through:
  - `ready`
  - `guarded`
  - `degraded`

## Runnable Evidence

Checked-in proof points:

- `ready`:
  [run_ready_ai_probe.sh](../../examples/ai-and-inference/model-load-canonfs/run_ready_ai_probe.sh)
- `guarded`:
  [run_guarded_ai_probe.sh](../../examples/ai-and-inference/model-load-canonfs/run_guarded_ai_probe.sh)
- `degraded`:
  [run_degraded_ai_probe.sh](../../examples/ai-and-inference/model-load-canonfs/run_degraded_ai_probe.sh)
- bounded forward/architecture-state lane:
  [run_forward_state_ai_probe.sh](../../examples/ai-and-inference/model-load-canonfs/run_forward_state_ai_probe.sh)

The current bounded forward-state probe demonstrates:

- `generated_tokens: 4`
- `bounded_horizon_steps: 4`
- `bounded_horizon_remaining: 0`
- `bounded_horizon_utilization: 1`
- `bounded_horizon_reached: true`
- `termination_reason: "deep_architecture_state_horizon_reached"`
- a later `architecture_state_feedback_state_transition.v1`
- a deeper fourth-step
  `architecture_state_deep_feedback_state_transition.v1`

## Contract Evidence

Relevant contract coverage:

- [cli_contract_test.cpp](../../tests/cpp/cli_contract_test.cpp)

Relevant implementation surface:

- [ai_cli_shared.cpp](../../tools/cli/ai/ai_cli_shared.cpp)

## What This Does Not Yet Prove

This milestone does not mean that T81 already has a finished general-purpose
interactive native inference runtime.

What is still missing:

- reusable runtime-state extraction beyond CLI-shaped glue
- a more general intermediate-state object beyond the current bounded
  compiled-literal carry path
- extension beyond the current bounded 4-step horizon only after that reusable
  state exists
- promotion from the current bounded decode lane to a true greedy decode path
- convergence of `benchmark run` and `policy test` onto that same reusable
  native execution path

## Related Status Docs

- [AI_CLI_IMPLEMENTATION_MATRIX.md](AI_CLI_IMPLEMENTATION_MATRIX.md)
- [AI_RFC_BACKLOG.md](AI_RFC_BACKLOG.md)
- [../BUILDABLE_NEXT_STEPS.md](../BUILDABLE_NEXT_STEPS.md)

## Build Evidence
- The CI pipeline executes deterministic compilation validation against `docs/status/AI_CLI_MILESTONE_EVIDENCE.md` per CI rules.
