#!/bin/bash
# Symlink to main debug script for continue execution
exec "$(dirname "$0")/debug.sh" "$@"
