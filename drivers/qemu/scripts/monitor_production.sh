#!/usr/bin/env bash
# Production monitoring script for local deployment

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRODUCTION_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PRODUCTION_DIR/build"
MONITOR_PORT="${MONITOR_PORT:-1234}"

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

echo "=== Production Monitoring ==="

# Start QEMU with monitor
cd "$PRODUCTION_DIR/../"
./experimental/ternaryos/scripts/build_qemu_slice6_artifact.sh "$BUILD_DIR" "$PRODUCTION_DIR/production_image"

qemu-system-aarch64 -machine virt \
    -bios "$EDK2_CODE" \
    -drive file="$PRODUCTION_DIR/production_image/qemu_slice6_guest.img",format=raw,if=none,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -nographic -serial null \
    -monitor "telnet:localhost:${MONITOR_PORT},server,nowait,wait" &

QEMU_PID=$!

echo "QEMU started with PID: $QEMU_PID"
echo "Monitor available on localhost:${MONITOR_PORT}"
echo "Connect with: telnet localhost ${MONITOR_PORT}"
echo ""
echo "Available monitor commands:"
echo "  info registers    - Show CPU registers"
echo "  info cpus         - Show CPU information"
echo "  info memory       - Show memory information"
echo "  xp /16i 0x09000000 - Show UART memory"
echo "  dump-guest-memory \${TMPDIR:-/tmp}/memory.dump - Dump memory"
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
