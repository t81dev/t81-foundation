#!/bin/bash
# Symlink to main model loader script for GGUF loading
exec "$(dirname "$0")/load-hf.sh" "$@"
