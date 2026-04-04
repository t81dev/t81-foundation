#!/bin/bash
# Symlink to main debug script for tracing
exec "$(dirname "$0")/debug.sh" "$@"
