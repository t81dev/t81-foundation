#!/bin/bash
# Symlink to main tensor operations script for conversion
exec "$(dirname "$0")/tensor-math.sh" "$@"
