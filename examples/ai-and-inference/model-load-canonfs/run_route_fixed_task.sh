#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

model_dir="$tmp_root/model"
canon_root="$tmp_root/.t81_canonfs"
model_path="$model_dir/route-fixed-demo.t81w"
policy_path="$tmp_root/route-fixed-policy.apl"
result_path="$tmp_root/route-fixed-result.json"

if [[ ! -x "build/t81_make_route_fixed_demo" ]]; then
  echo "error: missing route-fixed demo model builder at build/t81_make_route_fixed_demo" >&2
  echo "build it first: cmake --build build --target t81_make_route_fixed_demo" >&2
  exit 1
fi

if [[ ! -x "build/t81" ]]; then
  echo "error: missing build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

mkdir -p "$canon_root"
build/t81_make_route_fixed_demo "$model_dir" >/dev/null

model_hash="$(build/t81 determinism hash "$model_path" | awk 'NR==1{print $1}')"
cat > "$policy_path" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:${model_hash}"])
  (require-axion-event (reason "task:route_fixed.v1")))
EOF

input_text="${INPUT_TEXT:-greet hello}"

printf '==========================================================\n'
printf ' T81 AI Route-Fixed Task \n'
printf '==========================================================\n'
printf '  ↳ Model: route-fixed-demo\n'
printf '  ↳ Input: %s\n' "$input_text"
printf '  ↳ Expect: a canonical route artifact plus provenance refs.\n'

task_output="$(build/t81 ai task route-fixed \
  --model route-fixed-demo \
  --model-file "$model_path" \
  --policy "$policy_path" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic \
  --input "$input_text")"

printf '%s\n' "$task_output"

result_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"result_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$result_ref" ]]; then
  echo "error: route-fixed demo did not produce result_ref" >&2
  exit 1
fi

build/t81 canonfs get "$result_ref" --canonfs-root "$canon_root" --out "$result_path" --json >/dev/null

printf '\n---- stored result artifact ----\n'
cat "$result_path"
