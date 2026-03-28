#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
prompt="${PROMPT:-greet_hello}"
max_tokens="${MAX_TOKENS:-2}"

build/t81_make_guarded_llama_demo "$tmp_root" >/dev/null

exec build/t81 ai inference run \
  --model forward-state-demo \
  --model-file "$tmp_root/guarded-llama-demo.t81w" \
  --mode strict_deterministic \
  --prompt "$prompt" \
  --max-tokens "$max_tokens"
