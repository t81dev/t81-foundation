#!/usr/bin/env bash
# Verify the deployed image, forwarding the authoritative worker exit status.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export QEMU_IMG="${QEMU_IMG:-$SCRIPT_DIR/../production_image/qemu_slice6_guest.img}"
exec bash "$SCRIPT_DIR/automated_verification.sh" "$@"
