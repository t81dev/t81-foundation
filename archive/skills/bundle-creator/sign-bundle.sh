#!/bin/bash
# Symlink to main bundle creator script for bundle signing
exec "$(dirname "$0")/create-bundle.sh" "$@"
