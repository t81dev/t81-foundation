#!/bin/bash
# Symlink to main compiler script for disassembly mode
exec "$(dirname "$0")/build.sh" "$@"
