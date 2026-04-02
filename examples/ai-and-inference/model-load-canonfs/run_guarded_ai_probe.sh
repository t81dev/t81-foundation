#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

t81_bin="${T81_BIN:-$repo_root/build/t81}"
hf_bin="${HF_BIN:-$HOME/Library/Python/3.14/bin/hf}"
model_dir="${MODEL_DIR:-$repo_root/models/tiny-random-llama}"
model_safetensors="$model_dir/model.safetensors"
prompt="${PROMPT:-hello world}"
max_tokens="${MAX_TOKENS:-3}"

if [[ ! -x "$t81_bin" ]]; then
  echo "error: missing t81 binary at $t81_bin" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

mkdir -p "$model_dir"

if [[ ! -f "$model_safetensors" ]]; then
  if [[ ! -x "$hf_bin" ]]; then
    echo "error: tiny model not present and hf CLI not found at $hf_bin" >&2
    exit 1
  fi
  "$hf_bin" download \
    hf-internal-testing/tiny-random-LlamaForCausalLM \
    config.json tokenizer.json model.safetensors \
    --local-dir "$model_dir"
fi

echo "=========================================================="
echo " T81 AI Guarded Probe "
echo "=========================================================="
echo "  ↳ Model: tiny-random-llama"
echo "  ↳ Prompt: $prompt"
echo "  ↳ Max tokens: $max_tokens"
echo "  ↳ Expect: guarded inference output followed by full JSON evidence."
echo ""

exec "$t81_bin" ai inference run \
  --model tiny-random-llama \
  --model-file "$model_safetensors" \
  --mode strict_deterministic \
  --prompt "$prompt" \
  --max-tokens "$max_tokens"
