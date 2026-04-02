#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/../../.." && pwd)"
cd "$repo_root"

input_text="${INPUT_TEXT:-greet hello}"
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"
model_dir="$tmp_root/model"
model_path="$model_dir/answer-fixed-demo.t81w"
policy_path="$tmp_root/answer-fixed-policy.apl"

if [[ ! -x "build/t81_make_answer_fixed_demo" ]]; then
  echo "error: missing answer-fixed demo model builder at build/t81_make_answer_fixed_demo" >&2
  echo "build it first: cmake --build build --target t81_make_answer_fixed_demo" >&2
  exit 1
fi

if [[ ! -x "build/t81" ]]; then
  echo "error: missing CLI binary at build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

build/t81_make_answer_fixed_demo "$model_dir" >/dev/null
model_hash="$(build/t81 determinism hash "$model_path" | awk '{print $1}')"

cat > "$policy_path" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:$model_hash"])
  (require-axion-event (reason "task:answer_fixed.v1")))
EOF

echo "=========================================================="
echo " T81 AI Answer-Fixed Task "
echo "=========================================================="
echo "  ↳ Model: answer-fixed-demo"
echo "  ↳ Input: $input_text"
echo "  ↳ Expect: a canonical answer artifact plus provenance refs."

task_output="$(build/t81 ai task answer-fixed \
  --model answer-fixed-demo \
  --model-file "$model_path" \
  --policy "$policy_path" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic \
  --input "$input_text")"

printf '%s\n' "$task_output"

result_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"result_ref": "\([^"]*\)".*/\1/p')"
if [[ -n "$result_ref" ]]; then
  result_path="$tmp_root/result-artifact.json"
  echo
  echo "---- stored result artifact ----"
  build/t81 canonfs get "$result_ref" --canonfs-root "$canon_root" --out "$result_path" --json >/dev/null
  cat "$result_path"
fi
