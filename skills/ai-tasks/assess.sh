#!/bin/bash

# T81 AI Tasks Skill Wrapper
# Bridges OpenClaw skill to T81 AI task chains

set -euo pipefail

# Default values
MODEL_FILE=""
POLICY_FILE=""
OUTPUT_FILE=""
ROUTE_CONFIG=""
CLASS_CONFIG=""
CHAIN_CONFIG=""
BUNDLE_OUTPUT=""
TASK_ID=""
DETAILED=""
PROVENANCE=""
LIMIT=10
FILTER=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
INPUT_DATA=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --model)
            ((i++))
            MODEL_FILE="${ARGS[$i]}"
            ;;
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --output)
            ((i++))
            OUTPUT_FILE="${ARGS[$i]}"
            ;;
        --routes)
            ((i++))
            ROUTE_CONFIG="${ARGS[$i]}"
            ;;
        --classes)
            ((i++))
            CLASS_CONFIG="${ARGS[$i]}"
            ;;
        --chain)
            ((i++))
            CHAIN_CONFIG="${ARGS[$i]}"
            ;;
        --bundle-output)
            ((i++))
            BUNDLE_OUTPUT="${ARGS[$i]}"
            ;;
        --input)
            ((i++))
            INPUT_DATA="${ARGS[$i]}"
            ;;
        --detailed)
            DETAILED="--detailed"
            ;;
        --provenance)
            PROVENANCE="--provenance"
            ;;
        --limit)
            ((i++))
            LIMIT="${ARGS[$i]}"
            ;;
        --filter)
            ((i++))
            FILTER="${ARGS[$i]}"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$INPUT_DATA" ] && [ "$(basename "$0")" != "status.sh" ] && [ "$(basename "$0")" != "history.sh" ]; then
                INPUT_DATA="${ARGS[$i]}"
            elif [ -z "$TASK_ID" ] && [ "$(basename "$0")" = "status.sh" ]; then
                TASK_ID="${ARGS[$i]}"
            else
                echo "Error: Unexpected argument: ${ARGS[$i]}" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate filter
if [ -n "$FILTER" ]; then
    case "$FILTER" in
        assess|route|classify)
            ;;
        *)
            echo "Error: Invalid filter: $FILTER. Use 'assess', 'route', or 'classify'" >&2
            exit 1
            ;;
    esac
fi

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Validate required files based on command
case "$(basename "$0")" in
    assess.sh)
        if [ -z "$INPUT_DATA" ]; then
            echo "Error: No input data specified" >&2
            echo "Usage: assess <input_data> [--model <model_file>] [--policy <policy_file>] [--output <output_file>]" >&2
            exit 1
        fi
        if [ ! -f "$INPUT_DATA" ]; then
            echo "Error: Input data not found: $INPUT_DATA" >&2
            exit 1
        fi
        ;;
    route.sh)
        if [ -z "$INPUT_DATA" ]; then
            echo "Error: No input data specified" >&2
            echo "Usage: route <input_data> [--routes <route_config>] [--policy <policy_file>] [--output <output_file>]" >&2
            exit 1
        fi
        if [ ! -f "$INPUT_DATA" ]; then
            echo "Error: Input data not found: $INPUT_DATA" >&2
            exit 1
        fi
        ;;
    classify.sh)
        if [ -z "$INPUT_DATA" ]; then
            echo "Error: No input data specified" >&2
            echo "Usage: classify <input_data> [--classes <class_config>] [--policy <policy_file>] [--output <output_file>]" >&2
            exit 1
        fi
        if [ ! -f "$INPUT_DATA" ]; then
            echo "Error: Input data not found: $INPUT_DATA" >&2
            exit 1
        fi
        ;;
    chain.sh)
        if [ -z "$CHAIN_CONFIG" ]; then
            echo "Error: No chain configuration specified" >&2
            echo "Usage: chain --chain <chain_config> [--input <input_data>] [--policy <policy_file>] [--bundle-output <bundle_file>]" >&2
            exit 1
        fi
        if [ ! -f "$CHAIN_CONFIG" ]; then
            echo "Error: Chain configuration not found: $CHAIN_CONFIG" >&2
            exit 1
        fi
        ;;
    status.sh)
        if [ -z "$TASK_ID" ]; then
            echo "Error: No task ID specified" >&2
            echo "Usage: status <task_id> [--detailed] [--provenance]" >&2
            exit 1
        fi
        ;;
    history.sh)
        # No required arguments for history
        ;;
esac

# Validate optional files
for file in "$MODEL_FILE" "$POLICY_FILE" "$OUTPUT_FILE" "$ROUTE_CONFIG" "$CLASS_CONFIG" "$BUNDLE_OUTPUT"; do
    if [ -n "$file" ] && [ ! -f "$file" ]; then
        echo "Error: File not found: $file" >&2
        exit 1
    fi
done

# Determine command based on script name
case "$(basename "$0")" in
    assess.sh)
        T81_CMD="t81 ai task assess"
        ;;
    route.sh)
        T81_CMD="t81 ai task route"
        ;;
    classify.sh)
        T81_CMD="t81 ai task classify"
        ;;
    chain.sh)
        T81_CMD="t81 ai task chain"
        ;;
    status.sh)
        T81_CMD="t81 ai task status"
        ;;
    history.sh)
        T81_CMD="t81 ai task history"
        ;;
    *)
        echo "Error: Unknown AI task script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$INPUT_DATA" ] && [ "$(basename "$0")" != "status.sh" ] && [ "$(basename "$0")" != "history.sh" ]; then
    T81_CMD="$T81_CMD \"$INPUT_DATA\""
fi

if [ -n "$MODEL_FILE" ]; then
    T81_CMD="$T81_CMD --model \"$MODEL_FILE\""
fi

if [ -n "$POLICY_FILE" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$OUTPUT_FILE" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$ROUTE_CONFIG" ]; then
    T81_CMD="$T81_CMD --routes \"$ROUTE_CONFIG\""
fi

if [ -n "$CLASS_CONFIG" ]; then
    T81_CMD="$T81_CMD --classes \"$CLASS_CONFIG\""
fi

if [ -n "$CHAIN_CONFIG" ]; then
    T81_CMD="$T81_CMD --chain \"$CHAIN_CONFIG\""
fi

if [ -n "$BUNDLE_OUTPUT" ]; then
    T81_CMD="$T81_CMD --bundle-output \"$BUNDLE_OUTPUT\""
fi

if [ -n "$TASK_ID" ] && [ "$(basename "$0")" = "status.sh" ]; then
    T81_CMD="$T81_CMD $TASK_ID"
fi

if [ -n "$DETAILED" ] && [ "$(basename "$0")" = "status.sh" ]; then
    T81_CMD="$T81_CMD $DETAILED"
fi

if [ -n "$PROVENANCE" ] && [ "$(basename "$0")" = "status.sh" ]; then
    T81_CMD="$T81_CMD $PROVENANCE"
fi

if [ -n "$LIMIT" ] && [ "$(basename "$0")" = "history.sh" ]; then
    T81_CMD="$T81_CMD --limit $LIMIT"
fi

if [ -n "$FILTER" ] && [ "$(basename "$0")" = "history.sh" ]; then
    T81_CMD="$T81_CMD --filter $FILTER"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 AI task
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "AI task completed successfully" >&2
    fi
else
    echo "Error: AI task failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
