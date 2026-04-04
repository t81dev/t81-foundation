#!/bin/bash
# Symlink to main export script for metadata export
exec "$(dirname "$0")/export.sh" "$@"
