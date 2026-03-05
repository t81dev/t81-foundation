#!/bin/bash

# T81 AI CLI Smoke Test
# Verifies minimal AI CLI integration works correctly

set -e  # Exit on any error

echo "🧪 T81 AI CLI Smoke Test"
echo "=========================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to print status
print_status() {
    local status=$1
    local message=$2
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $message"
    else
        echo -e "${RED}❌ FAIL${NC}: $message"
        exit 1
    fi
}

# Function to print warning
print_warning() {
    echo -e "${YELLOW}⚠️  WARN${NC}: $1"
}

# Check prerequisites
echo "Checking prerequisites..."

if ! command_exists cmake; then
    print_status "FAIL" "cmake not found"
    exit 1
fi

if ! command_exists make; then
    print_status "FAIL" "make not found"
    exit 1
fi

print_status "PASS" "Prerequisites found"

# Get repository directory (assuming we're running from scripts directory)
REPO_DIR=$(cd "$(dirname "$0")/.." && pwd)
print_status "PASS" "Repository directory: $REPO_DIR"

# Create test directory
TEST_DIR=$(mktemp -d)
echo "Working in temporary directory: $TEST_DIR"

# Create build directory
BUILD_DIR="$REPO_DIR/build_test_ai"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Test 1: Configure build with AI experiments
echo ""
echo "Test 1: CMake Configuration"
if cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON >/dev/null 2>&1; then
    print_status "PASS" "CMake configuration with AI experiments"
else
    print_status "FAIL" "CMake configuration failed"
    exit 1
fi

# Test 2: Build AI CLI
echo ""
echo "Test 2: Build AI CLI"
if make t81_ai >/dev/null 2>&1; then
    print_status "PASS" "AI CLI build successful"
else
    print_status "FAIL" "AI CLI build failed"
    exit 1
fi

# Check if binary exists
AI_CLI="$BUILD_DIR/experiments/ai/ux_tools/t81_ai"
if [ -f "$AI_CLI" ]; then
    print_status "PASS" "AI CLI binary exists at $AI_CLI"
else
    print_status "FAIL" "AI CLI binary not found"
    exit 1
fi

# Test 3: Help command
echo ""
echo "Test 3: Help Command"
HELP_OUTPUT=$("$AI_CLI" --help 2>&1)
if echo "$HELP_OUTPUT" | grep -q "T81 AI CLI"; then
    print_status "PASS" "Help command works"
else
    print_status "FAIL" "Help command failed"
    echo "Output: $HELP_OUTPUT"
    exit 1
fi

# Test 4: Model inspect with existing file
echo ""
echo "Test 4: Model Inspect (Existing File)"
echo "mock model data" > test_model.gguf
INSPECT_OUTPUT=$("$AI_CLI" model inspect test_model.gguf 2>&1)
if echo "$INSPECT_OUTPUT" | grep -q "Model Inspection"; then
    print_status "PASS" "Model inspect works"
else
    print_status "FAIL" "Model inspect failed"
    echo "Output: $INSPECT_OUTPUT"
    exit 1
fi

# Test 5: Model verify with existing file
echo ""
echo "Test 5: Model Verify (Existing File)"
VERIFY_OUTPUT=$("$AI_CLI" verify test_model.gguf 2>&1)
if echo "$VERIFY_OUTPUT" | grep -q "Model Verification"; then
    print_status "PASS" "Model verify works"
else
    print_status "FAIL" "Model verify failed"
    echo "Output: $VERIFY_OUTPUT"
    exit 1
fi

# Test 6: Error handling for missing file
echo ""
echo "Test 6: Error Handling (Missing File)"
ERROR_OUTPUT=$("$AI_CLI" verify nonexistent.gguf 2>&1)
ERROR_CODE=$?
if echo "$ERROR_OUTPUT" | grep -q "does not exist" && [ $ERROR_CODE -eq 1 ]; then
    print_status "PASS" "Error handling works correctly"
else
    print_status "FAIL" "Error handling failed"
    echo "Output: $ERROR_OUTPUT"
    echo "Exit code: $ERROR_CODE"
    exit 1
fi

# Test 7: Invalid command handling
echo ""
echo "Test 7: Invalid Command Handling"
INVALID_OUTPUT=$("$AI_CLI" invalid_command 2>&1)
if echo "$INVALID_OUTPUT" | grep -q "Unknown command"; then
    print_status "PASS" "Invalid command handling works"
else
    print_status "FAIL" "Invalid command handling failed"
    echo "Output: $INVALID_OUTPUT"
    exit 1
fi

# Cleanup
echo ""
echo "Cleanup..."
rm -rf "$BUILD_DIR"
rm -f test_model.gguf
print_status "PASS" "Cleanup completed"

# Summary
echo ""
echo "=========================="
echo -e "${GREEN}🎉 ALL TESTS PASSED${NC}"
echo "T81 AI CLI minimal integration is working correctly!"
echo ""
echo "Next steps:"
echo "1. Review documentation: docs/experiments/AI_EXPERIMENTS.md"
echo "2. Plan next incremental feature"
echo "3. Ensure all changes remain isolated to /experiments/ai"
echo ""
echo "Repository is ready for stable commit."
