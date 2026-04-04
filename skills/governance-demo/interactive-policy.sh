#!/bin/bash
# Symlink to main demo script for interactive policy testing
exec "$(dirname "$0")/demo-governance.sh" "$@"
