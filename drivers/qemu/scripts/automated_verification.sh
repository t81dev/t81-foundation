#!/usr/bin/env bash
# Five bounded QEMU checks; the worker owns fresh results and child lifetime.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/verification.py" "$@"
