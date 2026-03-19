#!/bin/bash
# test_qemu_serial.sh - Comprehensive QEMU serial output testing

set -e

echo "=== QEMU Serial Output Testing ==="
echo "Testing various QEMU serial configurations for TernaryOS"

# Test configurations
TESTS=(
    "stdio:Standard I/O"
    "file:/tmp/qemu_test.log:File Output"
    "pipe:/tmp/qemu_test_pipe:Named Pipe"
    "socket:localhost:5555:TCP Socket"
    "pty:Pseudo Terminal"
    "null:Null Device"
    "memory:Memory Buffer"
)

QEMU_IMG="build/qemu_test_debug/qemu_slice6_guest.img"
BIOS="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"

if [ ! -f "$QEMU_IMG" ]; then
    echo "Error: QEMU image not found: $QEMU_IMG"
    exit 1
fi

for test_config in "${TESTS[@]}"; do
    IFS=':' read -r backend target description <<< "$test_config"
    echo ""
    echo "Testing: $description ($backend:$target)"
    
    case $backend in
        "stdio")
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial stdio -monitor none 2>&1 | head -10 || true
            ;;
        "file")
            rm -f "$target"
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial "file:$target" -monitor none &
            QEMU_PID=$!
            sleep 5
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            if [ -f "$target" ]; then
                echo "File size: $(wc -c < "$target") bytes"
                if [ -s "$target" ]; then
                    echo "Content preview:"
                    head -5 "$target"
                else
                    echo "File is empty"
                fi
            else
                echo "File not created"
            fi
            rm -f "$target"
            ;;
        "pipe")
            rm -f "$target"
            mkfifo "$target"
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial "pipe:$target" -monitor none &
            QEMU_PID=$!
            sleep 3
            timeout 5 cat "$target" &
            CAT_PID=$!
            sleep 5
            kill $QEMU_PID 2>/dev/null || true
            kill $CAT_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            wait $CAT_PID 2>/dev/null || true
            rm -f "$target"
            ;;
        "socket")
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial "tcp:$target,server,nowait" -monitor none &
            QEMU_PID=$!
            sleep 3
            timeout 5 nc localhost 5555 2>/dev/null || echo "Connection failed"
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            ;;
        "pty")
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial pty -monitor none &
            QEMU_PID=$!
            sleep 5
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            ;;
        "null")
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial null -monitor none &
            QEMU_PID=$!
            sleep 5
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            echo "Null device test completed"
            ;;
        "memory")
            timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
                -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
                -device virtio-blk-device,drive=hd0 \
                -nographic -serial "chardev:memory,size=1M" -monitor none &
            QEMU_PID=$!
            sleep 5
            kill $QEMU_PID 2>/dev/null || true
            wait $QEMU_PID 2>/dev/null || true
            echo "Memory buffer test completed"
            ;;
    esac
    
    echo "Test completed"
done

echo ""
echo "=== Testing with virtio-serial device ==="

timeout 8 qemu-system-aarch64 -machine virt \
    -device virtio-serial-device \
    -chardev file,id=serial0,path=/tmp/qemu_virtio.log \
    -device virtserialport,chardev=serial0 \
    -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -monitor none &
QEMU_PID=$!
sleep 5
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

if [ -f "/tmp/qemu_virtio.log" ]; then
    echo "Virtio-serial log size: $(wc -c < /tmp/qemu_virtio.log) bytes"
    if [ -s "/tmp/qemu_virtio.log" ]; then
        echo "Content preview:"
        head -5 "/tmp/qemu_virtio.log"
    else
        echo "Virtio-serial log is empty"
    fi
else
    echo "Virtio-serial log not created"
fi

rm -f /tmp/qemu_virtio.log

echo ""
echo "=== Testing with QEMU monitor ==="

mkfifo /tmp/qemu_monitor_in /tmp/qemu_monitor_out
timeout 8 qemu-system-aarch64 -machine virt \
    -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -monitor pipe:/tmp/qemu_monitor_in &
QEMU_PID=$!
sleep 2
echo "info version" > /tmp/qemu_monitor_in &
sleep 3
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

echo "Monitor output:"
cat /tmp/qemu_monitor_out 2>/dev/null || echo "No monitor output"

rm -f /tmp/qemu_monitor_in /tmp/qemu_monitor_out

echo ""
echo "=== QEMU Serial Testing Complete ==="
