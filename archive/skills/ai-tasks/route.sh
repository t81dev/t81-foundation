#!/bin/bash
# Symlink to main AI tasks script for routing
exec "$(dirname "$0")/assess.sh" "$@"
