#!/bin/bash
# Symlink to main export script for listing
exec "$(dirname "$0")/export.sh" "$@"
