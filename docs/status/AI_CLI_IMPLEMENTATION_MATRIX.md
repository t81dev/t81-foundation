# AI CLI Implementation Matrix

Last Updated: 2026-03-28
Authority: `tools/cli/ai/ai_cli_shared.cpp`, `tools/cli/main.cpp`, related tests and examples

This document answers one narrow question:

- what `t81 ai ...` commands are real today,
- what commands are mostly orchestration/reporting scaffolds,
- and what is still missing for a true native inference product surface.

## Summary

The `t81 ai` command family is real and first-class in the CLI, but it is not yet a finished native end-user inference runtime.

Today, the strongest real model-execution lanes are:

- `t81 weights ...`
- `t81 code run ... --weights-model ...`
- `t81 internal llama-run ...` for the experimental bridge path

The biggest current gap is that `t81 ai inference run` is only partly real today:

- real for the strict deterministic `t81_reference_vm` lane, where it executes a
  narrow native VM probe over an imported model
- still scaffolded for the other backend modes, which continue to emit
  synthetic/reporting payloads

Readiness example coverage today:

- `ready`: proven by
  [run_ready_ai_probe.sh](/Users/t81dev/Code/t81-foundation/examples/model-load-canonfs/run_ready_ai_probe.sh)
- `guarded`: proven by
  [run_guarded_ai_probe.sh](/Users/t81dev/Code/t81-foundation/examples/model-load-canonfs/run_guarded_ai_probe.sh)
  using the real tiny Hugging Face Llama artifact, and now uses explicit
  guarded-caution evidence policies instead of looking identical to `ready`
- the bounded native lane now also emits a top-level `artifact_visibility`
  block so callers can inspect evidence posture in one place
- the simple `ready` single-probe lane now emits the same top-level
  `artifact_visibility` shape, using `not_applicable_single_probe.v1`
  markers where bounded-decode-specific evidence does not exist
- when the bounded combined architecture state is active, `artifact_visibility`
  now also carries the architecture-state `confidence_envelope`,
  `architecture_state_stability_kind`, and
  `architecture_state_guardrail_triggered` fields alongside `readiness`
- the bounded native lane now also carries an explicit `forward_state`
  object, derived from the bounded hidden projection and used on the later
  decode transition path
- that carried `forward_state` now also has its own class/signature and can
  shape later decode transitions directly
- the carried `forward_state` now also records generation depth and chains the
  previous forward-state signature into later-step state evolution, so it is no
  longer overwritten as a purely single-step snapshot
- the carried `forward_state` now also persists bounded prior rows with decay,
  ranks and trims them intentionally, and exposes a runnable 4-step evolution
  path through
  [run_forward_state_ai_probe.sh](/Users/t81dev/Code/t81-foundation/examples/model-load-canonfs/run_forward_state_ai_probe.sh)
- that forward-state path now also has visibly different later control modes,
  including an architecture-state-led fourth-step continuation beyond the
  earlier 3-step ceiling:
  `forward_state_history_feedback_state_transition.v1`,
  `forward_state_history_projection.v1`, and
  `prefer_tail_nonnegative_else_max.v1`
- that fourth-step continuation now also has its own deeper architecture-state
  control mode instead of repeating step 3 unchanged:
  `architecture_state_deep_feedback_state_transition.v1`,
  `architecture_state_deep_feedback_window.v1`, and
  `architecture_state_deep_feedback_projection.v1`
- when that deeper fourth-step architecture-state mode is actually reached, the
  top-level confidence envelope now upgrades to
  `bounded_deep_architecture_state_probe.v1` and reports
  `architecture_state_deep_feedback_used: true`
- the top-level payload now also emits a compact `forward_state_summary`, so
  callers can see max generation depth and carried-state usage without walking
  the full trace
- later decode steps now also expose the actual combined carried-state input
  that was fed back into the VM through `consumed_state_input_row_ids`, rather
  than only the forward-state and hidden-projection slices separately
- the top-level payload now also emits a compact `state_input_summary`, so
  callers can see how much combined carried state actually influenced later VM
  steps without reading the full trace
- that `state_input_summary` now also carries a final state-input signature, so
  callers can distinguish the actual merged carried-state control input from
  the forward-state summary alone
- the native lane now also exports real intermediate hidden-tensor evidence
  from live VM state:
  - `intermediate_tensor_export_supported`
  - `hidden_tensor_summary`
  - per-step `hidden_tensor_signature_sha256`
  - and later decode window seeding now prefers that hidden-tensor signature
    before looser merged row-state digests
- the bounded forward-state lane now also imports that carried hidden tensor
  back into later decode probes through the compiled tensor-pool path:
  - `true_state_carry_supported: true`
  - `state_carry_limitations.kind:
    "bounded_intermediate_tensor_literal_import.v1"`
  - `state_carry_limitations.import_path:
    "compiled_tensor_literal_reimport.v1"`
  - `state_carry_limitations.import_compute_path:
    "attn0_plus_carried_hidden_blend.v1"`
  - per-step `hidden_tensor_import_used`
  - per-step `hidden_tensor_blend_used`
  - carried hidden tensors now evolve across steps and report
    `hidden_tensor_carry_mode_kind:
    "evolved_hidden_tensor_feedback.v1"`
  - later decode transitions can now enter
    `hidden_tensor_feedback_state_transition.v1` with
    `candidate_basis_kind: "hidden_tensor_feedback_window.v1"`
  - top-level `kv_state_summary` now exposes a bounded
    `bounded_qk_tensor_state.v1` artifact derived from live `q1`/`k1`
    tensor signatures
  - top-level `architecture_state_summary` now exposes a bounded combined
    `bounded_hidden_tensor_qk_forward_state.v1` artifact, and later decode
    window seeding now prefers that combined architecture-state signature
  - later decode transitions can now enter
    `architecture_state_feedback_state_transition.v1` with
    `candidate_basis_kind: "architecture_state_feedback_window.v1"` and
    `decode_mode_kind: "architecture_state_feedback_projection.v1"`
  - `architecture_state_summary` now also carries a compact confidence/stability
    readout for that combined state
  - per-step `decode_trace` and `final_decode_state` now also expose
    `architecture_state_confidence_score` and
    `architecture_state_stability_kind`, so callers can see the combined
    architecture-state posture without inferring it from the generic probe
    `stability` block
  - that combined architecture-state stability is now also part of the
    recovery/termination path for later bounded decode steps
  - top-level `bounded_decode_health` and `readiness` now switch to
    `confidence_envelope: "bounded_architecture_state_probe.v1"` when the
    combined architecture state is active, and they surface
    `architecture_state_guardrail_triggered` plus
    `architecture_state_stability_kind`
  - when combined architecture state is active, hidden-tensor carry evolution
    is now architecture-state-conditioned instead of using the old fixed
    75/25 blend, and the carry mode reports
    `architecture_state_evolved_hidden_tensor_feedback.v1`
  - bounded Q/K state now also evolves across steps, with
    `kv_state_carry_mode_kind:
    "architecture_state_evolved_qk_signature.v1"` on the architecture-state-led
    path, and that evolved KV signature feeds the combined architecture state
  - top-level `kv_state_summary` now also reports `feedback_steps`, so the
    bounded Q/K carry depth is visible without reading the full decode trace
  - top-level `architecture_state_summary` now also reports `feedback_steps`,
    so callers can see how many later decode steps were actually
    architecture-state-led
  - top-level `hidden_tensor_summary` now also reports `feedback_steps`, so
    hidden-tensor carry depth is visible alongside the forward/KV/architecture
    summaries
- the remaining limitation is narrower now: this is a bounded compiled-literal
  carry path, not a general KV-cache or arbitrary intermediate-tensor import
  primitive
- the checked-in forward-state probe now proves that bounded hidden-tensor
  carry is on the real compute path, not just the reporting path:
  - later decode steps report `hidden_tensor_import_used: true`
  - `forward_state_generation` reaches `2`
  - `consumed_state_input_row_ids` shows the merged carried-state rows that
    fed back into later VM steps
- `degraded`: proven by
  [run_degraded_ai_probe.sh](/Users/t81dev/Code/t81-foundation/examples/model-load-canonfs/run_degraded_ai_probe.sh)
  using a checked-in synthetic fixture that forces `decode_probe_unavailable`,
  and by
  [run_real_hf_tiny_model.sh](/Users/t81dev/Code/t81-foundation/examples/model-load-canonfs/run_real_hf_tiny_model.sh)

## Classification

- `real`: performs a concrete implemented operation with meaningful current value
- `scaffold`: implemented command surface, but payloads or decisions are still mostly synthetic or policy/reporting-oriented
- `stub`: intentionally minimal compatibility surface; not a real execution lane

## Matrix

| Command | Status | Classification | Notes |
| :--- | :--- | :--- | :--- |
| `t81 ai backend capabilities` | implemented | real | Emits a concrete backend capability JSON document |
| `t81 ai backend select` | implemented | real | Runs actual backend-selection logic and emits a trace payload |
| `t81 ai model inspect <model-file>` | implemented | real | Checks existence, infers format, fingerprints the file |
| `t81 ai verify determinism <model-file>` | implemented | scaffold | Real file inspection, but not a deep conformance replay or execution proof |
| `t81 ai verify --model <hash>` | implemented | scaffold | Hash-oriented reporting surface, not runtime verification |
| `t81 ai inference run ...` | implemented | mixed | Real for `strict_deterministic` + `t81_reference_vm`; other modes still emit synthetic payloads |
| `t81 ai quantization inspect ...` | implemented | scaffold | Emits quantization-shaped metadata, not a full analysis pass |
| `t81 ai benchmark run ...` | implemented | scaffold | Emits benchmark-shaped payload with fixed values, not measured execution |
| `t81 ai policy test ...` | implemented | scaffold | Event-type allow/deny behavior is mostly table-driven, not full runtime policy replay |
| `t81 ai workflow run ...` | implemented | scaffold | Produces workflow artifact scaffolding |
| `t81 ai workflow replay <artifact>` | implemented | scaffold | Artifact existence/reporting surface |
| `t81 ai workflow report <artifact>` | implemented | scaffold | Human-readable report over saved workflow artifact |
| `t81 ai observability trace <artifact>` | implemented | scaffold | Emits a canned trace-shaped artifact |
| `t81 ai quantize --input <file>` | implemented | real | Runs a small real ternary quantization transform over integer input |
| `t81 ai run --model <hash>` | implemented | stub | Compatibility alias; prints a minimal pass surface, not real inference |

## Evidence

Primary implementation surface:

- [ai_cli_shared.cpp](/Users/t81dev/Code/t81-foundation/tools/cli/ai/ai_cli_shared.cpp)
- [main.cpp](/Users/t81dev/Code/t81-foundation/tools/cli/main.cpp)

Key observations from the current implementation:

- backend capability and selection logic are concrete and deterministic enough to be useful today
- file inspection and fingerprinting are real
- `inference run` now executes a real native VM probe for the strict deterministic reference-VM lane
- for Llama-shaped imports, that native probe now covers:
  - embedding lookup via `std.tnn.embed`
  - a two-layer attention slice
  - bounded row-wise logits scoring against `lm_head.weight`
- when a companion `tokenizer.json` is present, the current native lane now
  attempts a narrow tokenizer seed path:
  - exact whole-prompt vocab lookup first
  - then a merge-aware normalized prompt walk using the tokenizer's
    SentencePiece-style `▁` prefix convention, seeded from the last matched
    subword token
- when no exact tokenizer seed is available, the lane falls back to the existing
  prompt-hash seeded window
- the current native JSON payload now includes:
  - `execution_kind: "real_vm_native_probe"`
  - `probe_kind: "llama_two_layer_attention_row_logits_sample"` for the current Llama lane
  - `candidate_selection` metadata, including:
    - `mode`
    - `tokenizer_seed_supported`
    - `basis`
    - `vocab_size`
    - `prompt_token_history_token_ids`
    - `prompt_token_history_count`
    - `seed_token_id`
    - `window_start`
    - `window_end`
    - `window_ids`
  - for multi-token prompts, the initial candidate window can now be seeded from
    the full matched prompt-token history while the actual embed input still
    uses the last matched prompt token
    - this now reports `mode: "tokenizer_prompt_history"` instead of
      overloading `tokenizer_prompt_token`
  - `sampled_logits`
  - `selected_candidate`
  - `stateful_decode_supported: false`
  - `requested_max_tokens`
  - `termination_reason`
  - `decode_trace` for the current bounded three-step history-feedback trace
    - later steps now use the previously selected/generated token as the actual
      `std.tnn.embed(...)` input while still choosing candidate windows from
      prompt-plus-history feedback
    - when tokenizer matching yields more than one prompt token, step `0` now
      starts with bounded prompt-token context instead of an empty context set,
      and the initial candidate window can be prompt-history seeded
    - later steps can also blend the current generated token with accumulated
      token-history context via repeated `std.tensor.vec_add`, currently using a
      bounded recent-history window
    - that bounded window now preserves the original prompt anchor token when
      the recent history grows, instead of dropping prompt conditioning
    - each step now carries an explicit
      `state_kind: "prompt_history_bounded_context.v1"` decode-state block shape
    - each step now also separates decode-state carry from candidate-window
      selection through `transition_kind`, `next_window_start`,
      `state_seed_sha256`, and `candidate_window_seed_sha256`
    - later decode windows are now seeded from a carried hidden-state probe
      artifact:
      `hidden_carry_row_ids`, `hidden_carry_scores`, and
      `hidden_carry_signature_sha256`
    - that carry artifact is derived from a dedicated hidden-carry probe over
      the real native `attn1` state, rather than only from prompt/history
      bookkeeping
    - later decode steps now also consume the previously carried hidden rows as
      part of the actual VM input path, but now through a narrower bounded
      hidden-state projection:
      `hidden_projection_row_ids`, `hidden_projection_scores`,
      `hidden_projection_signature_sha256`, and
      `consumed_hidden_projection_row_ids`
    - the projection now also emits a small stable hidden-state class:
      `hidden_state_class` and `hidden_state_class_signature_sha256`, now
      shaped by the hidden-carry probe layout as well as the projection scores
    - later decode transitions now use a class-and-projection-mode-conditioned
      window policy rather than only a raw digest lookup, surfaced as
      `window_selection_kind:
      "hidden_state_class_projection_mode_conditioned_window_seed.v1"`
    - later decode steps now also emit a per-step candidate-window rationale
      aligned with that richer control path, surfaced as `candidate_basis_kind`
    - later decode steps and the final decode state now also emit a compact
      `state_rationale` block summarizing the active class, carry layout,
      carry mode, selection policy, and decode mode
    - later decode steps and the final decode state now also emit a bounded
      `stability` block derived from sampled-logit margin and hidden-carry
      peak evidence
    - low-margin `steady` steps now take the same explicit recovery branch as
      `fragile` or `ambiguous` steps, so the recovery path is exercised on a
      real bounded decode lane instead of being dead logic
    - the top-level payload now summarizes whether recovery was used through
      `recovery_triggered` and `recovery_steps`
    - repeated weak or recovery-grade bounded decode steps now terminate
      intentionally instead of pretending confidence held, surfaced as
      `stability_recovery_exhausted`, `weak_steps`, and
      `termination_reason: "stability_recovery_exhausted"`
    - when a companion tokenizer is present, the top-level payload now also
      emits `generated_token_ids`, `generated_token_pieces`, and a compact
      `generated_text_preview`, so bounded native decode output is easier to
      inspect than raw token ids alone
    - degraded bounded decode now uses a conservative
      `generated_preview_policy`, clipping preview output to the first decoded
      token piece instead of showing the full low-confidence preview
    - degraded bounded decode now also uses a conservative `output_policy`,
      suppressing the raw VM probe `output` and replacing it with an explicit
      `output_summary`
    - degraded bounded decode now also uses a conservative
      `decode_trace_policy`, exposing boundary steps instead of the full trace
      once later bounded steps are no longer equally trustworthy
    - degraded bounded decode also switches `decode_trace_detail_policy` to a
      summary-only view, keeping signatures and counts while suppressing later
      raw evidence arrays in `decode_trace` and `final_decode_state`
    - degraded bounded decode now also switches top-level
      `logits_evidence_policy` to a summary-only view, keeping
      `sampled_logits_count`, `sampled_logits_signature_sha256`, and
      `selected_candidate` while suppressing the raw `sampled_logits` array
    - degraded bounded decode now also switches `candidate_selection` to a
      summary-only evidence view, keeping `candidate_window_count`,
      `candidate_window_signature_sha256`, and other metadata while
      suppressing raw `window_ids` and prompt-token id arrays
    - degraded bounded decode now also emits a compact
      `degraded_artifact_summary` so callers can see, in one place, which
      evidence surfaces were clipped or suppressed
    - the top-level payload now also emits a compact `readiness` block so
      callers can judge, at a glance, whether the bounded native lane ended in
      a `ready`, `guarded`, or `degraded` posture
    - the top-level payload now also emits `bounded_decode_health`, and the
      overall `status` can degrade from `pass` to `degraded` when bounded
      native decode exhausts repeated weak or unavailable recovery steps
    - the carried `forward_state` is no longer just a freshly overwritten
      snapshot:
      later steps now persist a bounded slice of prior forward-state rows with
      score decay, and the state records `forward_state_generation` so
      multi-step runs can prove real carried-state evolution
    - once that carried state is established, later decode steps can now shift
      into an explicitly history-heavy forward-state mode with a distinct
      transition kind, decode mode, carry layout, and selection policy
    - the hidden-state class now also conditions the bounded sample width for
      later decode steps, surfaced as `sample_window_kind` and
      `sample_window_used`
    - later decode steps also use a class-conditioned bounded candidate
      selection policy, surfaced as `selection_policy_kind`
    - later decode steps now also use a class-conditioned decode mode that
      changes how much context and hidden projection they consume, surfaced as
      `decode_mode_kind` and `context_window_used`
    - the carried hidden projection itself is now mode-shaped before it is
      stored into decode state, surfaced as `projection_carry_mode_kind`
    - that carry mode now also affects the next bounded decode window seed, not
      only the stored and consumed projection shape
    - later decode steps now also use a projection-mode-shaped hidden-carry
      probe layout, surfaced as `carry_probe_layout_kind`
    - includes `input_token_id`, `context_anchor_token_id`,
      `prompt_token_history_token_ids`, `generated_token_history_token_ids`,
      `combined_history_token_ids`, `context_history_window`,
      `context_history_token_ids`, `seed_token_id`, and
      the state/candidate seed digests
  - `final_decode_state` for the last realized bounded decode step, now using
    the same explicit bounded decode-state shape and transition metadata
- `inference run` still fabricates output as `deterministic:<prompt-hash-prefix>` for the non-native scaffolded lanes
- `benchmark run` currently emits fixed latency and throughput values
- `policy test` currently decides allow/deny via a narrow event-type check
- `run --model <hash>` is a compatibility stub, not a model-execution path

## Relationship To Other Inference Surfaces

### Real execution surfaces today

- `t81 weights import|info|verify|quantize|export`
- `t81 code run ... --weights-model <model.t81w|sha3-256:hash>`
- `t81 repl --weights-model ...`

These surfaces are backed by the model import path, CanonFS registration, Axion policy, and VM tensor execution.

### Experimental bridge surface today

- `t81 internal llama-run <model.gguf|sha3-256:hash> <prompt> --policy <policy.apl>`

This is explicitly marked experimental and non-DCP in the CLI help. It is a governed external-runtime lane, not the same thing as native RFC-0034 inference completion.

## What Is Complete In The RFC Sense

These layers are substantially implemented and evidenced elsewhere in the repo:

- RFC-0025: policy-gated tensor/model loading
- RFC-0026: phase-1 AI-native inference opcodes
- RFC-0031: deterministic AI execution contract
- RFC-0032: AI promotion/gating path
- RFC-0034: native ternary inference opcode/runtime path
- RFC-0037: T81Lang `std.tnn.*` lowering surface

This means the missing work is not "invent inference." The missing work is the higher-level product lane that turns those pieces into a runnable model architecture and decode UX.

## Missing For True Native Inference

The current repo does not yet provide a complete native `t81 ai` experience equivalent to "load a model and chat with it" through the T81-native path.

Still missing:

- tokenizer integration beyond exact-token / merge-aware normalized-subword seed lookup for at least one supported architecture
- promotion from narrow tokenizer-seeded windows to real prompt tokenization and decode
- architecture-specific forward-pass orchestration beyond the current bounded probe
- promotion of the current bounded candidate-feedback trace into a true decode loop and sampler wired to the native runtime
- promotion of `t81 ai inference run` from a bounded architecture probe to full architecture execution
- real benchmark and policy-test execution derived from that same lane

## Recommended Next Milestone

The smallest honest next milestone is:

1. wire `t81 ai inference run` to one real backend
2. start with `t81_reference_vm`
3. support one narrow architecture first, likely tiny Llama
4. widen tokenizer support from narrow exact-token / whitespace-token seed lookup to real prompt tokenization
5. widen the bounded decode lane into a true state-carrying greedy decode path
6. make `benchmark run` and `policy test` consume the same execution path

That would promote `t81 ai` from "partly scaffolded orchestration shell" to "real native inference entry point."
