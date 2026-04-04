#!/bin/bash
# Symlink to main tensor operations script for batch processing
exec "$(dirname "$0")/tensor-math.sh" "$@"
