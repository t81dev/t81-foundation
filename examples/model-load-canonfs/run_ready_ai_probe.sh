#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

model_path="${MODEL_PATH:-/tmp/t81-ready-demo.t81w}"
prompt="${PROMPT:-hello}"

build/t81_make_demo_model "$model_path" >/dev/null

exec build/t81 ai inference run \
  --model ready-demo \
  --model-file "$model_path" \
  --mode strict_deterministic \
  --prompt "$prompt"
