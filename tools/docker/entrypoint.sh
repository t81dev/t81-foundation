#!/usr/bin/env sh
# docker/entrypoint.sh
#
# Routes container invocations:
#   (no args)  → interactive REPL
#   demo       → guided 60-second demo, then REPL
#   <anything> → passed directly to the t81 CLI

set -e

case "${1:-}" in
  "")
    exec t81 repl
    ;;
  demo)
    exec /t81/docker/demo.sh
    ;;
  *)
    exec t81 "$@"
    ;;
esac
