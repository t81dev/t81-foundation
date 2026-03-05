#!/bin/bash

set -euo pipefail

echo "🧪 Simple AI CLI Test"
echo "====================="

# Determine repository root robustly
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

# Test 1: Build
rm -rf build_simple
mkdir build_simple
cd build_simple

if cmake .. -G Ninja -DT81_ENABLE_AI_EXPERIMENTS=ON >/dev/null 2>&1; then
    echo "✅ CMake config OK"
else
    echo "❌ CMake config failed"
    exit 1
fi

if cmake --build . --target t81_ai >/dev/null 2>&1; then
    echo "✅ Build OK"
else
    echo "❌ Build failed"
    exit 1
fi

# Test 2: Commands
if ./experiments/ai/ux_tools/t81_ai --help >/dev/null 2>&1; then
    echo "✅ Help command OK"
else
    echo "❌ Help command failed"
    exit 1
fi

echo "mock data" > test.gguf
if ./experiments/ai/ux_tools/t81_ai model inspect test.gguf >/dev/null 2>&1; then
    echo "✅ Model inspect OK"
else
    echo "❌ Model inspect failed"
    exit 1
fi

if ./experiments/ai/ux_tools/t81_ai verify test.gguf >/dev/null 2>&1; then
    echo "✅ Model verify OK"
else
    echo "❌ Model verify failed"
    exit 1
fi

if ./experiments/ai/ux_tools/t81_ai verify missing.gguf >/dev/null 2>&1; then
    echo "❌ Error handling failed (should have failed)"
    exit 1
else
    echo "✅ Error handling OK"
fi

echo ""
echo "🎉 ALL TESTS PASSED"
echo "AI CLI minimal integration is working!"
