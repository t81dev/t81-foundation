#!/bin/bash
# Symlink to main export script for verification
exec "$(dirname "$0")/export.sh" "$@"
