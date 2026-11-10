#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# Model Fixtures Provisioning Script
#
# Provisions test model fixtures required for CLI contract tests and model
# artifact review smoke tests:
#   1. models/tiny-random-llama/model.safetensors (Source float fixture)
#   2. models/tiny-random-llama-exported.safetensors (Exported native-ternary-i8 fixture)
#
# Provenance:
#   - Source repo: hf-internal-testing/tiny-random-LlamaForCausalLM
#   - Pinned immutable commit: 9fb191250dd56d0ba7ec9785a025ed29c03d5998
#   - Expected source size: 4,131,280 bytes
#   - Expected source SHA-256: 49c20f32c6c597480fcaec5df2f86c645eabea765cbea1e67886dbae45e5c992
#   - Deterministic generator: t81 weights import + t81 weights export
#   - Expected exported size: 1,034,687 bytes
#   - Expected exported SHA-256: ce3629ffc25dfce184677dd7dc9796e8c259c0ff9f544b37ea00fe098f86b780
# ==============================================================================

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

t81_bin=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --t81-bin)
      t81_bin="$2"
      shift 2
      ;;
    *)
      echo "error: unknown option: $1" >&2
      exit 1
      ;;
  esac
done

if [[ -z "$t81_bin" ]]; then
  if [[ -x "build-warn-strict/t81" ]]; then
    t81_bin="build-warn-strict/t81"
  elif [[ -x "build/t81" ]]; then
    t81_bin="build/t81"
  elif [[ -x "build-cxxstd/t81" ]]; then
    t81_bin="build-cxxstd/t81"
  fi
fi

source_dir="models/tiny-random-llama"
source_file="$source_dir/model.safetensors"
exported_file="models/tiny-random-llama-exported.safetensors"

expected_source_sha256="49c20f32c6c597480fcaec5df2f86c645eabea765cbea1e67886dbae45e5c992"
expected_source_size=4131280

expected_exported_sha256="ce3629ffc25dfce184677dd7dc9796e8c259c0ff9f544b37ea00fe098f86b780"
expected_exported_size=1034687

hf_commit="9fb191250dd56d0ba7ec9785a025ed29c03d5998"
hf_url="https://huggingface.co/hf-internal-testing/tiny-random-LlamaForCausalLM/resolve/${hf_commit}/model.safetensors"

compute_sha256() {
  local file="$1"
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$file" | awk '{print $1}'
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$file" | awk '{print $1}'
  else
    python3 -c "import hashlib, sys; print(hashlib.sha256(open(sys.argv[1], 'rb').read()).hexdigest())" "$file"
  fi
}

verify_file() {
  local path="$1"
  local exp_size="$2"
  local exp_hash="$3"
  local label="$4"

  if [[ ! -f "$path" ]]; then
    return 1
  fi

  local actual_size
  actual_size=$(python3 -c "import os, sys; print(os.path.getsize(sys.argv[1]))" "$path")
  if [[ "$actual_size" -ne "$exp_size" ]]; then
    echo "::warning::$label size mismatch: got $actual_size, expected $exp_size" >&2
    return 1
  fi

  local actual_hash
  actual_hash="$(compute_sha256 "$path")"
  if [[ "$actual_hash" != "$exp_hash" ]]; then
    echo "::warning::$label sha256 mismatch: got $actual_hash, expected $exp_hash" >&2
    return 1
  fi

  return 0
}

# Check if both fixtures already exist and match integrity check
if verify_file "$source_file" "$expected_source_size" "$expected_source_sha256" "source fixture" && \
   verify_file "$exported_file" "$expected_exported_size" "$expected_exported_sha256" "exported fixture"; then
  echo "Model fixtures are already provisioned and verified."
  exit 0
fi

mkdir -p "$source_dir"

# Download and verify source fixture if needed
if ! verify_file "$source_file" "$expected_source_size" "$expected_source_sha256" "source fixture"; then
  echo "Downloading pinned source fixture from Hugging Face ($hf_url)..."
  rm -f "$source_file"
  tmp_download="${source_file}.tmp.$$"
  curl -fsSL --retry 3 --retry-delay 5 "$hf_url" -o "$tmp_download"
  mv "$tmp_download" "$source_file"

  if ! verify_file "$source_file" "$expected_source_size" "$expected_source_sha256" "source fixture"; then
    echo "error: failed to verify source fixture at $source_file" >&2
    rm -f "$source_file"
    exit 1
  fi
  echo "Source fixture verified (SHA-256: $expected_source_sha256)."
fi

# Generate and verify exported fixture if needed
if ! verify_file "$exported_file" "$expected_exported_size" "$expected_exported_sha256" "exported fixture"; then
  if [[ -z "$t81_bin" || ! -x "$t81_bin" ]]; then
    echo "error: missing t81 binary to generate exported fixture. Build t81 first or pass --t81-bin <path>." >&2
    exit 1
  fi

  echo "Generating exported fixture using $t81_bin..."
  tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/t81-export-fixture.XXXXXX")"
  trap 'rm -rf "$tmp_dir"' EXIT

  tmp_t81w="$tmp_dir/intermediate.t81w"
  tmp_exported="$tmp_dir/exported.safetensors"

  "$t81_bin" weights import "$source_file" -o "$tmp_t81w" >/dev/null
  "$t81_bin" weights export "$tmp_t81w" --to-safetensors "$tmp_exported" >/dev/null

  mv "$tmp_exported" "$exported_file"

  if ! verify_file "$exported_file" "$expected_exported_size" "$expected_exported_sha256" "exported fixture"; then
    echo "error: failed to verify generated exported fixture at $exported_file" >&2
    rm -f "$exported_file"
    exit 1
  fi
  echo "Exported fixture verified (SHA-256: $expected_exported_sha256)."
fi

echo "All model fixtures successfully provisioned and verified."
