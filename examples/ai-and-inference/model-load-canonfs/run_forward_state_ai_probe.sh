#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
prompt="${PROMPT:-greet_hello}"
max_tokens="${MAX_TOKENS:-4}"

if [[ ! -x "build/t81" ]]; then
  echo "error: missing t81 binary at build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

if [[ ! -x "build/t81_make_guarded_llama_demo" ]]; then
  echo "error: missing guarded-demo model builder at build/t81_make_guarded_llama_demo" >&2
  echo "build it first: cmake --build build --target t81_make_guarded_llama_demo" >&2
  exit 1
fi

build/t81_make_guarded_llama_demo "$tmp_root" >/dev/null

echo "=========================================================="
echo " T81 AI Forward-State Probe "
echo "=========================================================="
echo "  ↳ Model: forward-state-demo"
echo "  ↳ Prompt: $prompt"
echo "  ↳ Max tokens: $max_tokens"
echo "  ↳ Expect: a bounded decode result followed by full JSON evidence."
echo ""

exec build/t81 ai inference run \
  --model forward-state-demo \
  --model-file "$tmp_root/guarded-llama-demo.t81w" \
  --mode strict_deterministic \
  --prompt "$prompt" \
  --max-tokens "$max_tokens"
