#!/bin/bash
# memory_analyzer.sh - Memory Dump Analysis for TernaryOS Verification

set -e

echo "=== Memory Dump Analysis Tool ==="

MEMORY_DUMP_FILE="$1"
OUTPUT_DIR="/tmp/memory_analysis"

if [ -z "$MEMORY_DUMP_FILE" ]; then
    echo "Usage: $0 <memory_dump_file>"
    echo "Example: $0 /tmp/qemu_memory_dump.bin"
    exit 1
fi

if [ ! -f "$MEMORY_DUMP_FILE" ]; then
    echo "Error: Memory dump file not found: $MEMORY_DUMP_FILE"
    exit 1
fi

echo "Analyzing memory dump: $MEMORY_DUMP_FILE"
echo "File size: $(wc -c < "$MEMORY_DUMP_FILE") bytes"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Function to extract and analyze memory region
analyze_memory_region() {
    local addr="$1"
    local size="$2"
    local description="$3"
    local output_file="$OUTPUT_DIR/region_${addr}_${description}.bin"
    
    echo "Analyzing $description at 0x$addr (${size} bytes)..."
    
    # Extract memory region (using dd with skip and count)
    local skip_bytes=$((addr))
    local count_bytes=$size
    
    if [ $skip_bytes -lt $(wc -c < "$MEMORY_DUMP_FILE") ]; then
        dd if="$MEMORY_DUMP_FILE" of="$output_file" bs=1 skip=$skip_bytes count=$count_bytes 2>/dev/null || true
        
        if [ -f "$output_file" ] && [ -s "$output_file" ]; then
            echo "  ✅ Extracted $(wc -c < "$output_file") bytes"
            
            # Hex dump first 64 bytes
            echo "  Hex dump (first 64 bytes):"
            hexdump -C "$output_file" | head -4
            
            # Search for interesting strings
            echo "  String analysis:"
            strings "$output_file" | head -10 || echo "    No strings found"
            
            # Check for ARM instruction patterns
            echo "  ARM instruction patterns:"
            hexdump -C "$output_file" | grep -E "([0-9a-f]{8}.*[0-9a-f]{4})" | head -5 || echo "    No clear instruction patterns"
        else
            echo "  ❌ Failed to extract region (out of bounds)"
        fi
    else
        echo "  ❌ Address 0x$addr out of memory dump range"
    fi
}

echo ""
echo "=== Memory Region Analysis ==="

# Analyze key memory regions for TernaryOS
analyze_memory_region "0x09000000" "4096" "pl011_uart"
analyze_memory_region "0x08000000" "4096" "gic_distributor"
analyze_memory_region "0x080A0000" "4096" "gic_redistributor"
analyze_memory_region "0x40000000" "4096" "ram_base"
analyze_memory_region "0x48000000" "4096" "kernel_load_area"
analyze_memory_region "0x00000000" "4096" "null_page"
analyze_memory_region "0x00008000" "4096" "exception_vectors"
analyze_memory_region "0x00010000" "4096" "boot_area"

echo ""
echo "=== String Pattern Analysis ==="

# Search for TernaryOS-specific strings
echo "Searching for TernaryOS strings..."
if strings "$MEMORY_DUMP_FILE" | grep -i -E "(axion|ternaryos|bootaa64|efi|kernel)" > "$OUTPUT_DIR/ternaryos_strings.txt"; then
    echo "✅ Found TernaryOS-related strings:"
    cat "$OUTPUT_DIR/ternaryos_strings.txt"
else
    echo "❌ No TernaryOS strings found"
fi

# Search for ARM instruction patterns
echo ""
echo "Searching for ARM instruction patterns..."
hexdump -C "$MEMORY_DUMP_FILE" | grep -E "([0-9a-f]{8}.*[0-9a-f]{8})" | head -20 > "$OUTPUT_DIR/arm_patterns.txt"
if [ -s "$OUTPUT_DIR/arm_patterns.txt" ]; then
    echo "✅ Found potential ARM instruction patterns:"
    cat "$OUTPUT_DIR/arm_patterns.txt"
else
    echo "❌ No clear ARM instruction patterns found"
fi

# Search for ASCII text patterns
echo ""
echo "Searching for ASCII text patterns..."
strings "$MEMORY_DUMP_FILE" | grep -E "^[A-Za-z][A-Za-z0-9 ]{4,}$" | head -20 > "$OUTPUT_DIR/ascii_patterns.txt"
if [ -s "$OUTPUT_DIR/ascii_patterns.txt" ]; then
    echo "✅ Found ASCII text patterns:"
    cat "$OUTPUT_DIR/ascii_patterns.txt"
else
    echo "❌ No ASCII text patterns found"
fi

echo ""
echo "=== Boot Signature Analysis ==="

# Look for UEFI/EFI signatures
echo "Searching for UEFI signatures..."
if hexdump -C "$MEMORY_DUMP_FILE" | grep -i "efi" > "$OUTPUT_DIR/efi_signatures.txt"; then
    echo "Found EFI signatures:"
    cat "$OUTPUT_DIR/efi_signatures.txt"
else
    echo "No EFI signatures found"
fi

# Look for ARM64 boot signatures
echo ""
echo "Searching for ARM64 boot signatures..."
if hexdump -C "$MEMORY_DUMP_FILE" | grep -E "([0-9a-f]{8}.*[0-9a-f]{8})" | grep -i "b000" > "$OUTPUT_DIR/arm64_boot.txt"; then
    echo "✅ Found ARM64 boot signatures:"
    cat "$OUTPUT_DIR/arm64_boot.txt"
else
    echo "❌ No ARM64 boot signatures found"
fi

echo ""
echo "=== Memory Pattern Analysis ==="

# Look for repeated patterns (indicative of uninitialized memory)
echo "Analyzing memory patterns..."
hexdump -C "$MEMORY_DUMP_FILE" | grep -E "(00 00 00 00|ff ff ff ff)" | head -10 > "$OUTPUT_DIR/memory_patterns.txt"
if [ -s "$OUTPUT_DIR/memory_patterns.txt" ]; then
    echo "✅ Found memory patterns:"
    cat "$OUTPUT_DIR/memory_patterns.txt"
else
    echo "❌ No clear memory patterns found"
fi

# Look for potential code regions (non-zero, non-repeating patterns)
echo ""
echo "Analyzing potential code regions..."
hexdump -C "$MEMORY_DUMP_FILE" | grep -v -E "(00 00 00 00|ff ff ff ff)" | head -20 > "$OUTPUT_DIR/code_regions.txt"
if [ -s "$OUTPUT_DIR/code_regions.txt" ]; then
    echo "✅ Found potential code regions:"
    cat "$OUTPUT_DIR/code_regions.txt"
else
    echo "❌ No clear code regions found"
fi

echo ""
echo "=== Statistical Analysis ==="

# Calculate memory statistics
echo "Memory dump statistics:"
echo "  Total size: $(wc -c < "$MEMORY_DUMP_FILE") bytes"
echo "  Zero bytes: $(hexdump -C "$MEMORY_DUMP_FILE" | grep -c "00 00 00 00") occurrences"
echo "  Non-zero regions: $(hexdump -C "$MEMORY_DUMP_FILE" | grep -v -c "00 00 00 00") regions"

# Find the highest non-zero address
echo "  Highest non-zero address: $(hexdump -C "$MEMORY_DUMP_FILE" | grep -v "00 00 00 00" | tail -1 | cut -d: -f1)"

echo ""
echo "=== Boot Sequence Indicators ==="

# Look for indicators of boot progress
echo "Checking for boot sequence indicators..."

# Check if UART was initialized (non-zero values at UART base)
UART_STATUS=$(hexdump -C "$MEMORY_DUMP_FILE" -s 0x09000000 -n 16 | grep -v "00 00 00 00")
if [ -n "$UART_STATUS" ]; then
    echo "✅ UART appears to have been accessed (non-zero values at 0x09000000)"
else
    echo "❌ UART appears uninitialized (all zeros at 0x09000000)"
fi

# Check if GIC was accessed
GIC_STATUS=$(hexdump -C "$MEMORY_DUMP_FILE" -s 0x08000000 -n 16 | grep -v "00 00 00 00")
if [ -n "$GIC_STATUS" ]; then
    echo "✅ GIC appears to have been accessed (non-zero values at 0x08000000)"
else
    echo "❌ GIC appears uninitialized (all zeros at 0x08000000)"
fi

# Check kernel load area
KERNEL_STATUS=$(hexdump -C "$MEMORY_DUMP_FILE" -s 0x48000000 -n 16 | grep -v "00 00 00 00")
if [ -n "$KERNEL_STATUS" ]; then
    echo "✅ Kernel load area shows activity (non-zero values at 0x48000000)"
else
    echo "❌ Kernel load area appears unused (all zeros at 0x48000000)"
fi

echo ""
echo "=== Summary Report ==="

# Generate summary
echo "Memory Dump Analysis Summary:" > "$OUTPUT_DIR/summary.txt"
echo "File: $MEMORY_DUMP_FILE" >> "$OUTPUT_DIR/summary.txt"
echo "Size: $(wc -c < "$MEMORY_DUMP_FILE") bytes" >> "$OUTPUT_DIR/summary.txt"
echo "Analysis Date: $(date)" >> "$OUTPUT_DIR/summary.txt"
echo "" >> "$OUTPUT_DIR/summary.txt"
echo "Key Findings:" >> "$OUTPUT_DIR/summary.txt"

if [ -f "$OUTPUT_DIR/ternaryos_strings.txt" ] && [ -s "$OUTPUT_DIR/ternaryos_strings.txt" ]; then
    echo "✅ TernaryOS strings found" >> "$OUTPUT_DIR/summary.txt"
else
    echo "❌ No TernaryOS strings found" >> "$OUTPUT_DIR/summary.txt"
fi

if [ -n "$UART_STATUS" ]; then
    echo "✅ UART accessed" >> "$OUTPUT_DIR/summary.txt"
else
    echo "❌ UART not accessed" >> "$OUTPUT_DIR/summary.txt"
fi

if [ -n "$GIC_STATUS" ]; then
    echo "✅ GIC accessed" >> "$OUTPUT_DIR/summary.txt"
else
    echo "❌ GIC not accessed" >> "$OUTPUT_DIR/summary.txt"
fi

if [ -n "$KERNEL_STATUS" ]; then
    echo "✅ Kernel load area active" >> "$OUTPUT_DIR/summary.txt"
else
    echo "❌ Kernel load area inactive" >> "$OUTPUT_DIR/summary.txt"
fi

echo "Analysis complete. Results saved to: $OUTPUT_DIR"
echo "Summary report: $OUTPUT_DIR/summary.txt"

cat "$OUTPUT_DIR/summary.txt"

echo ""
echo "=== Memory Analysis Complete ==="
echo "This analysis provides insights into TernaryOS boot progress without relying on serial output."
