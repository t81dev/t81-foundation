#!/bin/bash
# Symlink to main model loader script for model conversion
exec "$(dirname "$0")/load-hf.sh" "$@"
