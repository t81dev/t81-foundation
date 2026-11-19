#!/bin/bash
# Symlink to main model loader script for batch loading
exec "$(dirname "$0")/load-hf.sh" "$@"
