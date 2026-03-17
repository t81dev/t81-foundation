#!/bin/bash
# Production launch script for local deployment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PRODUCTION_DIR/build"

echo "=== Launching Production TernaryOS ==="

# Build disk image
cd "$PRODUCTION_DIR/../"
./experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh "$BUILD_DIR" "$PRODUCTION_DIR/production_image"

# Launch QEMU
echo "Starting QEMU with TernaryOS..."
qemu-system-aarch64 -machine virt \
    -bios /opt/homebrew/share/qemu/edk2-aarch64-code.fd \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -m 256M -smp 2

echo "QEMU session ended"
