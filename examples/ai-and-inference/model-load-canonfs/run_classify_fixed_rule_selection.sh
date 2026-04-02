#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

model_dir="$tmp_root/model"
canon_root="$tmp_root/.t81_canonfs"
model_path="$model_dir/classify-fixed-demo.t81w"
policy_path="$tmp_root/classify-fixed-policy.apl"
result_path="$tmp_root/classify-fixed-result.json"
record_out_path="$tmp_root/classify-rule-record.out.json"
bundle_out_path="$tmp_root/classify-fixed-bundle.out.json"

if [[ ! -x "build/t81_make_classify_fixed_demo" ]]; then
  echo "error: missing classify-fixed demo model builder at build/t81_make_classify_fixed_demo" >&2
  echo "build it first: cmake --build build --target t81_make_classify_fixed_demo" >&2
  exit 1
fi

if [[ ! -x "build/t81" ]]; then
  echo "error: missing build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

mkdir -p "$canon_root"
build/t81_make_classify_fixed_demo "$model_dir" >/dev/null

model_hash="$(build/t81 determinism hash "$model_path" | awk 'NR==1{print $1}')"
cat > "$policy_path" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:${model_hash}"])
  (require-axion-event (reason "task:classify_fixed.v1")))
EOF

input_text="${INPUT_TEXT:-greet hello}"

printf '==========================================================\n'
printf ' T81 AI Classify-Fixed Rule Selection \n'
printf '==========================================================\n'
printf '  ↳ Model: classify-fixed-demo\n'
printf '  ↳ Input: %s\n' "$input_text"
printf '  ↳ Expect: label result retrieved, consumed by a fixed rule-set selection, and persisted as a final bundle.\n'

task_output="$(build/t81 ai task classify-fixed \
  --model classify-fixed-demo \
  --model-file "$model_path" \
  --policy "$policy_path" \
  --canonfs-root "$canon_root" \
  --mode strict_deterministic \
  --input "$input_text")"

printf '%s\n' "$task_output"

result_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"result_ref": "\([^"]*\)".*/\1/p')"
provenance_ref="$(printf '%s\n' "$task_output" | sed -n 's/.*"provenance_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$result_ref" || -z "$provenance_ref" ]]; then
  echo "error: classify-fixed rule-selection demo did not produce result/provenance refs" >&2
  exit 1
fi

build/t81 canonfs get "$result_ref" --canonfs-root "$canon_root" --out "$result_path" --json >/dev/null

label="$(build/t81 ai task read-field "$result_path" --field label)"
termination_reason="$(build/t81 ai task read-field "$result_path" --field termination_reason)"

case "$label" in
  POSITIVE)
    selected_rule_set="positive-default"
    ;;
  NEGATIVE)
    selected_rule_set="negative-default"
    ;;
  *)
    echo "error: unsupported classify-fixed label in stored artifact: $label" >&2
    exit 1
    ;;
esac

rule_set_path="$tmp_root/$selected_rule_set.ruleset"
cat > "$rule_set_path" <<EOF
label=$label
selected_rule_set=$selected_rule_set
source_result_ref=$result_ref
EOF

rule_set_ref="$(build/t81 canonfs put-file "$rule_set_path" --canonfs-root "$canon_root")"

write_store_output="$(build/t81 artifact write-store-record \
  --schema t81.ai.task.classify-fixed.rule-selection-record.v1 \
  --field source_result_ref="$result_ref" \
  --field source_provenance_ref="$provenance_ref" \
  --field label="$label" \
  --field termination_reason="$termination_reason" \
  --field selected_rule_set="$selected_rule_set" \
  --field rule_set_ref="$rule_set_ref" \
  --canonfs-root "$canon_root")"
record_ref="$(printf '%s\n' "$write_store_output" | sed -n 's/.*"record_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$record_ref" ]]; then
  echo "error: artifact write-store-record did not produce record_ref" >&2
  exit 1
fi

build/t81 canonfs get "$record_ref" --canonfs-root "$canon_root" --out "$record_out_path" --json >/dev/null
stored_selected_rule_set="$(build/t81 artifact read-field "$record_ref" \
  --schema t81.ai.task.classify-fixed.rule-selection-record.v1 \
  --field selected_rule_set \
  --canonfs-root "$canon_root")"
stored_rule_set_ref="$(build/t81 artifact read-field "$record_ref" \
  --schema t81.ai.task.classify-fixed.rule-selection-record.v1 \
  --field rule_set_ref \
  --canonfs-root "$canon_root")"

bundle_store_output="$(build/t81 artifact store-bundle \
  --schema t81.ai.task.classify-fixed.bundle.v1 \
  --field source_result_ref="$result_ref" \
  --field source_provenance_ref="$provenance_ref" \
  --field action_ref="$rule_set_ref" \
  --field record_ref="$record_ref" \
  --canonfs-root "$canon_root")"
bundle_ref="$(printf '%s\n' "$bundle_store_output" | sed -n 's/.*"bundle_ref": "\([^"]*\)".*/\1/p')"
if [[ -z "$bundle_ref" ]]; then
  echo "error: artifact store-bundle did not produce bundle_ref" >&2
  exit 1
fi

build/t81 canonfs get "$bundle_ref" --canonfs-root "$canon_root" --out "$bundle_out_path" --json >/dev/null
stored_bundle_record_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema t81.ai.task.classify-fixed.bundle.v1 \
  --field record_ref \
  --canonfs-root "$canon_root")"
stored_bundle_action_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema t81.ai.task.classify-fixed.bundle.v1 \
  --field action_ref \
  --canonfs-root "$canon_root")"

printf '\n---- retrieved AI result artifact ----\n'
cat "$result_path"
printf '\n\n---- selected rule-set artifact ----\n'
cat "$rule_set_path"
printf '\n---- downstream record write-store ----\n'
printf '%s\n' "$write_store_output"
printf '\n---- downstream record typed reads ----\n'
printf 'selected_rule_set=%s\n' "$stored_selected_rule_set"
printf 'rule_set_ref=%s\n' "$stored_rule_set_ref"
printf '\n---- downstream rule-selection record ----\n'
cat "$record_out_path"
printf '\n\n---- final bundle store ----\n'
printf '%s\n' "$bundle_store_output"
printf '\n---- final bundle typed reads ----\n'
printf 'record_ref=%s\n' "$stored_bundle_record_ref"
printf 'action_ref=%s\n' "$stored_bundle_action_ref"
printf '\n---- final bundle artifact ----\n'
cat "$bundle_out_path"
