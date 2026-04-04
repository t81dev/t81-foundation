#!/bin/bash
# Symlink to main execution script for inspection mode
exec "$(dirname "$0")/run.sh" "$@"
