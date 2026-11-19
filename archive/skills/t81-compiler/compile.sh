#!/bin/bash
# Symlink to main compiler script for compilation mode
exec "$(dirname "$0")/build.sh" "$@"
