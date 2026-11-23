#!/bin/bash
# Symlink to main execution script for debug mode
exec "$(dirname "$0")/run.sh" "$@"
