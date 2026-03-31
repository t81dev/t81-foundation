#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

t81_bin="build/t81"
out_dir="build/governance-demo"
model_path="$out_dir/demo.t81w"
model_hash_file="$out_dir/model_hash.txt"

mkdir -p "$out_dir"

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
sed "s/__MODEL_HASH__/sha3-512:$model_hash/g" examples/governance-demo/allow.apl > "$out_dir/allow_resolved.apl"

echo "[2/3] Executing Model with ALLOW policy..."
echo "  ↳ Policy requires exactly the correct model hash."
echo "----------------------------------------------------------"
set +e
"$t81_bin" code run \
  "examples/governance-demo/governed_matmul.t81" \
  --weights-model "$model_path" \
  --policy "$out_dir/allow_resolved.apl" 2>"$out_dir/allow.err"
set -e
echo "----------------------------------------------------------"
echo ""

echo "[3/3] Executing Model with DENY policy..."
echo "  ↳ Policy explicitly forbids this model hash (e.g. untrusted supply chain)."
echo "----------------------------------------------------------"
set +e
"$t81_bin" code run \
  "examples/governance-demo/governed_matmul.t81" \
  --weights-model "$model_path" \
  --policy "examples/governance-demo/deny.apl" 2>"$out_dir/deny.err"
res=$?
set -e

cat "$out_dir/deny.err"

echo "----------------------------------------------------------"
echo "  ↳ Failed with exit code: $res"
echo "  ↳ Wait... how is that possible?"
echo "    The execution was blocked deep inside the VM interpreter loop"
echo "    BEFORE the QMATMUL side effect could happen."
echo "    It isn't an external wrapper; it's a natively governed runtime."
echo "=========================================================="
