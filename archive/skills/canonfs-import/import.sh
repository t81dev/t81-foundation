#!/bin/bash

# T81 CanonFS Import Skill Wrapper
# Bridges OpenClaw skill to T81 CLI

set -euo pipefail

# Default values
CANONFS_ROOT="${T81_CANONFS_ROOT:-$HOME/.t81_canonfs}"
POLICY_FILE=""
JSON_OUTPUT=""
VERBOSE=""

# Parse arguments
FILE_PATH=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --canonfs-root)
            ((i++))
            CANONFS_ROOT="${ARGS[$i]}"
            ;;
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --json)
            JSON_OUTPUT="--json"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$FILE_PATH" ]; then
                FILE_PATH="${ARGS[$i]}"
            else
                echo "Error: Multiple files specified. Use import-batch for multiple files." >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate input
if [ -z "$FILE_PATH" ]; then
    echo "Error: No file path specified" >&2
    echo "Usage: import <file_path> [--canonfs-root <path>] [--policy <policy_file>] [--json]" >&2
    exit 1
fi

if [ ! -f "$FILE_PATH" ]; then
    echo "Error: File not found: $FILE_PATH" >&2
    exit 1
fi

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Create CanonFS root if it doesn't exist
mkdir -p "$CANONFS_ROOT"

# Build T81 command
T81_CMD="t81 canonfs import"
T81_CMD="$T81_CMD \"$FILE_PATH\""
T81_CMD="$T81_CMD --canonfs-root \"$CANONFS_ROOT\""

if [ -n "$POLICY_FILE" ]; then
    if [ ! -f "$POLICY_FILE" ]; then
        echo "Error: Policy file not found: $POLICY_FILE" >&2
        exit 1
    fi
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$JSON_OUTPUT" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 import
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Import completed successfully" >&2
    fi
else
    echo "Error: Import failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
