# Model Artifact Review Walkthrough

This example shows the current narrow model-artifact intake wedge:

- import a model artifact into a reviewable JSON report
- persist a narrower manifest for storage or later comparison
- compare raw imported representations honestly
- compare a known `.gguf` / `safetensors` pair with explicit normalized rules

This is a review workflow, not a general execution workflow.

## Quick Run

If you want the smallest smoke path for this walkthrough, run:

```bash
bash examples/ai-and-inference/model-artifact-review/run_model_artifact_review.sh
```

That script:

- imports the checked-in exported `safetensors` fixture
- writes a manifest
- shows a raw representation-sensitive diff
- quantizes the source fixture to `.gguf`
- shows the opt-in normalized diff for the known pair

## Inputs

Checked-in source fixture:

- `models/tiny-random-llama/model.safetensors`

Checked-in native-ternary export fixture:

- `models/tiny-random-llama-exported.safetensors`

Temporary `.gguf` produced by the current supported path:

```bash
./build/t81 weights quantize models/tiny-random-llama/model.safetensors --to-gguf /tmp/t81-tiny-random-llama.gguf
```

## 1. Import A SafeTensors Artifact

Emit the review-oriented import record:

```bash
./build/t81 model import models/tiny-random-llama-exported.safetensors --json
```

Persist the narrower manifest:

```bash
./build/t81 model import models/tiny-random-llama-exported.safetensors --json --manifest /tmp/t81-exported.manifest.json
```

What to look for:

- `schema: "t81.model-import.v1"`
- `source_format: "safetensors"`
- `artifact_format`
- artifact provenance such as `source_sha3_512`
- tensor inventory with `name`, `shape`, and `trits`

The manifest intentionally keeps less:

- `schema: "t81.model-manifest.v1"`
- `normalized_artifact_type: "model-tensor-manifest"`
- explicit provenance keys used for later review and diff

## 2. Raw Diff Of Two SafeTensors Artifacts

Compare the exported native-ternary artifact to the float-backed source:

```bash
./build/t81 model diff \
  models/tiny-random-llama-exported.safetensors \
  models/tiny-random-llama/model.safetensors \
  --json
```

Expected outcome:

- `schema: "t81.model-diff.v1"`
- `identical: false`
- non-empty `changed`
- provenance review context in:
  - `provenance_lhs_only`
  - `provenance_rhs_only`
  - `provenance_changed`

This is representation-sensitive by design.

## 3. Manifest Versus Live Artifact

Compare the persisted manifest to the live exported artifact:

```bash
./build/t81 model diff \
  /tmp/t81-exported.manifest.json \
  models/tiny-random-llama-exported.safetensors \
  --json
```

Expected outcome:

- `identical: true`
- empty tensor difference arrays
- provenance-key differences may still appear

This is useful because the manifest is intentionally narrower than the live
import record.

## 4. Normalized `.gguf` Versus SafeTensors

First create the `.gguf`:

```bash
./build/t81 weights quantize models/tiny-random-llama/model.safetensors --to-gguf /tmp/t81-tiny-random-llama.gguf
```

Raw diff stays representation-sensitive:

```bash
./build/t81 model diff \
  /tmp/t81-tiny-random-llama.gguf \
  models/tiny-random-llama/model.safetensors \
  --json
```

Expected raw outcome:

- `identical: false`
- many tensors in `changed`

Normalized diff is explicit and opt-in:

```bash
./build/t81 model diff \
  /tmp/t81-tiny-random-llama.gguf \
  models/tiny-random-llama/model.safetensors \
  --json \
  --mode normalized
```

Expected normalized outcome:

- `schema: "t81.model-diff-normalized.v1"`
- `comparison_mode: "normalized"`
- `identical: true` for this known pair
- `normalized_matches`
- `normalization_rules`
- `normalized_match_reasons`
- provenance-key review context in the same `provenance_*` arrays

Current admitted normalization rule:

- `known_2d_transpose_for_gguf_safetensors`

## 5. Why This Example Exists

This walkthrough is the current best proof that the model-artifact lane is
real without broadening into a larger runtime story.

It demonstrates:

- reviewable intake
- explicit persisted records
- honest raw comparison
- narrow normalization with named rules
- provenance surfaced as first-class review context

## Related Contracts

- `docs/contracts/MODEL_IMPORT_V1_CONTRACT.md`
- `docs/contracts/MODEL_MANIFEST_V1_CONTRACT.md`
- `docs/contracts/MODEL_DIFF_V1_CONTRACT.md`
- `docs/contracts/MODEL_DIFF_NORMALIZED_V1_CONTRACT.md`
