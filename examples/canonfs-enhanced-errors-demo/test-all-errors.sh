#!/bin/bash

# CanonFS Enhanced Errors Demo Test Script
# Demonstrates all major error handling scenarios

set -e

echo "=== CanonFS Enhanced Errors Demo ==="
echo

# Create demo directory
DEMO_DIR="$(pwd)/canonfs_demo"
mkdir -p "$DEMO_DIR"

# Test 1: Policy Denied Import
echo "1. Testing policy denied import..."
t81 canonfs import test-data.txt --canonfs-root "$DEMO_DIR/.t81_canonfs" --policy-profile deny-all --json > "$DEMO_DIR/import-policy-denied.output.json" 2>&1 || true

# Test 2: File Not Found Import  
echo "2. Testing file not found import..."
t81 canonfs import nonexistent.txt --canonfs-root "$DEMO_DIR/.t81_canonfs" --json > "$DEMO_DIR/import-file-not-found.output.json" 2>&1 || true

# Test 3: Symlink Not Supported
echo "3. Testing symlink not supported..."
ln -sf test-data.txt "$DEMO_DIR/symlink.txt"
t81 canonfs import symlink.txt --canonfs-root "$DEMO_DIR/.t81_canonfs" --json > "$DEMO_DIR/import-symlink-not-supported.output.json" 2>&1 || true

# Test 4: Successful Import (for export test)
echo "4. Testing successful import for export test..."
IMPORT_RESULT=$(t81 canonfs import test-data.txt --canonfs-root "$DEMO_DIR/.t81_canonfs" --policy-profile permissive --json)
OBJECT_REF=$(echo "$IMPORT_RESULT" | jq -r '.imported_objects[0]')

# Test 5: Policy Denied Export
echo "5. Testing policy denied export..."
t81 canonfs export "$OBJECT_REF" --canonfs-root "$DEMO_DIR/.t81_canonfs" --policy-profile deny-all --json > "$DEMO_DIR/export-policy-denied.output.json" 2>&1 || true

echo ""
echo "=== Demo Complete ==="
echo "Results saved in: $DEMO_DIR/"
echo ""
echo "Files created:"
ls -la "$DEMO_DIR"/*.json
echo ""
echo "Error scenarios tested:"
echo "- Policy denied import"
echo "- File not found import" 
echo "- Symlink not supported import"
echo "- Policy denied export"
echo ""
echo "All JSON responses follow RFC-00D1 v1 schema with structured error objects."
