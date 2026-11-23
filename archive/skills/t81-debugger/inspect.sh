#!/bin/bash
# Symlink to main debug script for state inspection
exec "$(dirname "$0")/debug.sh" "$@"
