#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

t81_bin="${T81_BIN:-$repo_root/build/t81}"
hf_bin="${HF_BIN:-$HOME/Library/Python/3.14/bin/hf}"
model_dir="${MODEL_DIR:-$repo_root/models/tiny-random-llama}"
model_safetensors="$model_dir/model.safetensors"
model_t81w="${MODEL_T81W:-/tmp/tiny-random-llama.t81w}"

if [[ ! -x "$t81_bin" ]]; then
  echo "error: missing t81 binary at $t81_bin" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

mkdir -p "$model_dir"

if [[ ! -f "$model_safetensors" ]]; then
  if [[ ! -x "$hf_bin" ]]; then
    echo "error: tiny model not present and hf CLI not found at $hf_bin" >&2
    exit 1
  fi
  "$hf_bin" download \
    hf-internal-testing/tiny-random-LlamaForCausalLM \
    config.json tokenizer.json model.safetensors \
    --local-dir "$model_dir"
fi

"$t81_bin" weights import "$model_safetensors" -o "$model_t81w" >/tmp/t81_real_hf_import.log
cat /tmp/t81_real_hf_import.log

tmp_root="$(mktemp -d /tmp/t81-real-hf-flow.XXXXXX)"
canon_root="$tmp_root/.t81_canonfs"
program_path="$tmp_root/matmul_real_tensor.t81"
allow_policy="$tmp_root/allow.apl"
deny_policy="$tmp_root/deny.apl"
mkdir -p "$canon_root"

model_hash="$("$t81_bin" canonfs put-file "$model_t81w" --canonfs-root "$canon_root")"
model_hash="${model_hash//$'\n'/}"
model_checksum="$("$t81_bin" weights info "$model_t81w" --json | python3 -c 'import sys, json; print(json.load(sys.stdin)["checksum_sha3_512"])')"

cat >"$program_path" <<'EOF'
fn main() -> i32 {
  let q: i32 = std.tensor.load("model.layers.0.self_attn.q_proj.weight");
  let k: i32 = std.tensor.load("model.layers.0.self_attn.k_proj.weight");
  let out: Tensor = std.tensor.matmul(q, k);
  let _ = out;
  print(q);
  return 0;
}
EOF

cat >"$allow_policy" <<EOF
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:$model_checksum"]))
EOF

cat >"$deny_policy" <<'EOF'
(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:cafebabe"]))
EOF

export T81_CANONFS_ROOT="$canon_root"

echo "MODEL_HASH=$model_hash"
echo "MODEL_CHECKSUM=$model_checksum"
echo
echo "[ALLOW]"
"$t81_bin" code run "$program_path" --weights-model "$model_hash" --policy "$allow_policy"
echo
echo "[DENY]"
set +e
"$t81_bin" code run "$program_path" --weights-model "$model_hash" --policy "$deny_policy"
deny_rc=$?
set -e
echo
echo "DENY_EXIT=$deny_rc"
