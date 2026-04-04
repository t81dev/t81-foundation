#!/bin/bash
# Symlink to main debug script for stepping
exec "$(dirname "$0")/debug.sh" "$@"
