#!/bin/bash
# Symlink to main AI tasks script for classification
exec "$(dirname "$0")/assess.sh" "$@"
