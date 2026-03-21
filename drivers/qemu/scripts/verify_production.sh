#!/bin/bash
# Production verification script for local deployment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Production Verification ==="

# Check build artifacts
if [ ! -f "$BUILD_DIR/ternaryos/qemu_slice6/BOOTAA64.EFI" ]; then
    echo "ERROR: BOOTAA64.EFI not found"
    exit 1
fi

echo "✅ Build artifacts verified"

# Run verification suite
cd "$PRODUCTION_DIR/../"
./experimental/ternaryos/automated_verification.sh

# Check results
if grep -q "VERIFICATION SUCCESSFUL" /tmp/ternaryos_verification/verification_summary.txt; then
    echo "✅ Production verification PASSED"
    exit 0
else
    echo "❌ Production verification FAILED"
    exit 1
fi
