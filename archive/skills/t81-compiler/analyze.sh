#!/bin/bash
# Symlink to main compiler script for analysis mode
exec "$(dirname "$0")/build.sh" "$@"
