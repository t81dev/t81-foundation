#!/bin/bash
# Symlink to main export script for batch operations
exec "$(dirname "$0")/export.sh" "$@"
