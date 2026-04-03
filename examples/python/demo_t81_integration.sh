#!/usr/bin/env bash
set -euo pipefail

# Demo script for T81 Integrated Bundle Marketplace
# This script demonstrates the complete T81 bundle workflow

echo "=========================================================="
echo " T81 Integrated Bundle Marketplace Demo"
echo "=========================================================="

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

# Check if T81 is built
if [[ ! -x "build/t81" ]]; then
    echo "❌ Error: T81 CLI not found"
    echo "Build it first: cmake --build build --target t81"
    exit 1
fi

# Check if assess-fixed demo builder exists
if [[ ! -x "build/t81_make_assess_fixed_demo" ]]; then
    echo "❌ Error: assess-fixed demo builder not found"
    echo "Build it first: cmake --build build --target t81_make_assess_fixed_demo"
    exit 1
fi

# Create temporary CanonFS root
tmp_root="$(mktemp -d)"
canon_root="$tmp_root/.t81_canonfs"
mkdir -p "$canon_root"

export T81_CANONFS_ROOT="$canon_root"

echo "📁 CanonFS Root: $canon_root"
echo ""

# Step 1: Create an assess-fixed bundle
echo "🔄 Step 1: Creating assess-fixed bundle..."
python3 examples/python/t81_integrated_bundle_marketplace.py \
    create-assess-fixed \
    examples/python/example_model_config.json \
    examples/python/example_input_data.json

echo ""

# Step 2: Get the bundle reference from the output
output=$(python3 examples/python/t81_integrated_bundle_marketplace.py \
    create-assess-fixed \
    examples/python/example_model_config.json \
    examples/python/example_input_data.json 2>/dev/null)

bundle_ref=$(echo "$output" | grep "BUNDLE_REF:" | cut -d':' -f2-)

if [[ -z "$bundle_ref" ]]; then
    echo "❌ Error: No bundle found in CanonFS"
    exit 1
fi

echo "📦 Bundle Reference: $bundle_ref"
echo ""

# Step 3: Get bundle details
echo "🔍 Step 2: Getting bundle details..."
python3 examples/python/t81_integrated_bundle_marketplace.py \
    get-bundle "$bundle_ref"

echo ""

# Step 4: Consume the bundle following the contract
echo "📖 Step 3: Consuming bundle following contract..."
python3 examples/python/t81_integrated_bundle_marketplace.py \
    consume-bundle "$bundle_ref"

echo ""

# Step 5: Show bundle summary using T81's summarize script
echo "📋 Step 4: Summarizing bundle with T81 CLI..."
if [[ -f "examples/ai-and-inference/model-load-canonfs/summarize_ai_bundle.sh" ]]; then
    bash examples/ai-and-inference/model-load-canonfs/summarize_ai_bundle.sh \
        "$bundle_ref" "$canon_root"
else
    echo "⚠️  T81 summarize script not found"
fi

echo ""
echo "=========================================================="
echo " Demo Complete"
echo "=========================================================="
echo "📁 CanonFS artifacts stored in: $canon_root"
echo ""
echo "To inspect the CanonFS contents:"
echo "  find $canon_root -type f -name '*.json' | head -10"
echo ""
echo "To clean up:"
echo "  rm -rf $tmp_root"
