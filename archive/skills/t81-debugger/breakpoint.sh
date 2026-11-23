#!/bin/bash
# Symlink to main debug script for breakpoint management
exec "$(dirname "$0")/debug.sh" "$@"
