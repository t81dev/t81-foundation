#!/bin/bash
# Symlink to main execution script for trace mode
exec "$(dirname "$0")/run.sh" "$@"
