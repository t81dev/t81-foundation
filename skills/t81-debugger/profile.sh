#!/bin/bash
# Symlink to main debug script for profiling
exec "$(dirname "$0")/debug.sh" "$@"
