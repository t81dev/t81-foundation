#!/bin/bash
# Symlink to main tensor operations script for reshaping
exec "$(dirname "$0")/tensor-math.sh" "$@"
