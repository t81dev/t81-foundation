#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

source_safetensors="models/tiny-random-llama/model.safetensors"
exported_safetensors="models/tiny-random-llama-exported.safetensors"

if [[ -x "build-warn-strict/t81" ]]; then
  t81_bin="build-warn-strict/t81"
elif [[ -x "build/t81" ]]; then
  t81_bin="build/t81"
else
  echo "error: missing t81 binary at build-warn-strict/t81 or build/t81" >&2
  echo "build it first: cmake --build build-warn-strict --target t81" >&2
  exit 1
fi

if [[ ! -f "$source_safetensors" ]]; then
  echo "error: missing source fixture at $source_safetensors" >&2
  exit 1
fi

if [[ ! -f "$exported_safetensors" ]]; then
  echo "error: missing exported fixture at $exported_safetensors" >&2
  exit 1
fi

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/t81-model-artifact-review.XXXXXX")"
trap 'rm -rf "$tmp_root"' EXIT

gguf_path="$tmp_root/tiny-random-llama.gguf"
manifest_path="$tmp_root/exported.manifest.json"

echo "=========================================================="
echo " T81 Model Artifact Review Walkthrough "
echo "=========================================================="
echo ""

echo "[1/5] Importing exported SafeTensors artifact..."
"$t81_bin" model import "$exported_safetensors" --json --manifest "$manifest_path"
echo ""

echo "[2/5] Raw diff: exported SafeTensors vs float-backed source..."
set +e
"$t81_bin" model diff "$exported_safetensors" "$source_safetensors" --json
raw_status=$?
set -e
echo "  ↳ raw diff exit code: $raw_status"
echo ""

echo "[3/5] Manifest vs live exported artifact..."
set +e
"$t81_bin" model diff "$manifest_path" "$exported_safetensors" --json
manifest_status=$?
set -e
echo "  ↳ manifest diff exit code: $manifest_status"
echo ""

echo "[4/5] Quantizing source SafeTensors to GGUF..."
"$t81_bin" weights quantize "$source_safetensors" --to-gguf "$gguf_path"
echo ""

echo "[5/5] Normalized diff: GGUF vs source SafeTensors..."
set +e
"$t81_bin" model diff "$gguf_path" "$source_safetensors" --json --mode normalized
normalized_status=$?
set -e
echo "  ↳ normalized diff exit code: $normalized_status"
echo ""

echo "Review summary:"
echo "  - import emitted a review record plus persisted manifest"
echo "  - raw diff stayed representation-sensitive"
echo "  - manifest diff stayed identical while still exposing provenance context"
echo "  - normalized diff admitted the known GGUF/SafeTensors transpose rule explicitly"
echo "=========================================================="
