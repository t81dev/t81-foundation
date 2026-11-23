#!/bin/bash
# Symlink to main export script for searching
exec "$(dirname "$0")/export.sh" "$@"
