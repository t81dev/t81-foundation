#!/bin/bash
# Symlink to main compiler script for batch compilation
exec "$(dirname "$0")/build.sh" "$@"
