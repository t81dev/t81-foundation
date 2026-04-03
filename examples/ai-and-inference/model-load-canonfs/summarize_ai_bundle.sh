#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

usage() {
  echo "usage: bash examples/ai-and-inference/model-load-canonfs/summarize_ai_bundle.sh <bundle_ref> <canonfs_root>" >&2
  echo "summarizes one admitted-family AI bundle without changing CanonFS state" >&2
}

if [[ $# -ne 2 ]]; then
  usage
  exit 1
fi

if [[ ! -x "build/t81" ]]; then
  echo "error: missing build/t81" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

bundle_ref="$1"
canon_root="$2"

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

bundle_path="$tmp_root/bundle.json"
record_path="$tmp_root/record.json"

# Add error handling for bundle retrieval
if ! build/t81 canonfs get "$bundle_ref" --canonfs-root "$canon_root" --out "$bundle_path" --json 2>/dev/null; then
  echo "error: failed to retrieve bundle $bundle_ref" >&2
  exit 1
fi

bundle_schema="$(sed -n 's/.*"schema": "\([^"]*\)".*/\1/p' "$bundle_path" | head -n 1)"

case "$bundle_schema" in
  t81.ai.task.assess-fixed.bundle.v1)
    family="assess-fixed"
    expected_record_schema="t81.ai.task.assess-fixed.host-action-record.v1"
    primary_field="selected_action"
    secondary_field="selected_path"
    ;;
  t81.ai.task.route-fixed.bundle.v1)
    family="route-fixed"
    expected_record_schema="t81.ai.task.route-fixed.path-selection-record.v1"
    primary_field="selected_action"
    secondary_field="selected_path"
    ;;
  t81.ai.task.classify-fixed.bundle.v1)
    family="classify-fixed"
    expected_record_schema="t81.ai.task.classify-fixed.rule-selection-record.v1"
    primary_field="selected_rule_set"
    secondary_field="rule_set_ref"
    ;;
  *)
    echo "error: unsupported bundle schema: $bundle_schema" >&2
    exit 1
    ;;
esac

source_result_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field source_result_ref \
  --canonfs-root "$canon_root")"
source_provenance_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field source_provenance_ref \
  --canonfs-root "$canon_root")"
record_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field record_ref \
  --canonfs-root "$canon_root")"
action_ref="$(build/t81 artifact read-field "$bundle_ref" \
  --schema "$bundle_schema" \
  --field action_ref \
  --canonfs-root "$canon_root")"

# Add error handling for record retrieval
if ! build/t81 canonfs get "$record_ref" --canonfs-root "$canon_root" --out "$record_path" --json 2>/dev/null; then
  echo "error: failed to retrieve record $record_ref" >&2
  exit 1
fi

record_schema="$(sed -n 's/.*"schema": "\([^"]*\)".*/\1/p' "$record_path" | head -n 1)"
if [[ "$record_schema" != "$expected_record_schema" ]]; then
  echo "error: unexpected record schema: $record_schema" >&2
  echo "expected: $expected_record_schema" >&2
  exit 1
fi

# Add error handling for field extraction
if ! primary_value="$(build/t81 artifact read-field "$record_ref" \
  --schema "$record_schema" \
  --field "$primary_field" \
  --canonfs-root "$canon_root" 2>/dev/null)"; then
  echo "error: failed to extract primary field $primary_field from $record_ref" >&2
  exit 1
fi

if ! secondary_value="$(build/t81 artifact read-field "$record_ref" \
  --schema "$record_schema" \
  --field "$secondary_field" \
  --canonfs-root "$canon_root" 2>/dev/null)"; then
  echo "error: failed to extract secondary field $secondary_field from $record_ref" >&2
  exit 1
fi

printf 'bundle_ref=%s\n' "$bundle_ref"
printf 'bundle_schema=%s\n' "$bundle_schema"
printf 'family=%s\n' "$family"
printf 'record_ref=%s\n' "$record_ref"
printf 'action_ref=%s\n' "$action_ref"
printf 'source_result_ref=%s\n' "$source_result_ref"
printf 'source_provenance_ref=%s\n' "$source_provenance_ref"
printf 'record_schema=%s\n' "$record_schema"
printf '%s=%s\n' "$primary_field" "$primary_value"
printf '%s=%s\n' "$secondary_field" "$secondary_value"
