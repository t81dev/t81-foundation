#!/bin/bash
# qemu_monitor_test.sh - QEMU Monitor Integration for TernaryOS Verification

set -e

echo "=== QEMU Monitor Integration Test ==="

QEMU_IMG="build/qemu_test_debug/qemu_slice6_guest.img"
BIOS="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"
MONITOR_PORT=1234
MEMORY_DUMP_FILE="/tmp/qemu_memory_dump.bin"

if [ ! -f "$QEMU_IMG" ]; then
    echo "Error: QEMU image not found: $QEMU_IMG"
    exit 1
fi

# Create a fresh copy of the image to avoid locking issues
cp "$QEMU_IMG" "/tmp/qemu_monitor_test.img"
QEMU_IMG="/tmp/qemu_monitor_test.img"

echo "Starting QEMU with monitor integration..."

# Start QEMU with monitor
qemu-system-aarch64 -machine virt \
    -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic \
    -monitor telnet:localhost:$MONITOR_PORT,server,nowait,wait \
    -serial null \
    -d int,cpu,exec \
    -D /tmp/qemu_debug.log &
QEMU_PID=$!

echo "QEMU started with PID: $QEMU_PID"
echo "Monitor available on localhost:$MONITOR_PORT"

# Wait for QEMU to initialize
sleep 3

# Function to send monitor command
send_monitor_cmd() {
    local cmd="$1"
    echo "Sending monitor command: $cmd"
    echo "$cmd" | nc localhost $MONITOR_PORT 2>/dev/null || echo "Command failed: $cmd"
    sleep 1
}

# Function to check if monitor is responsive
check_monitor() {
    echo "info version" | nc localhost $MONITOR_PORT 2>/dev/null | grep -q "QEMU" || return 1
    return 0
}

# Test monitor connectivity
if check_monitor; then
    echo "✅ Monitor is responsive"
else
    echo "❌ Monitor not responding"
    kill $QEMU_PID 2>/dev/null || true
    wait $QEMU_PID 2>/dev/null || true
    exit 1
fi

echo ""
echo "=== Monitor Commands ==="

# 1. Get QEMU version
echo "1. QEMU Version:"
send_monitor_cmd "info version"

# 2. Get CPU information
echo "2. CPU Information:"
send_monitor_cmd "info cpus"

# 3. Get register information
echo "3. Register Information:"
send_monitor_cmd "info registers"

# 4. Get memory information
echo "4. Memory Information:"
send_monitor_cmd "info memory"

# 5. Get interrupt information
echo "5. Interrupt Information:"
send_monitor_cmd "info interrupts"

# 6. Get block device information
echo "6. Block Device Information:"
send_monitor_cmd "info block"

# 7. Get network information
echo "7. Network Information:"
send_monitor_cmd "info network"

# 8. Get PCI device information
echo "8. PCI Device Information:"
send_monitor_cmd "info pci"

# 9. Get QTree information
echo "9. Device Tree Information:"
send_monitor_cmd "info qtree"

# 10. Get history
echo "10. Command History:"
send_monitor_cmd "info history"

echo ""
echo "=== Memory Analysis ==="

# 11. Memory dump around known addresses
echo "11. Memory dump around PL011 UART base (0x09000000):"
send_monitor_cmd "xp /16i 0x09000000"

# 12. Memory dump around GIC distributor (0x08000000)
echo "12. Memory dump around GIC distributor (0x08000000):"
send_monitor_cmd "xp /16i 0x08000000"

# 13. Memory dump around RAM base (0x40000000)
echo "13. Memory dump around RAM base (0x40000000):"
send_monitor_cmd "xp /16i 0x40000000"

# 14. Memory dump around potential kernel load area (0x48000000)
echo "14. Memory dump around kernel load area (0x48000000):"
send_monitor_cmd "xp /16i 0x48000000"

echo ""
echo "=== Advanced Monitor Commands ==="

# 15. Get boot time information
echo "15. Boot Time Information:"
send_monitor_cmd "info jit"

# 16. Get KVM information (if available)
echo "16. KVM Information:"
send_monitor_cmd "info kvm" || echo "KVM not available"

# 17. Get profiling information
echo "17. Profiling Information:"
send_monitor_cmd "info profile" || echo "Profile not available"

# 18. Get status information
echo "18. Status Information:"
send_monitor_cmd "info status"

echo ""
echo "=== Memory Dump Creation ==="

# 19. Create memory dump
echo "19. Creating memory dump..."
send_monitor_cmd "dump-guest-memory $MEMORY_DUMP_FILE"

# 20. Wait a bit more for boot to progress
echo "20. Waiting for boot to progress..."
sleep 5

# 21. Check registers again after boot attempt
echo "21. Registers after boot attempt:"
send_monitor_cmd "info registers"

# 22. Check memory at UART again
echo "22. UART memory after boot attempt:"
send_monitor_cmd "xp /16i 0x09000000"

echo ""
echo "=== Cleanup ==="

# 23. Graceful shutdown
echo "23. Shutting down QEMU..."
send_monitor_cmd "quit" || kill $QEMU_PID 2>/dev/null || true

# Wait for QEMU to exit
wait $QEMU_PID 2>/dev/null || true

echo ""
echo "=== Analysis Results ==="

# Analyze memory dump if created
if [ -f "$MEMORY_DUMP_FILE" ]; then
    echo "✅ Memory dump created: $MEMORY_DUMP_FILE"
    echo "   File size: $(wc -c < "$MEMORY_DUMP_FILE") bytes"
    
    # Look for interesting patterns in memory dump
    echo "   Searching for 'axion' string in memory dump..."
    if strings "$MEMORY_DUMP_FILE" | grep -i axion; then
        echo "   ✅ Found 'axion' strings in memory dump"
    else
        echo "   ❌ No 'axion' strings found in memory dump"
    fi
    
    echo "   Searching for 'TernaryOS' string in memory dump..."
    if strings "$MEMORY_DUMP_FILE" | grep -i ternaryos; then
        echo "   ✅ Found 'TernaryOS' strings in memory dump"
    else
        echo "   ❌ No 'TernaryOS' strings found in memory dump"
    fi
    
    echo "   Searching for 'BOOTAA64' string in memory dump..."
    if strings "$MEMORY_DUMP_FILE" | grep -i bootaa64; then
        echo "   ✅ Found 'BOOTAA64' strings in memory dump"
    else
        echo "   ❌ No 'BOOTAA64' strings found in memory dump"
    fi
else
    echo "❌ Memory dump not created"
fi

# Analyze debug log
if [ -f "/tmp/qemu_debug.log" ]; then
    echo ""
    echo "=== Debug Log Analysis ==="
    echo "Debug log size: $(wc -c < /tmp/qemu_debug.log) bytes"
    
    echo "Last 20 lines of debug log:"
    tail -20 /tmp/qemu_debug.log
    
    echo "Searching for 'axion' in debug log..."
    if grep -i axion /tmp/qemu_debug.log; then
        echo "✅ Found 'axion' references in debug log"
    else
        echo "❌ No 'axion' references in debug log"
    fi
else
    echo "❌ Debug log not created"
fi

# Cleanup
rm -f "/tmp/qemu_monitor_test.img" "$MEMORY_DUMP_FILE" "/tmp/qemu_debug.log"

echo ""
echo "=== QEMU Monitor Integration Test Complete ==="
echo "This test provides comprehensive state inspection without relying on serial output."
