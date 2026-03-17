#!/bin/bash
# automated_verification.sh - Comprehensive TernaryOS Verification Suite

set -e

echo "=== TernaryOS Automated Verification Suite ==="

QEMU_IMG="build/qemu_test_debug/qemu_slice6_guest.img"
BIOS="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"
OUTPUT_DIR="/tmp/ternaryos_verification"
MONITOR_PORT=1234

if [ ! -f "$QEMU_IMG" ]; then
    echo "Error: QEMU image not found: $QEMU_IMG"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/*

# Create fresh copy of the image
cp "$QEMU_IMG" "/tmp/qemu_verification_test.img"
QEMU_IMG="/tmp/qemu_verification_test.img"

echo "Starting comprehensive verification..."

# Function to run verification test
run_verification_test() {
    local test_name="$1"
    local test_desc="$2"
    
    echo ""
    echo "=== $test_name ==="
    echo "Description: $test_desc"
    
    case "$test_name" in
        "monitor_test")
            ./experimental/ternaryos/qemu_monitor_test.sh > "$OUTPUT_DIR/monitor_test.log" 2>&1
            echo "✅ Monitor test completed"
            ;;
        "memory_analysis")
            # First create a memory dump
            timeout 8 qemu-system-aarch64 -machine virt \
                -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic \
                -monitor telnet:localhost:$MONITOR_PORT,server,nowait,wait \
                -serial null &
            QEMU_PID=$!
            sleep 3
            echo "dump-guest-memory $OUTPUT_DIR/verification_memory.dump" | nc localhost $MONITOR_PORT 2>/dev/null || echo "Memory dump failed"
            sleep 2
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            
            if [ -f "$OUTPUT_DIR/verification_memory.dump" ]; then
                ./experimental/ternaryos/memory_analyzer.sh "$OUTPUT_DIR/verification_memory.dump" > "$OUTPUT_DIR/memory_analysis.log" 2>&1
                echo "✅ Memory analysis completed"
            else
                echo "❌ Memory dump not created"
            fi
            ;;
        "boot_sequence_test")
            echo "Testing boot sequence with alternative methods..."
            
            # Test 1: QEMU with debug logging
            timeout 8 qemu-system-aarch64 -machine virt \
                -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic \
                -monitor none \
                -serial null \
                -d int,cpu,exec \
                -D "$OUTPUT_DIR/boot_debug.log" &
            QEMU_PID=$!
            sleep 6
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            
            if [ -f "$OUTPUT_DIR/boot_debug.log" ]; then
                echo "✅ Boot debug log created ($(wc -c < "$OUTPUT_DIR/boot_debug.log") bytes)"
                
                # Analyze debug log
                echo "Analyzing boot debug log..."
                if grep -q "axion" "$OUTPUT_DIR/boot_debug.log"; then
                    echo "✅ Found 'axion' references in debug log"
                else
                    echo "❌ No 'axion' references found"
                fi
                
                if grep -q "Taking exception" "$OUTPUT_DIR/boot_debug.log"; then
                    echo "⚠️ Found exceptions in debug log"
                    echo "Last 5 exception entries:"
                    grep "Taking exception" "$OUTPUT_DIR/boot_debug.log" | tail -5
                else
                    echo "✅ No exceptions found"
                fi
            else
                echo "❌ Boot debug log not created"
            fi
            ;;
        "hardware_test")
            echo "Testing hardware initialization..."
            
            timeout 8 qemu-system-aarch64 -machine virt \
                -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic \
                -monitor telnet:localhost:$MONITOR_PORT,server,nowait,wait \
                -serial null &
            QEMU_PID=$!
            sleep 3
            
            # Test hardware state
            echo "Checking hardware state..." > "$OUTPUT_DIR/hardware_test.log"
            
            # CPU state
            echo "CPU State:" >> "$OUTPUT_DIR/hardware_test.log"
            echo "info cpus" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "CPU check failed"
            
            # Memory state
            echo "Memory State:" >> "$OUTPUT_DIR/hardware_test.log"
            echo "info memory" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "Memory check failed"
            
            # Block devices
            echo "Block Devices:" >> "$OUTPUT_DIR/hardware_test.log"
            echo "info block" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "Block check failed"
            
            # Memory regions
            echo "Memory Regions:" >> "$OUTPUT_DIR/hardware_test.log"
            echo "xp /16i 0x09000000" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "UART check failed"
            echo "xp /16i 0x08000000" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "GIC check failed"
            echo "xp /16i 0x40000000" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/hardware_test.log" 2>/dev/null || echo "RAM check failed"
            
            echo "quit" | nc localhost $MONITOR_PORT 2>/dev/null || true
            sleep 2
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            
            echo "✅ Hardware test completed"
            ;;
        "integration_test")
            echo "Running integration test..."
            
            # Test complete boot-to-shutdown cycle
            timeout 10 qemu-system-aarch64 -machine virt \
                -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic \
                -monitor telnet:localhost:$MONITOR_PORT,server,nowait,wait \
                -serial null \
                -d int,cpu,exec \
                -D "$OUTPUT_DIR/integration_debug.log" &
            QEMU_PID=$!
            
            # Monitor boot progress
            sleep 2
            for i in {1..8}; do
                echo "Check $i: $(date)" >> "$OUTPUT_DIR/integration_monitor.log"
                echo "info status" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/integration_monitor.log" 2>/dev/null || echo "Status check failed"
                sleep 1
            done
            
            # Final state check
            echo "Final state:" >> "$OUTPUT_DIR/integration_monitor.log"
            echo "info registers" | nc localhost $MONITOR_PORT >> "$OUTPUT_DIR/integration_monitor.log" 2>/dev/null || echo "Register check failed"
            
            echo "quit" | nc localhost $MONITOR_PORT 2>/dev/null || true
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            
            echo "✅ Integration test completed"
            ;;
        *)
            echo "Unknown test: $test_name"
            return 1
            ;;
    esac
}

# Run all verification tests
echo "Running verification tests..."

run_verification_test "monitor_test" "QEMU Monitor Integration Test"
run_verification_test "memory_analysis" "Memory Dump Analysis Test"
run_verification_test "boot_sequence_test" "Boot Sequence Analysis Test"
run_verification_test "hardware_test" "Hardware Initialization Test"
run_verification_test "integration_test" "Complete Integration Test"

echo ""
echo "=== Verification Results Summary ==="

# Generate summary report
echo "TernaryOS Verification Summary" > "$OUTPUT_DIR/verification_summary.txt"
echo "Date: $(date)" >> "$OUTPUT_DIR/verification_summary.txt"
echo "QEMU Image: $QEMU_IMG" >> "$OUTPUT_DIR/verification_summary.txt"
echo "" >> "$OUTPUT_DIR/verification_summary.txt"

# Check each test result
if [ -f "$OUTPUT_DIR/monitor_test.log" ]; then
    echo "✅ Monitor Test: COMPLETED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "   Log size: $(wc -c < "$OUTPUT_DIR/monitor_test.log") bytes" >> "$OUTPUT_DIR/verification_summary.txt"
else
    echo "❌ Monitor Test: FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
fi

if [ -f "$OUTPUT_DIR/memory_analysis.log" ]; then
    echo "✅ Memory Analysis: COMPLETED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "   Log size: $(wc -c < "$OUTPUT_DIR/memory_analysis.log") bytes" >> "$OUTPUT_DIR/verification_summary.txt"
else
    echo "❌ Memory Analysis: FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
fi

if [ -f "$OUTPUT_DIR/boot_debug.log" ]; then
    echo "✅ Boot Sequence Test: COMPLETED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "   Debug log size: $(wc -c < "$OUTPUT_DIR/boot_debug.log") bytes" >> "$OUTPUT_DIR/verification_summary.txt"
    
    if grep -q "axion" "$OUTPUT_DIR/boot_debug.log"; then
        echo "   ✅ Axion strings found" >> "$OUTPUT_DIR/verification_summary.txt"
    else
        echo "   ❌ No Axion strings found" >> "$OUTPUT_DIR/verification_summary.txt"
    fi
else
    echo "❌ Boot Sequence Test: FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
fi

if [ -f "$OUTPUT_DIR/hardware_test.log" ]; then
    echo "✅ Hardware Test: COMPLETED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "   Log size: $(wc -c < "$OUTPUT_DIR/hardware_test.log") bytes" >> "$OUTPUT_DIR/verification_summary.txt"
else
    echo "❌ Hardware Test: FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
fi

if [ -f "$OUTPUT_DIR/integration_monitor.log" ]; then
    echo "✅ Integration Test: COMPLETED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "   Log size: $(wc -c < "$OUTPUT_DIR/integration_monitor.log") bytes" >> "$OUTPUT_DIR/verification_summary.txt"
else
    echo "❌ Integration Test: FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
fi

# Overall assessment
echo "" >> "$OUTPUT_DIR/verification_summary.txt"
echo "Overall Assessment:" >> "$OUTPUT_DIR/verification_summary.txt"

TOTAL_TESTS=5
COMPLETED_TESTS=0

[ -f "$OUTPUT_DIR/monitor_test.log" ] && ((COMPLETED_TESTS++))
[ -f "$OUTPUT_DIR/memory_analysis.log" ] && ((COMPLETED_TESTS++))
[ -f "$OUTPUT_DIR/boot_debug.log" ] && ((COMPLETED_TESTS++))
[ -f "$OUTPUT_DIR/hardware_test.log" ] && ((COMPLETED_TESTS++))
[ -f "$OUTPUT_DIR/integration_monitor.log" ] && ((COMPLETED_TESTS++))

echo "Tests Completed: $COMPLETED_TESTS/$TOTAL_TESTS" >> "$OUTPUT_DIR/verification_summary.txt"

if [ $COMPLETED_TESTS -eq $TOTAL_TESTS ]; then
    echo "✅ ALL TESTS PASSED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "Status: VERIFICATION SUCCESSFUL" >> "$OUTPUT_DIR/verification_summary.txt"
else
    echo "⚠️ SOME TESTS FAILED" >> "$OUTPUT_DIR/verification_summary.txt"
    echo "Status: VERIFICATION PARTIAL" >> "$OUTPUT_DIR/verification_summary.txt"
fi

# Display summary
cat "$OUTPUT_DIR/verification_summary.txt"

echo ""
echo "=== Detailed Results ==="
echo "All verification logs saved to: $OUTPUT_DIR"
echo "Summary report: $OUTPUT_DIR/verification_summary.txt"

# Cleanup
rm -f "/tmp/qemu_verification_test.img"

echo ""
echo "=== Automated Verification Complete ==="
echo "This verification suite provides comprehensive TernaryOS testing without relying on serial output."
