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
record_path="$tmp_root/route-fixed-record.json"
bundle_path="$tmp_root/route-fixed-bundle.json"

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

task_output="$(build/t81 ai task route-fixed \
  --model route-fixed-demo \
  --model-file "$model_path" \
  --policy "$policy_path" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic \
  --input "$input_text")"

result_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"result_ref": "\([^"]*\)".*/\1/p')"
provenance_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"provenance_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$result_ref" || -z "$provenance_ref" ]]; then
  echo "error: route-fixed bundle-consumer demo did not produce result/provenance refs" >&2
  exit 1
fi

build/t81 canonfs get "$result_ref" --canonfs-root "$canon_root" --out "$result_path" --json >/dev/null

route="$(build/t81 ai task read-field "$result_path" --field route)"
termination_reason="$(build/t81 ai task read-field "$result_path" --field termination_reason)"

case "$route" in
  A)
    action_name="write_route_a_target"
    action_rel_path="routes/a.target"
    ;;
  B)
    action_name="write_route_b_target"
    action_rel_path="routes/b.target"
    ;;
  C)
    action_name="write_route_c_target"
    action_rel_path="routes/c.target"
    ;;
  *)
    echo "error: unsupported route in stored artifact: $route" >&2
    exit 1
    ;;
esac

action_path="$tmp_root/$action_rel_path"
mkdir -p "$(dirname "$action_path")"
cat > "$action_path" <<EOF
route=$route
selected_path=$action_rel_path
source_result_ref=$result_ref
EOF

action_ref="$(build/t81 canonfs put-file "$action_path" --canonfs-root "$canon_root")"

record_store_output="$(build/t81 artifact write-store-record \
  --schema t81.ai.task.route-fixed.path-selection-record.v1 \
  --field source_result_ref="$result_ref" \
  --field source_provenance_ref="$provenance_ref" \
  --field route="$route" \
  --field termination_reason="$termination_reason" \
  --field selected_action="$action_name" \
  --field selected_path="$action_rel_path" \
  --field action_ref="$action_ref" \
  --canonfs-root "$canon_root")"
record_ref="$(printf '%s\n' "$record_store_output" | sed -n 's/.*"record_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$record_ref" ]]; then
  echo "error: artifact write-store-record did not produce record_ref" >&2
  exit 1
fi

bundle_store_output="$(build/t81 artifact store-bundle \
  --schema t81.ai.task.route-fixed.bundle.v1 \
  --field source_result_ref="$result_ref" \
  --field source_provenance_ref="$provenance_ref" \
  --field action_ref="$action_ref" \
  --field record_ref="$record_ref" \
  --canonfs-root "$canon_root")"
bundle_ref="$(printf '%s\n' "$bundle_store_output" | sed -n 's/.*"bundle_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$bundle_ref" ]]; then
  echo "error: artifact store-bundle did not produce bundle_ref" >&2
  exit 1
fi

build/t81 canonfs get "$bundle_ref" --canonfs-root "$canon_root" --out "$bundle_path" --json >/dev/null

bundle_schema="$(sed -n 's/.*"schema": "\([^"]*\)".*/\1/p' "$bundle_path" | head -n 1)"
if [[ "$bundle_schema" != "t81.ai.task.route-fixed.bundle.v1" ]]; then
  echo "error: unexpected bundle schema: $bundle_schema" >&2
  exit 1
fi

bundle_record_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field record_ref \
  --canonfs-root "$canon_root")"
bundle_action_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field action_ref \
  --canonfs-root "$canon_root")"

build/t81 canonfs get "$bundle_record_ref" --canonfs-root "$canon_root" --out "$record_path" --json >/dev/null

selected_action="$(build/t81 artifact read-field "$bundle_record_ref" \
  --schema t81.ai.task.route-fixed.path-selection-record.v1 \
  --field selected_action \
  --canonfs-root "$canon_root")"
selected_path="$(build/t81 artifact read-field "$bundle_record_ref" \
  --schema t81.ai.task.route-fixed.path-selection-record.v1 \
  --field selected_path \
  --canonfs-root "$canon_root")"

printf '==========================================================\n'
printf ' T81 Bundle Consumer Example \n'
printf '==========================================================\n'
printf 'bundle_ref=%s\n' "$bundle_ref"
printf 'bundle_schema=%s\n' "$bundle_schema"
printf '\n---- canonical bundle artifact ----\n'
cat "$bundle_path"
printf '\n\n---- consumer-facing typed reads ----\n'
printf 'record_ref=%s\n' "$bundle_record_ref"
printf 'action_ref=%s\n' "$bundle_action_ref"
printf 'selected_action=%s\n' "$selected_action"
printf 'selected_path=%s\n' "$selected_path"
printf '\n---- downstream record artifact ----\n'
cat "$record_path"
