#!/bin/bash
# Symlink to main model loader script for SafeTensors loading
exec "$(dirname "$0")/load-hf.sh" "$@"
