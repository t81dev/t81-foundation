#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

model_path="${MODEL_PATH:-/tmp/t81-ready-demo.t81w}"
prompt="${PROMPT:-hello}"

if [[ ! -x "build/t81" ]]; then
  echo "error: missing t81 binary at build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

if [[ ! -x "build/t81_make_demo_model" ]]; then
  echo "error: missing ready-demo model builder at build/t81_make_demo_model" >&2
  echo "build it first: cmake --build build --target t81_make_demo_model" >&2
  exit 1
fi

build/t81_make_demo_model "$model_path" >/dev/null

echo "=========================================================="
echo " T81 AI Ready Probe "
echo "=========================================================="
echo "  ↳ Model: ready-demo"
echo "  ↳ Prompt: $prompt"
echo "  ↳ Expect: a single native probe result followed by full JSON evidence."
echo ""

exec build/t81 ai inference run \
  --model ready-demo \
  --model-file "$model_path" \
  --mode strict_deterministic \
  --prompt "$prompt"
