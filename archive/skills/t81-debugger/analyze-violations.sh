#!/bin/bash
# Symlink to main debug script for violation analysis
exec "$(dirname "$0")/debug.sh" "$@"
