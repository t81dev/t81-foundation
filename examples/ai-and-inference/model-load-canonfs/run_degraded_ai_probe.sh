#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
prompt="${PROMPT:-greet hello}"
max_tokens="${MAX_TOKENS:-2}"

build/t81_make_degraded_llama_demo "$tmp_root" >/dev/null

exec build/t81 ai inference run \
  --model degraded-demo \
  --model-file "$tmp_root/degraded-llama-demo.t81w" \
  --mode strict_deterministic \
  --prompt "$prompt" \
  --max-tokens "$max_tokens"
