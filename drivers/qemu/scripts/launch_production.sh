#!/usr/bin/env bash
# Production launch script for local deployment

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PRODUCTION_DIR/build"

find_edk2_code() {
    local candidates=(
        /usr/share/AAVMF/AAVMF_CODE.fd
        /usr/share/qemu/edk2-aarch64-code.fd
        /opt/homebrew/share/qemu/edk2-aarch64-code.fd
        /usr/local/share/qemu/edk2-aarch64-code.fd
    )
    local f
    for f in "${candidates[@]}"; do
        if [[ -f "$f" ]]; then
            echo "$f"
            return
        fi
    done
    echo "Error: EDK2 AArch64 firmware not found. Install qemu-efi-aarch64 (Linux) or qemu via Homebrew (macOS)." >&2
    exit 1
}

if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
    echo "Error: qemu-system-aarch64 not found. Install QEMU first." >&2
    exit 1
fi

EDK2_CODE="$(find_edk2_code)"

echo "=== Launching Production TernaryOS ==="

# Build disk image
cd "$PRODUCTION_DIR/../"
./experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh "$BUILD_DIR" "$PRODUCTION_DIR/production_image"

# Launch QEMU
echo "Starting QEMU with TernaryOS..."
qemu-system-aarch64 -machine virt \
    -bios "$EDK2_CODE" \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null -monitor none \
    -m 256M -smp 2

echo "QEMU session ended"
