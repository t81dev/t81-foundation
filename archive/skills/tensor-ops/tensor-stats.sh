#!/bin/bash
# Symlink to main tensor operations script for statistics
exec "$(dirname "$0")/tensor-math.sh" "$@"
