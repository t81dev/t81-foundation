#!/bin/bash
# Symlink to main tensor operations script for validation
exec "$(dirname "$0")/tensor-math.sh" "$@"
