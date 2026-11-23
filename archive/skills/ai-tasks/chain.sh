#!/bin/bash
# Symlink to main AI tasks script for chain execution
exec "$(dirname "$0")/assess.sh" "$@"
