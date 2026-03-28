# CanonFS Model Load Example

This is the smallest contributor-facing example of the CanonFS-backed
`--weights-model` lane.

It shows how to:

- create a tiny `.t81w` model fixture
- create a tiny SafeTensors source artifact
- convert that source artifact with `t81 weights import`
- create a tiny float SafeTensors source artifact for T3_K GGUF
- download and import a real tiny Hugging Face model
- store it in CanonFS
- run a program that loads model tensors by name
- verify the allow and deny policy paths

## Build

From the repo root:

```bash
cmake -S . -B build -G Ninja -DT81_BUILD_EXAMPLES=ON
cmake --build build --target t81 t81_make_demo_model t81_make_guarded_llama_demo t81_make_degraded_llama_demo t81_make_demo_safetensors t81_make_demo_float_safetensors
```

## Files

- `make_demo_model.cpp` builds a tiny `.t81w` containing `mat_a` and `mat_b`
- `make_demo_safetensors.cpp` builds a tiny `.safetensors` source file with the
  same tensor names
- `make_demo_float_safetensors.cpp` builds a tiny float SafeTensors source file
  for the T3_K GGUF quantize/import lane
- [03_matmul_weights.t81](/Users/t81dev/Code/t81-foundation/tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81)
  loads those tensors via `std.tensor.load(...)`

## Healthy AI Probe Path

This is the smallest healthy `t81 ai inference run` lane in the repo. It does
not exercise the bounded logits/decode path, so it stays in a simple `ready`
posture instead of falling into degraded evidence shaping.

```bash
tmp_root="$(mktemp -d)"
model_path="$tmp_root/ready-demo.t81w"

build/t81_make_demo_model "$model_path"
build/t81 ai inference run \
  --model ready-demo \
  --model-file "$model_path" \
  --mode strict_deterministic \
  --prompt hello
```

Expected result:

- `status: "pass"`
- `readiness.kind: "ready"`
- `artifact_visibility.kind: "ready"`
- `termination_reason: "no_logits_row_probe"`
- `output: "<tensor#1>"`

The same path is also wrapped in:

```bash
bash examples/model-load-canonfs/run_ready_ai_probe.sh
```

## Guarded AI Probe Path

This is the checked-in `guarded` example. It uses the real tiny Hugging Face
Llama artifact and lands in the guarded envelope: bounded decode stays weak,
but it does not exhaust into degraded mode.

```bash
bash examples/model-load-canonfs/run_guarded_ai_probe.sh
```

Expected result:

- `status: "pass"`
- `bounded_decode_health.kind: "guarded"`
- `readiness.kind: "guarded"`
- `decode_trace_policy: "full_trace_with_guarded_caution.v1"`
- `output_policy: "verbatim_with_guarded_caution.v1"`
- `weak_steps: [2]`
- `termination_reason: "max_tokens_reached"`

## Forward-State AI Probe Path

This is the smallest checked-in multi-step forward-state example. It runs the
same tiny synthetic Llama-shaped model for four decode steps and now proves a
bounded carried hidden-tensor path, not just row-derived forward-state
summaries. Later decode steps reimport a bounded hidden tensor through the
compiled tensor-pool path, expose bounded q/k signature state, and now carry a
combined bounded architecture-state object through the `ready` envelope.
The fourth decode step now takes a deeper architecture-state-led control path
instead of reusing the same mode as step three.
When that happens, the top-level readiness/health envelope upgrades to
`bounded_deep_architecture_state_probe.v1`.
At the current bounded 4-step ceiling, the run now ends with
`termination_reason: "deep_architecture_state_horizon_reached"`.

```bash
MAX_TOKENS=4 bash examples/model-load-canonfs/run_forward_state_ai_probe.sh
```

Expected result:

- `status: "pass"`
- `readiness.kind: "ready"`
- `generated_tokens: 4`
- `true_state_carry_supported: true`
- `state_carry_limitations.kind: "bounded_intermediate_tensor_literal_import.v1"`
- `intermediate_tensor_import_used: true`
- `intermediate_tensor_blend_used: true`
- `architecture_state_summary.kind: "bounded_hidden_tensor_qk_forward_state.v1"`
- `final_architecture_state_signature_sha256: ...`
- `architecture_state_summary.max_confidence_score: ...`
- `architecture_state_summary.feedback_steps: ...`
- later steps and `final_decode_state` now expose
  `architecture_state_confidence_score` and
  `architecture_state_stability_kind`
- top-level `bounded_decode_health` / `readiness` switch to
  `confidence_envelope: "bounded_architecture_state_probe.v1"` for this lane
- top-level `artifact_visibility` now carries the same
  `confidence_envelope`, `architecture_state_stability_kind`, and
  `architecture_state_guardrail_triggered` fields
- later decode enters `architecture_state_feedback_state_transition.v1`
- `hidden_tensor_carry_mode_kind: "evolved_hidden_tensor_feedback.v1"`
- `kv_state_summary.kind: "bounded_qk_tensor_state.v1"`
- `final_kv_state_signature_sha256` is present
- `kv_state_summary.feedback_steps: ...`
- `hidden_tensor_summary.feedback_steps: ...`
- architecture-state stability now also participates in bounded recovery
  decisions
- later carried hidden-tensor evolution reports
  `hidden_tensor_carry_mode_kind:
  "architecture_state_evolved_hidden_tensor_feedback.v1"`
- later carried Q/K state reports
  `kv_state_carry_mode_kind:
  "architecture_state_evolved_qk_signature.v1"`
- later steps can report
  `transition_kind: "hidden_tensor_feedback_state_transition.v1"`
- `hidden_tensor_import_used: true` on later decode steps
- `forward_state_generation: 2`
- `consumed_state_input_row_ids` is present on later decode steps
- `forward_state_kind: "projection_carried_forward_state.v1"`
- `forward_state_summary.kind: "evolving_projection_forward_state.v1"`

## Degraded AI Probe Path

This is the smallest one-command `degraded` rerun path. It uses a checked-in
synthetic Llama-shaped helper with an intentionally undersized embedding table,
so the decode probe becomes unavailable and the lane drops into the conservative
degraded posture.

```bash
bash examples/model-load-canonfs/run_degraded_ai_probe.sh
```

Expected result:

- `status: "degraded"`
- `readiness.kind: "degraded"`
- `artifact_visibility.kind: "degraded"`
- `output_policy: "suppressed_on_degraded.v1"`
- `termination_reason: "decode_probe_unavailable"`

## Real Weights Import Path

This is the smallest source-format example in the repo. It uses a generated
SafeTensors file, converts it to `.t81w` through the real CLI, and then follows
the same CanonFS-backed model load flow as the section below.

```bash
tmp_root="$(mktemp -d)"
source_path="$tmp_root/demo-model.safetensors"
model_path="$tmp_root/demo-model.t81w"

build/t81_make_demo_safetensors "$source_path"
build/t81 weights import "$source_path" -o "$model_path"
```

Expected result:

- the helper reports `tensors=mat_a,mat_b`
- `t81 weights import` prints model info and saves a `.t81w`

After that, continue with the CanonFS flow below using `model_path="$model_path"`.

## Tiny T3_K GGUF Path

This is the smallest in-repo happy path for the GGUF lane.

```bash
tmp_root="$(mktemp -d)"
float_source="$tmp_root/demo-model-f32.safetensors"
gguf_path="$tmp_root/demo-model.t3k.gguf"
model_path="$tmp_root/demo-model-from-gguf.t81w"

build/t81_make_demo_float_safetensors "$float_source"
build/t81 weights quantize "$float_source" --to-gguf "$gguf_path"
build/t81 weights import "$gguf_path" --format gguf -o "$model_path"
```

Expected result:

- the helper reports `arch=llama`
- `t81 weights quantize` reports `Success! T3_K GGUF created`
- `t81 weights import` prints GGUF model info and saves a `.t81w`

This lane avoids the bridge-gated non-`T3_K` GGUF path by generating a tiny
native `T3_K` GGUF in-repo.

## Run

From the repo root:

```bash
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"
model_path="$tmp_root/demo-model.t81w"
allow_policy="$tmp_root/allow.apl"
deny_policy="$tmp_root/deny.apl"

mkdir -p "$canon_root"

helper_output="$(build/t81_make_demo_model "$model_path")"
printf '%s\n' "$helper_output"
model_checksum="$(printf '%s\n' "$helper_output" | sed -n 's/^sha3-512=//p')"
model_hash="$(build/t81 canonfs put-file "$model_path" --canonfs-root "$canon_root")"
model_hash="${model_hash//$'\n'/}"
```

Use the emitted `sha3-512=` checksum in the allow policy:

```bash
cat > "$allow_policy" <<'EOF'
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:MODEL_CHECKSUM"]))
EOF

perl -0pi -e "s/MODEL_CHECKSUM/$model_checksum/g" "$allow_policy"

cat > "$deny_policy" <<'EOF'
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:cafebabe"]))
EOF
```

Now run the CanonFS-backed model load path:

```bash
export T81_CANONFS_ROOT="$canon_root"

build/t81 code run \
  tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 \
  --weights-model "$model_hash"
```

Expected result:

- compilation succeeds
- output includes `<tensor#1>`
- the program terminates normally

Allow path:

```bash
build/t81 code run \
  tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 \
  --weights-model "$model_hash" \
  --policy "$allow_policy"
```

Expected result:

- output includes `<tensor#1>`
- the program terminates normally

Deny path:

```bash
build/t81 code run \
  tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 \
  --weights-model "$model_hash" \
  --policy "$deny_policy"
```

Expected result:

- execution traps with `SecurityFault`
- no `<tensor#1>` is printed

## Real Hugging Face Tiny Model Path

This is the smallest real external model flow currently validated in the repo.

One-command runner:

```bash
examples/model-load-canonfs/run_real_hf_tiny_model.sh
```

The script reuses the existing tiny model under `models/tiny-random-llama/` if
present and only falls back to `hf download` when the files are missing.

Prerequisite:

- Hugging Face CLI installed as `hf`
  If it is not on `PATH`, use:
  `/Users/t81dev/Library/Python/3.14/bin/hf`

Download the model:

```bash
mkdir -p models

/Users/t81dev/Library/Python/3.14/bin/hf download \
  hf-internal-testing/tiny-random-LlamaForCausalLM \
  config.json tokenizer.json model.safetensors \
  --local-dir models/tiny-random-llama
```

Import it into `.t81w`:

```bash
build/t81 weights import \
  models/tiny-random-llama/model.safetensors \
  -o /tmp/tiny-random-llama.t81w
```

Expected result:

- the import succeeds
- output reports `Model contains 21 tensors`
- output format is `SafeTensors(float-quantized; profile=native-dense-v1)`

Register it in CanonFS and run a real tensor operation under policy:

```bash
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"
program_path="$tmp_root/matmul_real_tensor.t81"
allow_policy="$tmp_root/allow.apl"
deny_policy="$tmp_root/deny.apl"

mkdir -p "$canon_root"

model_hash="$(build/t81 canonfs put-file /tmp/tiny-random-llama.t81w --canonfs-root "$canon_root")"
model_hash="${model_hash//$'\n'/}"
model_checksum="$(build/t81 weights info /tmp/tiny-random-llama.t81w --json | python3 -c 'import sys,json; print(json.load(sys.stdin)[\"checksum_sha3_512\"])')"

cat > "$program_path" <<'EOF'
fn main() -> i32 {
  let q: i32 = std.tensor.load("model.layers.0.self_attn.q_proj.weight");
  let k: i32 = std.tensor.load("model.layers.0.self_attn.k_proj.weight");
  let out: Tensor = std.tensor.matmul(q, k);
  let _ = out;
  print(q);
  return 0;
}
EOF

cat > "$allow_policy" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:$model_checksum"]))
EOF

cat > "$deny_policy" <<'EOF'
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:cafebabe"]))
EOF

export T81_CANONFS_ROOT="$canon_root"
```

Allow path:

```bash
build/t81 code run \
  "$program_path" \
  --weights-model "$model_hash" \
  --policy "$allow_policy"
```

Expected result:

- output includes `<tensor#1>`
- the program terminates normally

Deny path:

```bash
build/t81 code run \
  "$program_path" \
  --weights-model "$model_hash" \
  --policy "$deny_policy"
```

Expected result:

- execution traps with `SecurityFault`
- exit code is non-zero

## Notes

- This example proves the current accepted RFC-0025 / RFC-00D1-adjacent lane:
  CanonFS-backed model identity plus policy-gated loading.
- It is intentionally tiny. It now covers both:
  a synthetic `.t81w` helper path and a real `t81 weights import` path from a
  generated SafeTensors source file.
- It also covers a native `T3_K` GGUF path via
  `weights quantize ... --to-gguf` followed by `weights import --format gguf`.
- It now also covers a real downloaded Hugging Face SafeTensors model through
  CanonFS registration and governed execution.
- The next larger step after this example is:
  importing a real GGUF or larger SafeTensors artifact, followed by the same
  `canonfs put-file` and `code run --weights-model <sha3-256:...>` path.
