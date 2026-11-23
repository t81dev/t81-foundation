#!/bin/bash

# T81 Compiler Skill Wrapper
# Bridges OpenClaw skill to T81 compiler

set -euo pipefail

# Default values
OUTPUT_FILE=""
OPTIMIZE_LEVEL=""
OPTIMIZATION="space"
TARGET_ARCH="t81v1"
FORMAT="binary"
ADDRESSES=""
PRESERVE_SEMANTICS=""
OUTPUT_DIR=""
RECURSIVE=""
METRICS=""
COMPLEXITY=""
POLICY_CHECK=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
SOURCE_FILE=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --output)
            ((i++))
            OUTPUT_FILE="${ARGS[$i]}"
            ;;
        --optimize)
            ((i++))
            OPTIMIZE_LEVEL="${ARGS[$i]}"
            ;;
        --optimization)
            ((i++))
            OPTIMIZATION="${ARGS[$i]}"
            ;;
        --target)
            ((i++))
            TARGET_ARCH="${ARGS[$i]}"
            ;;
        --format)
            ((i++))
            FORMAT="${ARGS[$i]}"
            ;;
        --addresses)
            ADDRESSES="--addresses"
            ;;
        --preserve-semantics)
            PRESERVE_SEMANTICS="--preserve-semantics"
            ;;
        --output-dir)
            ((i++))
            OUTPUT_DIR="${ARGS[$i]}"
            ;;
        --recursive)
            RECURSIVE="--recursive"
            ;;
        --metrics)
            METRICS="--metrics"
            ;;
        --complexity)
            COMPLEXITY="--complexity"
            ;;
        --policy-check)
            POLICY_CHECK="--policy-check"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$SOURCE_FILE" ]; then
                SOURCE_FILE="${ARGS[$i]}"
            else
                echo "Error: Multiple source files specified" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate input
if [ -z "$SOURCE_FILE" ]; then
    echo "Error: No source file specified" >&2
    echo "Usage: build <source_file> [--output <output_file>] [--optimize level] [--json]" >&2
    exit 1
fi

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file not found: $SOURCE_FILE" >&2
    exit 1
fi

# Validate optimization level
if [ -n "$OPTIMIZE_LEVEL" ]; then
    case "$OPTIMIZE_LEVEL" in
        1|2|3)
            ;;
        *)
            echo "Error: Invalid optimization level: $OPTIMIZE_LEVEL. Use 1, 2, or 3" >&2
            exit 1
            ;;
    esac
fi

# Validate optimization strategy
case "$OPTIMIZATION" in
    space|time|determinism)
        ;;
    *)
        echo "Error: Invalid optimization strategy: $OPTIMIZATION. Use 'space', 'time', or 'determinism'" >&2
        exit 1
        ;;
esac

# Validate format
case "$FORMAT" in
    binary|ternary|mixed)
        ;;
    *)
        echo "Error: Invalid format: $FORMAT. Use 'binary', 'ternary', or 'mixed'" >&2
        exit 1
        ;;
esac

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Determine command based on script name
case "$(basename "$0")" in
    build.sh)
        T81_CMD="t81 code build"
        ;;
    compile.sh)
        T81_CMD="t81 code compile"
        ;;
    disasm.sh)
        T81_CMD="t81 code disasm"
        ;;
    optimize.sh)
        T81_CMD="t81 code optimize"
        ;;
    build-batch.sh)
        T81_CMD="t81 code build-batch"
        ;;
    analyze.sh)
        T81_CMD="t81 code analyze"
        ;;
    *)
        echo "Error: Unknown compiler script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
T81_CMD="$T81_CMD \"$SOURCE_FILE\""

if [ -n "$OUTPUT_FILE" ] && [ "$(basename "$0")" != "build-batch.sh" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$OPTIMIZE_LEVEL" ] && [ "$(basename "$0")" = "build.sh" ]; then
    T81_CMD="$T81_CMD --optimize $OPTIMIZE_LEVEL"
fi

if [ -n "$OPTIMIZATION" ] && [ "$(basename "$0")" = "compile.sh" ]; then
    T81_CMD="$T81_CMD --optimization $OPTIMIZATION"
fi

if [ -n "$TARGET_ARCH" ] && [ "$(basename "$0")" = "compile.sh" ]; then
    T81_CMD="$T81_CMD --target $TARGET_ARCH"
fi

if [ -n "$FORMAT" ] && [ "$(basename "$0")" = "disasm.sh" ]; then
    T81_CMD="$T81_CMD --format $FORMAT"
fi

if [ -n "$ADDRESSES" ] && [ "$(basename "$0")" = "disasm.sh" ]; then
    T81_CMD="$T81_CMD $ADDRESSES"
fi

if [ -n "$PRESERVE_SEMANTICS" ] && [ "$(basename "$0")" = "optimize.sh" ]; then
    T81_CMD="$T81_CMD $PRESERVE_SEMANTICS"
fi

if [ -n "$OUTPUT_DIR" ] && [ "$(basename "$0")" = "build-batch.sh" ]; then
    mkdir -p "$OUTPUT_DIR"
    T81_CMD="$T81_CMD --output-dir \"$OUTPUT_DIR\""
fi

if [ -n "$RECURSIVE" ] && [ "$(basename "$0")" = "build-batch.sh" ]; then
    T81_CMD="$T81_CMD $RECURSIVE"
fi

if [ -n "$METRICS" ] && [ "$(basename "$0")" = "analyze.sh" ]; then
    T81_CMD="$T81_CMD $METRICS"
fi

if [ -n "$COMPLEXITY" ] && [ "$(basename "$0")" = "analyze.sh" ]; then
    T81_CMD="$T81_CMD $COMPLEXITY"
fi

if [ -n "$POLICY_CHECK" ] && [ "$(basename "$0")" = "analyze.sh" ]; then
    T81_CMD="$T81_CMD $POLICY_CHECK"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 compiler
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Compilation completed successfully" >&2
    fi
else
    echo "Error: Compilation failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
