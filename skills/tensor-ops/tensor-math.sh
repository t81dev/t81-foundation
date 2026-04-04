#!/bin/bash

# T81 Tensor Operations Skill Wrapper
# Bridges OpenClaw skill to T81 tensor operations

set -euo pipefail

# Default values
OUTPUT_FILE=""
PRECISION="ternary"
NEW_SHAPE=""
CHECK_INTEGRITY=""
CHECK_DETERMINISM=""
TO_FORMAT=""
STATS_FILE=""
HISTOGRAM=""
DETAILED=""
OUTPUT_DIR=""
RECURSIVE=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
OPERATION=""
TENSOR_A=""
TENSOR_B=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --output)
            ((i++))
            OUTPUT_FILE="${ARGS[$i]}"
            ;;
        --precision)
            ((i++))
            PRECISION="${ARGS[$i]}"
            ;;
        --check-integrity)
            CHECK_INTEGRITY="--check-integrity"
            ;;
        --check-determinism)
            CHECK_DETERMINISM="--check-determinism"
            ;;
        --to-format)
            ((i++))
            TO_FORMAT="${ARGS[$i]}"
            ;;
        --stats-file)
            ((i++))
            STATS_FILE="${ARGS[$i]}"
            ;;
        --histogram)
            HISTOGRAM="--histogram"
            ;;
        --detailed)
            DETAILED="--detailed"
            ;;
        --output-dir)
            ((i++))
            OUTPUT_DIR="${ARGS[$i]}"
            ;;
        --recursive)
            RECURSIVE="--recursive"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$OPERATION" ]; then
                OPERATION="${ARGS[$i]}"
            elif [ -z "$TENSOR_A" ]; then
                TENSOR_A="${ARGS[$i]}"
            elif [ -z "$TENSOR_B" ] && [ "$(basename "$0")" = "tensor-math.sh" ]; then
                TENSOR_B="${ARGS[$i]}"
            elif [ -z "$TENSOR_B" ] && [ "$(basename "$0")" != "tensor-math.sh" ]; then
                TENSOR_B="${ARGS[$i]}"
            else
                echo "Error: Too many arguments" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate precision
case "$PRECISION" in
    ternary|binary)
        ;;
    *)
        echo "Error: Invalid precision: $PRECISION. Use 'ternary' or 'binary'" >&2
        exit 1
        ;;
esac

# Validate format
if [ -n "$TO_FORMAT" ]; then
    case "$TO_FORMAT" in
        t81w|safetensors|gguf)
            ;;
        *)
            echo "Error: Invalid format: $TO_FORMAT. Use 't81w', 'safetensors', or 'gguf'" >&2
            exit 1
            ;;
    esac
fi

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Validate required arguments based on command
case "$(basename "$0")" in
    tensor-math.sh)
        if [ -z "$OPERATION" ] || [ -z "$TENSOR_A" ] || [ -z "$TENSOR_B" ]; then
            echo "Error: tensor-math requires operation, tensor_a, and tensor_b" >&2
            echo "Usage: tensor-math <operation> <tensor_a> <tensor_b> [--output <output_file>] [--precision ternary|binary]" >&2
            exit 1
        fi
        if [ ! -f "$TENSOR_A" ]; then
            echo "Error: Tensor A not found: $TENSOR_A" >&2
            exit 1
        fi
        if [ ! -f "$TENSOR_B" ]; then
            echo "Error: Tensor B not found: $TENSOR_B" >&2
            exit 1
        fi
        ;;
    tensor-reshape.sh|tensor-validate.sh|tensor-convert.sh|tensor-stats.sh)
        if [ -z "$TENSOR_A" ]; then
            echo "Error: No tensor file specified" >&2
            echo "Usage: $(basename "$0") <tensor_file> [options]" >&2
            exit 1
        fi
        if [ ! -f "$TENSOR_A" ]; then
            echo "Error: Tensor file not found: $TENSOR_A" >&2
            exit 1
        fi
        ;;
    tensor-batch.sh)
        if [ -z "$OPERATION" ] || [ -z "$TENSOR_A" ]; then
            echo "Error: tensor-batch requires operation and tensor directory" >&2
            echo "Usage: tensor-batch <operation> <tensor_dir> [--recursive] [--output-dir <dir>]" >&2
            exit 1
        fi
        if [ ! -d "$TENSOR_A" ]; then
            echo "Error: Tensor directory not found: $TENSOR_A" >&2
            exit 1
        fi
        ;;
esac

# Validate new shape for reshape
if [ "$(basename "$0")" = "tensor-reshape.sh" ] && [ -z "$NEW_SHAPE" ]; then
    NEW_SHAPE="$TENSOR_B"
fi

# Determine command based on script name
case "$(basename "$0")" in
    tensor-math.sh)
        T81_CMD="t81 tensor math"
        ;;
    tensor-reshape.sh)
        T81_CMD="t81 tensor reshape"
        ;;
    tensor-validate.sh)
        T81_CMD="t81 tensor validate"
        ;;
    tensor-convert.sh)
        T81_CMD="t81 tensor convert"
        ;;
    tensor-stats.sh)
        T81_CMD="t81 tensor stats"
        ;;
    tensor-batch.sh)
        T81_CMD="t81 tensor batch"
        ;;
    *)
        echo "Error: Unknown tensor operation script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$OPERATION" ]; then
    T81_CMD="$T81_CMD $OPERATION"
fi

if [ -n "$TENSOR_A" ]; then
    T81_CMD="$T81_CMD \"$TENSOR_A\""
fi

if [ -n "$TENSOR_B" ] && [ "$(basename "$0")" = "tensor-math.sh" ]; then
    T81_CMD="$T81_CMD \"$TENSOR_B\""
fi

if [ -n "$NEW_SHAPE" ] && [ "$(basename "$0")" = "tensor-reshape.sh" ]; then
    T81_CMD="$T81_CMD \"$NEW_SHAPE\""
fi

if [ -n "$OUTPUT_FILE" ] && [ "$(basename "$0")" != "tensor-batch.sh" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$PRECISION" ] && [ "$(basename "$0")" = "tensor-math.sh" ]; then
    T81_CMD="$T81_CMD --precision $PRECISION"
fi

if [ -n "$CHECK_INTEGRITY" ] && [ "$(basename "$0")" = "tensor-validate.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_INTEGRITY"
fi

if [ -n "$CHECK_DETERMINISM" ] && [ "$(basename "$0")" = "tensor-validate.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_DETERMINISM"
fi

if [ -n "$TO_FORMAT" ] && [ "$(basename "$0")" = "tensor-convert.sh" ]; then
    T81_CMD="$T81_CMD --to-format $TO_FORMAT"
fi

if [ -n "$STATS_FILE" ] && [ "$(basename "$0")" = "tensor-stats.sh" ]; then
    T81_CMD="$T81_CMD --stats-file \"$STATS_FILE\""
fi

if [ -n "$HISTOGRAM" ] && [ "$(basename "$0")" = "tensor-stats.sh" ]; then
    T81_CMD="$T81_CMD $HISTOGRAM"
fi

if [ -n "$DETAILED" ] && [ "$(basename "$0")" = "tensor-stats.sh" ]; then
    T81_CMD="$T81_CMD $DETAILED"
fi

if [ -n "$OUTPUT_DIR" ] && [ "$(basename "$0")" = "tensor-batch.sh" ]; then
    mkdir -p "$OUTPUT_DIR"
    T81_CMD="$T81_CMD --output-dir \"$OUTPUT_DIR\""
fi

if [ -n "$RECURSIVE" ] && [ "$(basename "$0")" = "tensor-batch.sh" ]; then
    T81_CMD="$T81_CMD $RECURSIVE"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 tensor operation
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Tensor operation completed successfully" >&2
    fi
else
    echo "Error: Tensor operation failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
