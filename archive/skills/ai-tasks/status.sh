#!/bin/bash
# Symlink to main AI tasks script for status checking
exec "$(dirname "$0")/assess.sh" "$@"
