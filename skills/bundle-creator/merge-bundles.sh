#!/bin/bash
# Symlink to main bundle creator script for bundle merging
exec "$(dirname "$0")/create-bundle.sh" "$@"
