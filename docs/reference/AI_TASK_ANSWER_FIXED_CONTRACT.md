# `answer_fixed.v1` Contract

`answer_fixed.v1` is the first governed, deterministic AI task primitive in T81.

Canonical runnable example:

- For the current end-to-end runnable example of the AI task framework, see the `Assess-Fixed
  OS-Object Chain` section in
  [examples/ai-and-inference/model-load-canonfs/README.md](../../examples/ai-and-inference/model-load-canonfs/README.md)
  and the companion script
  [run_assess_fixed_host_action.sh](../../examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh).
- That example shows the current full chain from AI task result to final bundle object; this
  document remains the normative contract for `answer_fixed.v1` itself.

It is intentionally narrow:
- one task kind
- one-step deterministic execution
- one approved model artifact per run
- one canonical result artifact
- one provenance artifact with retained execution evidence

## Command Shape

The command is:

```bash
t81 ai task answer-fixed \
  --model-file <path> \
  --policy <file.apl> \
  (--input <text> | --input-file <path>) \
  [--canonfs-root <path>] \
  [--mode strict_deterministic]
```

Rules:
- `--policy` is required.
- exactly one of `--input` or `--input-file` is required.
- `--mode` must be `strict_deterministic`.
- the command fails closed for unsupported models, unsupported architectures, or denied policy.

## Policy Semantics

`answer_fixed.v1` reuses the existing Axion policy surface.

Before execution, the command requires:
- exactly one `allowed-ternary-model-hashes` entry
- that entry must match the selected model artifact checksum
- one `require-axion-event` reason equal to `task:answer_fixed.v1`

If any of those checks fail, the command exits before execution and emits a `SecurityFault`-style denial.

This is a narrow admission helper for this task kind. It is not a new policy subsystem.

## Execution Mode

Execution is restricted to:
- backend: `t81_reference_vm`
- determinism class: `strict_deterministic`
- selection policy: single-step deterministic max-score selection
- runtime path: existing native VM probe path

`answer_fixed.v1` does not expose probe output as its primary result.

## Vocabulary

The initial fixed vocabulary is:

- token `42` => `YES`
- token `9` => `NO`
- any other selected token => `UNKNOWN`

The current demo model is built so:
- input `greet hello` yields `YES`
- input `hello world` yields `NO`

## Input Normalization

Input is normalized to a single string before hashing and execution.

The task execution input is the normalized string itself. The task identity is carried by:
- the command name
- the result schema
- the provenance schema
- the policy marker `task:answer_fixed.v1`

## Result Artifact

The canonical result artifact is JSON with stable field order:

```json
{
  "schema": "t81.ai.task.answer-fixed.v1",
  "task": "answer_fixed",
  "input_hash": "<sha3-256 ref>",
  "model_hash": "<sha3-512 ref>",
  "answer": "<YES|NO|UNKNOWN>",
  "answer_token_ids": [<int>],
  "termination_reason": "<string>"
}
```

Properties:
- no tensor handles
- no probe traces
- deterministic serialization
- stable bytes for identical input, model, policy, and CanonFS root

## Provenance Artifact

The provenance artifact is JSON with stable field order:

```json
{
  "schema": "t81.ai.task.provenance.v1",
  "task": "answer_fixed",
  "result_ref": "<canonfs ref>",
  "input_hash": "<sha3-256 ref>",
  "model_hash": "<sha3-512 ref>",
  "policy_result": "allowed",
  "policy_hash": "<sha3-256 ref>",
  "determinism_class": "strict_deterministic",
  "backend": "t81_reference_vm",
  "termination_reason": "<string>",
  "execution_evidence": { ... }
}
```

Properties:
- full execution evidence remains in provenance
- evidence is not duplicated into the result artifact
- provenance is CanonFS-addressed and retrievable independently

## CanonFS Storage Guarantees

Each successful run writes two CanonFS raw-block objects:
- result artifact -> `result_ref`
- provenance artifact -> `provenance_ref`

Guarantees for identical input, model, policy, and CanonFS root:
- identical `result_ref`
- identical result artifact bytes
- stable provenance structure

Important distinction:
- result/provenance artifacts are stored as CanonFS raw blocks
- retrieve them with `t81 canonfs get`
- `t81 canonfs export` is for interchange manifests and directories, not raw task-result blocks

## CLI Output Contract

Successful CLI output must include, in order near the top:

1. `result_summary`
2. `result_class`
3. `result_meaning`
4. `next_step_hint`
5. `termination_reason`
6. `output_kind`
7. `output_preview`
8. `result_ref`
9. `provenance_ref`
10. `policy_result`

For `answer_fixed.v1`:
- `result_class` is `ai_task_result`
- `output_kind` is `canonical_task_answer`
- `output_preview` is the bounded answer string
- `next_step_hint` must point to `t81 canonfs get <result_ref> --out <path>` for result retrieval and `t81 canonfs get <provenance_ref>` for provenance inspection

## Failure Contract

The following must fail closed:
- missing `--policy`
- non-strict mode
- unsupported architecture
- wrong model hash
- missing `task:answer_fixed.v1` policy marker

Policy denials must happen before execution.

## Reusable Skeleton For Future AI Tasks

Reusable pieces:
- policy parsing and admission
- strict deterministic backend selection
- model hash verification
- CanonFS raw-block writes
- contract-shaped CLI result output
- provenance capture with retained evidence

Task-specific pieces:
- input normalization semantics
- vocabulary mapping
- result artifact schema
- termination semantics
- task marker string

Future tasks should reuse the same skeleton and only replace the task-specific layer.
