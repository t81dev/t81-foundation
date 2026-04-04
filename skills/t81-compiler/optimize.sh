#!/bin/bash
# Symlink to main compiler script for optimization mode
exec "$(dirname "$0")/build.sh" "$@"
