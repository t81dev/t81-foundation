#!/bin/bash
# Symlink to main model loader script for model validation
exec "$(dirname "$0")/load-hf.sh" "$@"
