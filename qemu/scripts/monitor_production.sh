#!/bin/bash
# Production monitoring script for local deployment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Production Monitoring ==="

# Start QEMU with monitor
cd "$PRODUCTION_DIR/../"
./experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh "$BUILD_DIR" "$PRODUCTION_DIR/production_image"

qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null \
    -monitor telnet:localhost:1234,server,nowait,wait &

QEMU_PID=$!

echo "QEMU started with PID: $QEMU_PID"
echo "Monitor available on localhost:1234"
echo "Connect with: telnet localhost 1234"
echo ""
echo "Available monitor commands:"
echo "  info registers    - Show CPU registers"
echo "  info cpus         - Show CPU information"
echo "  info memory       - Show memory information"
echo "  xp /16i 0x09000000 - Show UART memory"
echo "  dump-guest-memory /tmp/memory.dump - Dump memory"
echo "  quit             - Stop QEMU"
echo ""
echo "Press Ctrl+C to stop monitoring and shutdown QEMU"

# Wait for interrupt or user to stop
trap 'echo "Stopping QEMU..."; kill $QEMU_PID 2>/dev/null || true; wait $QEMU_PID 2>/dev/null || true; echo "QEMU stopped"; exit 0' INT

# Keep script running
while kill -0 $QEMU_PID 2>/dev/null; do
    sleep 1
done

echo "QEMU process ended"
