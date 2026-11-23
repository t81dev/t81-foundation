#!/bin/bash

# T81 Bundle Validator Skill Wrapper
# Bridges OpenClaw skill to T81 bundle validation

set -euo pipefail

# Default values
STRICT=""
POLICY_FILE=""
OUTPUT_FORMAT="summary"
CANONFS_ROOT="${T81_CANONFS_ROOT:-$HOME/.t81_canonfs}"
REPLAY_COUNT=1
VERBOSE=""

# Parse arguments
BUNDLE_FILE=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --strict)
            STRICT="--strict"
            ;;
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --output)
            ((i++))
            OUTPUT_FORMAT="${ARGS[$i]}"
            ;;
        --canonfs-root)
            ((i++))
            CANONFS_ROOT="${ARGS[$i]}"
            ;;
        --replay-count)
            ((i++))
            REPLAY_COUNT="${ARGS[$i]}"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$BUNDLE_FILE" ]; then
                BUNDLE_FILE="${ARGS[$i]}"
            else
                echo "Error: Multiple bundle files specified" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate input
if [ -z "$BUNDLE_FILE" ]; then
    echo "Error: No bundle file specified" >&2
    echo "Usage: validate <bundle_file> [--strict] [--policy <policy_file>] [--output json|summary]" >&2
    exit 1
fi

if [ ! -f "$BUNDLE_FILE" ]; then
    echo "Error: Bundle file not found: $BUNDLE_FILE" >&2
    exit 1
fi

# Validate output format
case "$OUTPUT_FORMAT" in
    json|summary)
        ;;
    *)
        echo "Error: Invalid output format: $OUTPUT_FORMAT. Use 'json' or 'summary'" >&2
        exit 1
        ;;
esac

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Validate policy file if specified
if [ -n "$POLICY_FILE" ] && [ ! -f "$POLICY_FILE" ]; then
    echo "Error: Policy file not found: $POLICY_FILE" >&2
    exit 1
fi

# Create CanonFS root if it doesn't exist
mkdir -p "$CANONFS_ROOT"

# Determine validation command based on file name pattern
case "$(basename "$0")" in
    validate.sh)
        T81_CMD="t81 bundle validate"
        ;;
    verify-provenance.sh)
        T81_CMD="t81 bundle verify-provenance"
        ;;
    check-determinism.sh)
        T81_CMD="t81 bundle check-determinism"
        ;;
    *)
        echo "Error: Unknown validation script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
T81_CMD="$T81_CMD \"$BUNDLE_FILE\""

if [ -n "$STRICT" ]; then
    T81_CMD="$T81_CMD $STRICT"
fi

if [ -n "$POLICY_FILE" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$CANONFS_ROOT" ]; then
    T81_CMD="$T81_CMD --canonfs-root \"$CANONFS_ROOT\""
fi

if [ "$OUTPUT_FORMAT" = "json" ]; then
    T81_CMD="$T81_CMD --json"
fi

if [ "$(basename "$0")" = "check-determinism.sh" ]; then
    T81_CMD="$T81_CMD --replay-count $REPLAY_COUNT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 validation
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Validation completed successfully" >&2
    fi
else
    echo "Error: Validation failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
