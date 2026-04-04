#!/bin/bash

# T81VM Runner Skill Wrapper
# Bridges OpenClaw skill to T81VM execution

set -euo pipefail

# Default values
POLICY_FILE=""
WEIGHTS_MODEL=""
TRACE_MODE=""
JSON_OUTPUT="--json"
OUTPUT_DIR=""
BREAKPOINTS=""
TRACE_LEVEL="basic"
INSPECT_STATE="registers"
INSPECT_ADDRESS=""
VERBOSE=""

# Parse arguments
PROGRAM_FILE=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --weights-model)
            ((i++))
            WEIGHTS_MODEL="${ARGS[$i]}"
            ;;
        --trace)
            TRACE_MODE="--trace"
            ;;
        --trace-level)
            ((i++))
            TRACE_LEVEL="${ARGS[$i]}"
            ;;
        --output)
            ((i++))
            JSON_OUTPUT="${ARGS[$i]}"
            ;;
        --output-dir)
            ((i++))
            OUTPUT_DIR="${ARGS[$i]}"
            ;;
        --breakpoints)
            ((i++))
            BREAKPOINTS="${ARGS[$i]}"
            ;;
        --state)
            ((i++))
            INSPECT_STATE="${ARGS[$i]}"
            ;;
        --address)
            ((i++))
            INSPECT_ADDRESS="${ARGS[$i]}"
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
            if [ -z "$PROGRAM_FILE" ]; then
                PROGRAM_FILE="${ARGS[$i]}"
            else
                echo "Error: Multiple program files specified" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate input
if [ -z "$PROGRAM_FILE" ]; then
    echo "Error: No program file specified" >&2
    echo "Usage: run <program_file> [--policy <policy_file>] [--weights-model <model_file>] [--trace] [--json]" >&2
    exit 1
fi

if [ ! -f "$PROGRAM_FILE" ]; then
    echo "Error: Program file not found: $PROGRAM_FILE" >&2
    exit 1
fi

# Validate trace level
case "$TRACE_LEVEL" in
    basic|full|policy)
        ;;
    *)
        echo "Error: Invalid trace level: $TRACE_LEVEL. Use 'basic', 'full', or 'policy'" >&2
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

# Validate weights model if specified
if [ -n "$WEIGHTS_MODEL" ] && [ ! -f "$WEIGHTS_MODEL" ]; then
    echo "Error: Weights model not found: $WEIGHTS_MODEL" >&2
    exit 1
fi

# Determine command based on script name
case "$(basename "$0")" in
    run.sh)
        T81_CMD="t81 vm run"
        ;;
    debug.sh)
        T81_CMD="t81 vm debug"
        ;;
    trace.sh)
        T81_CMD="t81 vm trace"
        ;;
    inspect.sh)
        T81_CMD="t81 vm inspect"
        ;;
    run-batch.sh)
        T81_CMD="t81 vm run-batch"
        ;;
    *)
        echo "Error: Unknown execution script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
T81_CMD="$T81_CMD \"$PROGRAM_FILE\""

if [ -n "$POLICY_FILE" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$WEIGHTS_MODEL" ]; then
    T81_CMD="$T81_CMD --weights-model \"$WEIGHTS_MODEL\""
fi

if [ -n "$TRACE_MODE" ]; then
    T81_CMD="$T81_CMD $TRACE_MODE"
fi

if [ -n "$TRACE_LEVEL" ] && [ "$(basename "$0")" = "trace.sh" ]; then
    T81_CMD="$T81_CMD --trace-level $TRACE_LEVEL"
fi

if [ -n "$BREAKPOINTS" ] && [ "$(basename "$0")" = "debug.sh" ]; then
    T81_CMD="$T81_CMD --breakpoints $BREAKPOINTS"
fi

if [ -n "$INSPECT_STATE" ] && [ "$(basename "$0")" = "inspect.sh" ]; then
    T81_CMD="$T81_CMD --state $INSPECT_STATE"
fi

if [ -n "$INSPECT_ADDRESS" ] && [ "$(basename "$0")" = "inspect.sh" ]; then
    T81_CMD="$T81_CMD --address $INSPECT_ADDRESS"
fi

if [ -n "$OUTPUT_DIR" ] && [ "$(basename "$0")" = "run-batch.sh" ]; then
    mkdir -p "$OUTPUT_DIR"
    T81_CMD="$T81_CMD --output-dir \"$OUTPUT_DIR\""
fi

if [ -n "$RECURSIVE" ] && [ "$(basename "$0")" = "run-batch.sh" ]; then
    T81_CMD="$T81_CMD $RECURSIVE"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81VM
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Execution completed successfully" >&2
    fi
else
    echo "Error: Execution failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
