#!/bin/bash

# T81 Model Loader Skill Wrapper
# Bridges OpenClaw skill to T81 model loading

set -euo pipefail

# Default values
OUTPUT_FILE=""
CANONFS_ROOT="${T81_CANONFS_ROOT:-$HOME/.t81_canonfs}"
POLICY_FILE=""
DETAILED=""
LAYER_INFO=""
PROBE_FILE=""
CHECK_DETERMINISM=""
SECURITY_SCAN=""
REPORT_FILE=""
TO_FORMAT=""
OUTPUT_DIR=""
RECURSIVE=""
VALIDATE=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
MODEL_SOURCE=""
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
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --detailed)
            DETAILED="--detailed"
            ;;
        --layer-info)
            LAYER_INFO="--layer-info"
            ;;
        --output)
            ((i++))
            PROBE_FILE="${ARGS[$i]}"
            ;;
        --check-determinism)
            CHECK_DETERMINISM="--check-determinism"
            ;;
        --security-scan)
            SECURITY_SCAN="--security-scan"
            ;;
        --output)
            ((i++))
            REPORT_FILE="${ARGS[$i]}"
            ;;
        --to-format)
            ((i++))
            TO_FORMAT="${ARGS[$i]}"
            ;;
        --output-dir)
            ((i++))
            OUTPUT_DIR="${ARGS[$i]}"
            ;;
        --recursive)
            RECURSIVE="--recursive"
            ;;
        --validate)
            VALIDATE="--validate"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$MODEL_SOURCE" ]; then
                MODEL_SOURCE="${ARGS[$i]}"
            else
                echo "Error: Multiple model sources specified" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

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
    load-hf.sh|load-gguf.sh|load-safetensors.sh|convert-model.sh)
        if [ -z "$MODEL_SOURCE" ]; then
            echo "Error: No model source specified" >&2
            echo "Usage: $(basename "$0") <model_source> [--output <output_file>] [--canonfs-root <path>]" >&2
            exit 1
        fi
        # For local files, check existence
        if [ "$(basename "$0")" != "load-hf.sh" ] && [ ! -f "$MODEL_SOURCE" ]; then
            echo "Error: Model file not found: $MODEL_SOURCE" >&2
            exit 1
        fi
        ;;
    probe-model.sh|validate-model.sh)
        if [ -z "$MODEL_SOURCE" ]; then
            echo "Error: No model file specified" >&2
            echo "Usage: $(basename "$0") <model_file> [--options]" >&2
            exit 1
        fi
        if [ ! -f "$MODEL_SOURCE" ]; then
            echo "Error: Model file not found: $MODEL_SOURCE" >&2
            exit 1
        fi
        ;;
    batch-load.sh)
        if [ -z "$MODEL_SOURCE" ]; then
            echo "Error: No model directory specified" >&2
            echo "Usage: batch-load <model_directory> [--recursive] [--output-dir <dir>]" >&2
            exit 1
        fi
        if [ ! -d "$MODEL_SOURCE" ]; then
            echo "Error: Model directory not found: $MODEL_SOURCE" >&2
            exit 1
        fi
        ;;
esac

# Validate optional files
for file in "$POLICY_FILE"; do
    if [ -n "$file" ] && [ ! -f "$file" ]; then
        echo "Error: File not found: $file" >&2
        exit 1
    fi
done

# Create CanonFS root if it doesn't exist
mkdir -p "$CANONFS_ROOT"

# Create output directory if specified
if [ -n "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR"
fi

# Determine command based on script name
case "$(basename "$0")" in
    load-hf.sh)
        T81_CMD="t81 model load-hf"
        ;;
    load-gguf.sh)
        T81_CMD="t81 model load-gguf"
        ;;
    load-safetensors.sh)
        T81_CMD="t81 model load-safetensors"
        ;;
    probe-model.sh)
        T81_CMD="t81 model probe"
        ;;
    validate-model.sh)
        T81_CMD="t81 model validate"
        ;;
    convert-model.sh)
        T81_CMD="t81 model convert"
        ;;
    batch-load.sh)
        T81_CMD="t81 model batch-load"
        ;;
    *)
        echo "Error: Unknown model loader script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$MODEL_SOURCE" ]; then
    T81_CMD="$T81_CMD \"$MODEL_SOURCE\""
fi

if [ -n "$OUTPUT_FILE" ] && [ "$(basename "$0")" != "batch-load.sh" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$CANONFS_ROOT" ]; then
    T81_CMD="$T81_CMD --canonfs-root \"$CANONFS_ROOT\""
fi

if [ -n "$POLICY_FILE" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$DETAILED" ] && [ "$(basename "$0")" = "probe-model.sh" ]; then
    T81_CMD="$T81_CMD $DETAILED"
fi

if [ -n "$LAYER_INFO" ] && [ "$(basename "$0")" = "probe-model.sh" ]; then
    T81_CMD="$T81_CMD $LAYER_INFO"
fi

if [ -n "$PROBE_FILE" ] && [ "$(basename "$0")" = "probe-model.sh" ]; then
    T81_CMD="$T81_CMD --output \"$PROBE_FILE\""
fi

if [ -n "$CHECK_DETERMINISM" ] && [ "$(basename "$0")" = "validate-model.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_DETERMINISM"
fi

if [ -n "$SECURITY_SCAN" ] && [ "$(basename "$0")" = "validate-model.sh" ]; then
    T81_CMD="$T81_CMD $SECURITY_SCAN"
fi

if [ -n "$REPORT_FILE" ] && [ "$(basename "$0")" = "validate-model.sh" ]; then
    T81_CMD="$T81_CMD --output \"$REPORT_FILE\""
fi

if [ -n "$TO_FORMAT" ] && [ "$(basename "$0")" = "convert-model.sh" ]; then
    T81_CMD="$T81_CMD --to-format $TO_FORMAT"
fi

if [ -n "$OUTPUT_DIR" ] && [ "$(basename "$0")" = "batch-load.sh" ]; then
    T81_CMD="$T81_CMD --output-dir \"$OUTPUT_DIR\""
fi

if [ -n "$RECURSIVE" ] && [ "$(basename "$0")" = "batch-load.sh" ]; then
    T81_CMD="$T81_CMD $RECURSIVE"
fi

if [ -n "$VALIDATE" ] && [ "$(basename "$0")" = "load-gguf.sh" ]; then
    T81_CMD="$T81_CMD $VALIDATE"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 model loader
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Model loading completed successfully" >&2
    fi
else
    echo "Error: Model loading failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
