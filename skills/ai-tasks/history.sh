#!/bin/bash
# Symlink to main AI tasks script for history
exec "$(dirname "$0")/assess.sh" "$@"
