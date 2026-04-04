#!/bin/bash
# Symlink to main demo script for determinism demonstrations
exec "$(dirname "$0")/demo-governance.sh" "$@"
