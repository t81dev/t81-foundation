#!/bin/bash
# Symlink to main bundle creator script for execution bundling
exec "$(dirname "$0")/create-bundle.sh" "$@"
