#!/bin/bash
# Symlink to main bundle creator script for bundle verification
exec "$(dirname "$0")/create-bundle.sh" "$@"
