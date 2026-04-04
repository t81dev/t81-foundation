#!/bin/bash

# T81 CanonFS Export Skill Wrapper
# Bridges OpenClaw skill to T81 CanonFS export

set -euo pipefail

# Default values
CANONFS_ROOT="${T81_CANONFS_ROOT:-$HOME/.t81_canonfs}"
OUTPUT_FILE=""
VERIFY=""
OUTPUT_DIR=""
EXPORTED_FILE=""
FORMAT="table"
FILTER=""
FIELD="name"
MAX_RESULTS=50
INCLUDE_PROVENANCE=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
CANON_HASH=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --output)
            ((i++))
            OUTPUT_FILE="${ARGS[$i]}"
            ;;
        --canonfs-root)
            ((i++))
            CANONFS_ROOT="${ARGS[$i]}"
            ;;
        --verify)
            VERIFY="--verify"
            ;;
        --output-dir)
            ((i++))
            OUTPUT_DIR="${ARGS[$i]}"
            ;;
        --exported-file)
            ((i++))
            EXPORTED_FILE="${ARGS[$i]}"
            ;;
        --format)
            ((i++))
            FORMAT="${ARGS[$i]}"
            ;;
        --filter)
            ((i++))
            FILTER="${ARGS[$i]}"
            ;;
        --field)
            ((i++))
            FIELD="${ARGS[$i]}"
            ;;
        --max-results)
            ((i++))
            MAX_RESULTS="${ARGS[$i]}"
            ;;
        --include-provenance)
            INCLUDE_PROVENANCE="--include-provenance"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$CANON_HASH" ]; then
                CANON_HASH="${ARGS[$i]}"
            elif [ "$(basename "$0")" = "export-batch.sh" ]; then
                # For batch export, collect multiple hashes
                CANON_HASH="$CANON_HASH ${ARGS[$i]}"
            else
                echo "Error: Too many arguments" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate format
case "$FORMAT" in
    json|table)
        ;;
    *)
        echo "Error: Invalid format: $FORMAT. Use 'json' or 'table'" >&2
        exit 1
        ;;
esac

# Validate field
case "$FIELD" in
    name|hash|type|size|date)
        ;;
    *)
        echo "Error: Invalid field: $FIELD. Use 'name', 'hash', 'type', 'size', or 'date'" >&2
        exit 1
        ;;
esac

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Validate required arguments based on command
case "$(basename "$0")" in
    export.sh|export-with-meta.sh)
        if [ -z "$CANON_HASH" ]; then
            echo "Error: No CanonHash81 specified" >&2
            echo "Usage: export <canon_hash> [--output <output_file>] [--canonfs-root <path>] [--verify]" >&2
            exit 1
        fi
        ;;
    export-batch.sh)
        if [ -z "$CANON_HASH" ]; then
            echo "Error: No CanonHash81 values specified" >&2
            echo "Usage: export-batch <hash1> <hash2> ... [--output-dir <dir>] [--canonfs-root <path>]" >&2
            exit 1
        fi
        ;;
    verify-export.sh)
        if [ -z "$CANON_HASH" ]; then
            echo "Error: No CanonHash81 specified" >&2
            echo "Usage: verify-export <canon_hash> [--exported-file <file>] [--canonfs-root <path>]" >&2
            exit 1
        fi
        if [ -n "$EXPORTED_FILE" ] && [ ! -f "$EXPORTED_FILE" ]; then
            echo "Error: Exported file not found: $EXPORTED_FILE" >&2
            exit 1
        fi
        ;;
    search.sh)
        if [ -z "$CANON_HASH" ]; then
            echo "Error: No search query specified" >&2
            echo "Usage: search <query> [--canonfs-root <path>] [--field name|hash|type] [--max-results <count>]" >&2
            exit 1
        fi
        ;;
    list.sh)
        # No required arguments for list
        ;;
esac

# Create CanonFS root if it doesn't exist
mkdir -p "$CANONFS_ROOT"

# Create output directory if specified
if [ -n "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
fi

# Determine command based on script name
case "$(basename "$0")" in
    export.sh)
        T81_CMD="t81 canonfs export"
        ;;
    export-batch.sh)
        T81_CMD="t81 canonfs export-batch"
        ;;
    verify-export.sh)
        T81_CMD="t81 canonfs verify-export"
        ;;
    list.sh)
        T81_CMD="t81 canonfs list"
        ;;
    search.sh)
        T81_CMD="t81 canonfs search"
        ;;
    export-with-meta.sh)
        T81_CMD="t81 canonfs export-with-meta"
        ;;
    *)
        echo "Error: Unknown export script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$CANON_HASH" ] && [ "$(basename "$0")" != "list.sh" ]; then
    T81_CMD="$T81_CMD $CANON_HASH"
fi

if [ -n "$OUTPUT_FILE" ] && [ "$(basename "$0")" != "export-batch.sh" ] && [ "$(basename "$0")" != "list.sh" ] && [ "$(basename "$0")" != "search.sh" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$CANONFS_ROOT" ]; then
    T81_CMD="$T81_CMD --canonfs-root \"$CANONFS_ROOT\""
fi

if [ -n "$VERIFY" ] && [ "$(basename "$0")" = "export.sh" ]; then
    T81_CMD="$T81_CMD $VERIFY"
fi

if [ -n "$OUTPUT_DIR" ] && [ "$(basename "$0")" = "export-batch.sh" ]; then
    T81_CMD="$T81_CMD --output-dir \"$OUTPUT_DIR\""
fi

if [ -n "$EXPORTED_FILE" ] && [ "$(basename "$0")" = "verify-export.sh" ]; then
    T81_CMD="$T81_CMD --exported-file \"$EXPORTED_FILE\""
fi

if [ -n "$FORMAT" ] && [ "$(basename "$0")" = "list.sh" ]; then
    T81_CMD="$T81_CMD --format $FORMAT"
fi

if [ -n "$FILTER" ] && [ "$(basename "$0")" = "list.sh" ]; then
    T81_CMD="$T81_CMD --filter \"$FILTER\""
fi

if [ -n "$FIELD" ] && [ "$(basename "$0")" = "search.sh" ]; then
    T81_CMD="$T81_CMD --field $FIELD"
fi

if [ -n "$MAX_RESULTS" ] && [ "$(basename "$0")" = "search.sh" ]; then
    T81_CMD="$T81_CMD --max-results $MAX_RESULTS"
fi

if [ -n "$INCLUDE_PROVENANCE" ] && [ "$(basename "$0")" = "export-with-meta.sh" ]; then
    T81_CMD="$T81_CMD $INCLUDE_PROVENANCE"
fi

# Always output JSON for machine readability (except for list.sh which has format option)
if [ "$JSON_OUTPUT" = "--json" ] && [ "$(basename "$0")" != "list.sh" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 CanonFS export
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "CanonFS export completed successfully" >&2
    fi
else
    echo "Error: CanonFS export failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
