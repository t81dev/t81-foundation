#!/bin/bash

# Symlink to main validation script for determinism checking
exec "$(dirname "$0")/validate.sh" "$@"
