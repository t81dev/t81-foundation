#!/bin/bash

# Symlink to main validation script for provenance verification
exec "$(dirname "$0")/validate.sh" "$@"
