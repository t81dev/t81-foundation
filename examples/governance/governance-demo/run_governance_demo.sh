#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$repo_root"

t81_bin="build/t81"
out_dir="build/governance-demo"
model_path="$out_dir/demo.t81w"
model_hash_file="$out_dir/model_hash.txt"
demo_dir="examples/governance/governance-demo"

mkdir -p "$out_dir"

if [[ ! -x "$t81_bin" ]]; then
  echo "error: missing t81 binary at $t81_bin" >&2
  echo "build it first: cmake --build build --target t81" >&2
  exit 1
fi

if [[ ! -x "build/t81_make_demo_model_gov" ]]; then
  echo "error: missing governance demo model builder at build/t81_make_demo_model_gov" >&2
  echo "build it first: cmake --build build --target t81_make_demo_model_gov" >&2
  exit 1
fi

echo "=========================================================="
echo " T81 Deterministic AI Governance Demo "
echo "=========================================================="
echo ""

echo "[1/3] Building synthetic .t81w model artifact..."
build/t81_make_demo_model_gov "$model_path" > "$out_dir/model_gen.log"
model_hash=$(grep "sha3-512=" "$out_dir/model_gen.log" | cut -d'=' -f2)
echo "  ↳ Model compiled deterministically."
echo "  ↳ Artifact Hash: sha3-512:$model_hash"
echo ""

# Inject correct model hash into allow.apl
sed "s/__MODEL_HASH__/sha3-512:$model_hash/g" "$demo_dir/allow.apl" > "$out_dir/allow_resolved.apl"

echo "[2/3] Executing Model with ALLOW policy..."
echo "  ↳ Policy requires exactly the correct model hash."
echo "----------------------------------------------------------"
set +e
"$t81_bin" code run \
  "$demo_dir/governed_matmul.t81" \
  --weights-model "$model_path" \
  --policy "$out_dir/allow_resolved.apl" >"$out_dir/allow.out" 2>"$out_dir/allow.err"
allow_res=$?
set -e
cat "$out_dir/allow.err"
cat "$out_dir/allow.out"
if [[ $allow_res -eq 0 ]]; then
  echo "  ↳ Result class: governed_execution"
  echo "  ↳ Approved sha3-512:$model_hash matched the allow policy."
  echo "  ↳ Weights were admitted and TMatMul executed, yielding a governed tensor result handle."
  echo "  ↳ Next (progress): none in this demo; the successful result remains a governed tensor handle."
  echo "  ↳ Next (inspect): compare the deny path below to confirm the same operation is blocked before compute."
fi
echo "----------------------------------------------------------"
echo ""

echo "[3/3] Executing Model with DENY policy..."
echo "  ↳ Policy explicitly forbids this model hash (e.g. untrusted supply chain)."
echo "----------------------------------------------------------"
set +e
"$t81_bin" code run \
  "$demo_dir/governed_matmul.t81" \
  --weights-model "$model_path" \
  --policy "$demo_dir/deny.apl" 2>"$out_dir/deny.err"
res=$?
set -e

cat "$out_dir/deny.err"

echo "----------------------------------------------------------"
echo "  ↳ Failed with exit code: $res"
echo "  ↳ Wait... how is that possible?"
echo "    The execution was blocked deep inside the VM interpreter loop"
echo "    BEFORE the TMatMul side effect could happen."
echo "    It isn't an external wrapper; it's a natively governed runtime."
echo "=========================================================="
