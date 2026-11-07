#!/bin/bash
# Fixed-purpose policy metadata adapter; independent of the unsafe check.sh dispatcher.
set -euo pipefail

fail() {
    printf 'validate-policy: %s\n' "$1" >&2
    exit 2
}

policy_file=''
json_seen=false
while [ "$#" -gt 0 ]; do
    case "$1" in
        --json)
            [ "$json_seen" = false ] || fail 'duplicate --json'
            json_seen=true
            ;;
        --)
            shift
            [ "$#" -eq 1 ] && [ -z "$policy_file" ] || fail 'expected exactly one policy file after --'
            policy_file=$1
            break
            ;;
        -*) fail "unsupported option: $1" ;;
        *)
            [ -z "$policy_file" ] || fail 'expected exactly one policy file'
            [ -n "$1" ] || fail 'empty policy path'
            policy_file=$1
            ;;
    esac
    shift
done

[ -n "$policy_file" ] || fail 'usage: validate-policy [--json] [--] <file.apl|file.axionb>'
case "$policy_file" in
    *.apl|*.axionb) ;;
    *) fail 'policy path must end in .apl or .axionb' ;;
esac
[ -f "$policy_file" ] && [ -r "$policy_file" ] || fail 'policy must be an existing readable regular file'

# The native shared parser recognizes option-shaped tokens anywhere. Prefix relative
# paths so even a filename such as --help.apl remains a positional input to the CLI.
case "$policy_file" in
    /*) ;;
    *) policy_file="./$policy_file" ;;
esac
command -v t81 >/dev/null 2>&1 || { printf 'validate-policy: t81 not found on PATH\n' >&2; exit 127; }
cmd=(t81 policy validate "$policy_file" --json)
exec "${cmd[@]}"
