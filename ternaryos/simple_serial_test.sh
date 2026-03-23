#!/bin/bash
# simple_serial_test.sh - Simple QEMU serial output test

set -e

echo "=== Simple QEMU Serial Test ==="

QEMU_IMG="build/qemu_test_debug/qemu_slice6_guest.img"
BIOS="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"

if [ ! -f "$QEMU_IMG" ]; then
    echo "Error: QEMU image not found: $QEMU_IMG"
    exit 1
fi

echo "Testing basic QEMU serial output..."

# Test 1: Basic stdio output
echo ""
echo "Test 1: Basic stdio output"
timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial stdio -monitor none 2>&1 | head -20 || echo "Timeout or error"

# Test 2: File output
echo ""
echo "Test 2: File output"
rm -f /tmp/qemu_serial_test.log
timeout 8 qemu-system-aarch64 -machine virt -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial "file:/tmp/qemu_serial_test.log" -monitor none &
QEMU_PID=$!
sleep 5
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

if [ -f "/tmp/qemu_serial_test.log" ]; then
    echo "File size: $(wc -c < /tmp/qemu_serial_test.log) bytes"
    if [ -s "/tmp/qemu_serial_test.log" ]; then
        echo "Content:"
        cat /tmp/qemu_serial_test.log
    else
        echo "File is empty"
    fi
else
    echo "File not created"
fi

# Test 3: Virtio-serial
echo ""
echo "Test 3: Virtio-serial device"
rm -f /tmp/qemu_virtio_test.log
timeout 8 qemu-system-aarch64 -machine virt \
    -device virtio-serial-device \
    -chardev file,id=serial0,path=/tmp/qemu_virtio_test.log \
    -device virtserialport,chardev=serial0 \
    -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -monitor none &
QEMU_PID=$!
sleep 5
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

if [ -f "/tmp/qemu_virtio_test.log" ]; then
    echo "Virtio-serial file size: $(wc -c < /tmp/qemu_virtio_test.log) bytes"
    if [ -s "/tmp/qemu_virtio_test.log" ]; then
        echo "Content:"
        cat /tmp/qemu_virtio_test.log
    else
        echo "Virtio-serial file is empty"
    fi
else
    echo "Virtio-serial file not created"
fi

# Test 4: Monitor test
echo ""
echo "Test 4: QEMU monitor"
mkfifo /tmp/qemu_mon_in /tmp/qemu_mon_out
timeout 8 qemu-system-aarch64 -machine virt \
    -bios "$BIOS" \
    -drive file="$QEMU_IMG",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -monitor pipe:/tmp/qemu_mon_in &
QEMU_PID=$!
sleep 2
echo "info version" > /tmp/qemu_mon_in &
sleep 3
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

echo "Monitor output:"
cat /tmp/qemu_mon_out 2>/dev/null || echo "No monitor output"

# Cleanup
rm -f /tmp/qemu_serial_test.log /tmp/qemu_virtio_test.log /tmp/qemu_mon_in /tmp/qemu_mon_out

echo ""
echo "=== Simple Serial Test Complete ==="
