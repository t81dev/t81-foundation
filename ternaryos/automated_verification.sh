#!/usr/bin/env bash
# Compatibility entry point for the canonical verification harness.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec bash "$SCRIPT_DIR/../drivers/qemu/scripts/automated_verification.sh" "$@"
