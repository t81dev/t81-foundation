#!/bin/bash
# Symlink to main model loader script for model probing
exec "$(dirname "$0")/load-hf.sh" "$@"
