#!/bin/bash
# Symlink to main execution script for batch mode
exec "$(dirname "$0")/run.sh" "$@"
