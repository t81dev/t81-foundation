#!/bin/bash
# Symlink to main bundle creator script for bundle extraction
exec "$(dirname "$0")/create-bundle.sh" "$@"
