#!/bin/bash
# Symlink to main bundle creator script for bundle information
exec "$(dirname "$0")/create-bundle.sh" "$@"
