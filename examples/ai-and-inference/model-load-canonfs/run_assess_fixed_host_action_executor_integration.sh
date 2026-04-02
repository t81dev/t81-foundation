#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

model_dir="$tmp_root/model"
canon_root="$tmp_root/.t81_canonfs"
model_path="$model_dir/assess-fixed-demo.t81w"
policy_path="$tmp_root/assess-fixed-policy.apl"
result_path="$tmp_root/assess-fixed-result.json"
bundle_path="$tmp_root/assess-fixed-bundle.json"
record_path="$tmp_root/assess-fixed-record.json"
action_artifact_path="$tmp_root/assess-fixed-action-artifact.txt"
action_stage_dir="$tmp_root/action-stage"
executor_root="$tmp_root/executor-output"

if [[ ! -x "build/t81_make_assess_fixed_demo" ]]; then
  echo "error: missing assess-fixed demo model builder at build/t81_make_assess_fixed_demo" >&2
  echo "build it first: cmake --build build --target t81_make_assess_fixed_demo" >&2
  exit 1
fi

if [[ ! -x "build/t81" ]]; then
  echo "error: missing build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

mkdir -p "$canon_root" "$action_stage_dir" "$executor_root"
build/t81_make_assess_fixed_demo "$model_dir" >/dev/null

model_hash="$(build/t81 determinism hash "$model_path" | awk 'NR==1{print $1}')"
cat > "$policy_path" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:${model_hash}"])
  (require-axion-event (reason "task:assess_fixed.v1")))
EOF

input_text="${INPUT_TEXT:-greet hello}"

task_output="$(build/t81 ai task assess-fixed \
  --model assess-fixed-demo \
  --model-file "$model_path" \
  --policy "$policy_path" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic \
  --input "$input_text")"

result_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"result_ref": "\([^"]*\)".*/\1/p')"
provenance_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"provenance_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$result_ref" || -z "$provenance_ref" ]]; then
  echo "error: assess-fixed integration demo did not produce result/provenance refs" >&2
  exit 1
fi

build/t81 canonfs get "$result_ref" --canonfs-root "$canon_root" --out "$result_path" --json >/dev/null

decision="$(build/t81 ai task read-field "$result_path" --field decision)"
reason_code="$(build/t81 ai task read-field "$result_path" --field reason_code)"
termination_reason="$(build/t81 ai task read-field "$result_path" --field termination_reason)"

case "$decision" in
  ALLOW)
    action_name="write_allow_marker"
    action_rel_path="actions/allow.marker"
    ;;
  DENY)
    action_name="write_deny_marker"
    action_rel_path="actions/deny.marker"
    ;;
  REVIEW)
    action_name="write_review_marker"
    action_rel_path="actions/review.marker"
    ;;
  *)
    echo "error: unsupported decision in stored artifact: $decision" >&2
    exit 1
    ;;
esac

action_source_path="$action_stage_dir/${action_name}.txt"
cat > "$action_source_path" <<EOF
decision=$decision
reason_code=$reason_code
source_result_ref=$result_ref
EOF

action_ref="$(build/t81 canonfs put-file "$action_source_path" --canonfs-root "$canon_root")"

record_store_output="$(build/t81 artifact write-store-record \
  --schema t81.ai.task.assess-fixed.host-action-record.v1 \
  --field source_result_ref="$result_ref" \
  --field source_provenance_ref="$provenance_ref" \
  --field decision="$decision" \
  --field reason_code="$reason_code" \
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
  --schema t81.ai.task.assess-fixed.bundle.v1 \
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
if [[ "$bundle_schema" != "t81.ai.task.assess-fixed.bundle.v1" ]]; then
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
build/t81 canonfs get "$bundle_action_ref" --canonfs-root "$canon_root" --out "$action_artifact_path" >/dev/null

selected_action="$(build/t81 artifact read-field "$bundle_record_ref" \
  --schema t81.ai.task.assess-fixed.host-action-record.v1 \
  --field selected_action \
  --canonfs-root "$canon_root")"
selected_path="$(build/t81 artifact read-field "$bundle_record_ref" \
  --schema t81.ai.task.assess-fixed.host-action-record.v1 \
  --field selected_path \
  --canonfs-root "$canon_root")"

executor_target="$executor_root/$selected_path"
mkdir -p "$(dirname "$executor_target")"
cat > "$executor_target" <<EOF
bundle_ref=$bundle_ref
record_ref=$bundle_record_ref
action_ref=$bundle_action_ref
selected_action=$selected_action
selected_path=$selected_path
decision=$decision
reason_code=$reason_code
termination_reason=$termination_reason
EOF

printf '==========================================================\n'
printf ' T81 Bundle -> Host Action Executor Integration \n'
printf '==========================================================\n'
printf 'bundle_ref=%s\n' "$bundle_ref"
printf 'bundle_schema=%s\n' "$bundle_schema"
printf 'executor_target=%s\n' "$executor_target"
printf '\n---- canonical bundle artifact ----\n'
cat "$bundle_path"
printf '\n\n---- typed downstream record ----\n'
cat "$record_path"
printf '\n\n---- canonical action artifact ----\n'
cat "$action_artifact_path"
printf '\n---- executor materialized output ----\n'
cat "$executor_target"
