# Llama.cpp Governed Reproducibility Runbook

Status: Active
Last Updated: 2026-02-25

## Purpose

Run a governed `llama.cpp` inference path in T81 with reproducibility checks.

## Scope

This procedure applies to the experimental governed non-DCP LLM path.

## Prerequisites

1. Build with llama support:

```bash
cmake -S . -B build-llama \
  -DT81_ENABLE_LLAMA_CPP=ON \
  -DT81_BUILD_TESTS=ON \
  -DT81_BUILD_BENCHMARKS=OFF \
  -DT81_BUILD_FUZZ_TESTS=OFF
cmake --build build-llama --target t81 -j4
```

2. Have a local GGUF model file.

## Step 1: Canonize Model Bytes (Raw)

```bash
./build-llama/t81 canonize-file /abs/path/model.gguf
```

Output format:

```text
sha3-256:<base81-hash>
```

## Step 2: Create Policy Allowlist

Create `policy.apl`:

```apl
(policy
  (tier 1)
  (allowed-tensor-hashes ["sha3-256:<base81-hash>"]))
```

## Step 3: Run Governed Inference by CanonFS Hash

```bash
./build-llama/t81 llama-run \
  sha3-256:<base81-hash> \
  "Write one short sentence about ternary computing." \
  --policy /abs/path/policy.apl \
  --max-tokens 64 \
  --seed 0 \
  --threads 1 \
  --top-k 1 \
  --top-p 1.0 \
  --temperature 0.0
```

Expected output includes:

- `model_hash:`
- `prompt_hash:`
- `token_ids_csv:`

## Step 4: Capture Reproducibility Baseline

```bash
python3 scripts/ci/llama_cpp_repro_gate.py \
  --t81-bin ./build-llama/t81 \
  --model sha3-256:<base81-hash> \
  --policy /abs/path/policy.apl \
  --prompt "Write one short sentence about ternary computing." \
  --runs 3 \
  --max-tokens 64 \
  --seed 0 \
  --threads 1 \
  --top-k 1 \
  --top-p 1.0 \
  --temperature 0.0 \
  --hash-out build-llama/llama_cpp_repro_hash.txt
```

## Failure Modes

1. `policy denied inference`:
   - model hash is missing from `allowed-tensor-hashes`.
2. `CanonFS model hash not found`:
   - model bytes were not canonized into the selected CanonFS root.
3. `token_ids drift detected`:
   - reproducibility settings/environment changed.

## Versioning Statement

Operational runbook only. DCP scope and specification authority are unchanged.
