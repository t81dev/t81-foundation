# AI CLI Milestone Evidence

Status: Active
Last Updated: 2026-03-29
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
  [run_ready_ai_probe.sh](../../examples/model-load-canonfs/run_ready_ai_probe.sh)
- `guarded`:
  [run_guarded_ai_probe.sh](../../examples/model-load-canonfs/run_guarded_ai_probe.sh)
- `degraded`:
  [run_degraded_ai_probe.sh](../../examples/model-load-canonfs/run_degraded_ai_probe.sh)
- bounded forward/architecture-state lane:
  [run_forward_state_ai_probe.sh](../../examples/model-load-canonfs/run_forward_state_ai_probe.sh)

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
