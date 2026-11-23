#!/bin/bash

# T81 Debugger Skill Wrapper
# Bridges OpenClaw skill to T81 debugging capabilities

set -euo pipefail

# Default values
POLICY_FILE=""
BREAKPOINTS=""
TRACE_LEVEL="basic"
CONDITION=""
POLICY_ONLY=""
ENABLED_ONLY=""
COUNT=1
STEP_TYPE=""
TO_ADDRESS=""
UNTIL_CONDITION=""
STATE_TYPE="registers"
ADDRESS=""
RANGE=""
START_TRACE=""
STOP_TRACE=""
TRACE_FILE=""
FILTER=""
DETAILED=""
SUGGEST_FIXES=""
REPORT_FILE=""
START_PROFILE=""
STOP_PROFILE=""
PROFILE_FILE=""
GRANULARITY="instruction"
JSON_OUTPUT="--json"
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
        --breakpoints)
            ((i++))
            BREAKPOINTS="${ARGS[$i]}"
            ;;
        --trace-level)
            ((i++))
            TRACE_LEVEL="${ARGS[$i]}"
            ;;
        --condition)
            ((i++))
            CONDITION="${ARGS[$i]}"
            ;;
        --policy-only)
            POLICY_ONLY="--policy-only"
            ;;
        --enabled-only)
            ENABLED_ONLY="--enabled-only"
            ;;
        --count)
            ((i++))
            COUNT="${ARGS[$i]}"
            ;;
        --into)
            STEP_TYPE="--into"
            ;;
        --over)
            STEP_TYPE="--over"
            ;;
        --out)
            STEP_TYPE="--out"
            ;;
        --to)
            ((i++))
            TO_ADDRESS="${ARGS[$i]}"
            ;;
        --until)
            ((i++))
            UNTIL_CONDITION="${ARGS[$i]}"
            ;;
        --state)
            ((i++))
            STATE_TYPE="${ARGS[$i]}"
            ;;
        --address)
            ((i++))
            ADDRESS="${ARGS[$i]}"
            ;;
        --range)
            ((i++))
            RANGE="${ARGS[$i]}"
            ;;
        --start)
            START_TRACE="--start"
            START_PROFILE="--start"
            ;;
        --stop)
            STOP_TRACE="--stop"
            STOP_PROFILE="--stop"
            ;;
        --save)
            ((i++))
            TRACE_FILE="${ARGS[$i]}"
            ;;
        --output)
            ((i++))
            REPORT_FILE="${ARGS[$i]}"
            ;;
        --filter)
            ((i++))
            FILTER="${ARGS[$i]}"
            ;;
        --detailed)
            DETAILED="--detailed"
            ;;
        --suggest-fixes)
            SUGGEST_FIXES="--suggest-fixes"
            ;;
        --granularity)
            ((i++))
            GRANULARITY="${ARGS[$i]}"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        --all)
            ALL="--all"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$PROGRAM_FILE" ]; then
                PROGRAM_FILE="${ARGS[$i]}"
            else
                echo "Error: Unexpected argument: ${ARGS[$i]}" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate trace level
case "$TRACE_LEVEL" in
    basic|full|policy)
        ;;
    *)
        echo "Error: Invalid trace level: $TRACE_LEVEL. Use 'basic', 'full', or 'policy'" >&2
        exit 1
        ;;
esac

# Validate step type
if [ -n "$STEP_TYPE" ]; then
    case "$STEP_TYPE" in
        --into|--over|--out)
            ;;
        *)
            echo "Error: Invalid step type: $STEP_TYPE. Use '--into', '--over', or '--out'" >&2
            exit 1
            ;;
    esac
fi

# Validate granularity
case "$GRANULARITY" in
    instruction|function|policy)
        ;;
    *)
        echo "Error: Invalid granularity: $GRANULARITY. Use 'instruction', 'function', or 'policy'" >&2
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
    debug.sh)
        if [ -z "$PROGRAM_FILE" ]; then
            echo "Error: No program file specified" >&2
            echo "Usage: debug <program_file> [--policy <policy_file>] [--breakpoints <addr1,addr2>]" >&2
            exit 1
        fi
        if [ ! -f "$PROGRAM_FILE" ]; then
            echo "Error: Program file not found: $PROGRAM_FILE" >&2
            exit 1
        fi
        ;;
    breakpoint.sh)
        if [ -z "$ADDRESS" ] && [ -z "$ALL" ]; then
            echo "Error: No address specified for breakpoint" >&2
            echo "Usage: breakpoint set <address> [--condition <condition>]" >&2
            exit 1
        fi
        ;;
    step.sh|continue.sh|inspect.sh|trace.sh|profile.sh|analyze-violations.sh)
        # These commands work on active debug session
        ;;
esac

# Validate policy file if specified
if [ -n "$POLICY_FILE" ] && [ ! -f "$POLICY_FILE" ]; then
    echo "Error: Policy file not found: $POLICY_FILE" >&2
    exit 1
fi

# Determine command based on script name
case "$(basename "$0")" in
    debug.sh)
        T81_CMD="t81 debug start"
        ;;
    breakpoint.sh)
        T81_CMD="t81 debug breakpoint"
        ;;
    step.sh)
        T81_CMD="t81 debug step"
        ;;
    continue.sh)
        T81_CMD="t81 debug continue"
        ;;
    inspect.sh)
        T81_CMD="t81 debug inspect"
        ;;
    trace.sh)
        T81_CMD="t81 debug trace"
        ;;
    analyze-violations.sh)
        T81_CMD="t81 debug analyze-violations"
        ;;
    profile.sh)
        T81_CMD="t81 debug profile"
        ;;
    *)
        echo "Error: Unknown debug script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$PROGRAM_FILE" ] && [ "$(basename "$0")" = "debug.sh" ]; then
    T81_CMD="$T81_CMD \"$PROGRAM_FILE\""
fi

if [ -n "$POLICY_FILE" ] && [ "$(basename "$0")" = "debug.sh" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$BREAKPOINTS" ] && [ "$(basename "$0")" = "debug.sh" ]; then
    T81_CMD="$T81_CMD --breakpoints $BREAKPOINTS"
fi

if [ -n "$TRACE_LEVEL" ] && [ "$(basename "$0")" = "debug.sh" ]; then
    T81_CMD="$T81_CMD --trace-level $TRACE_LEVEL"
fi

# Handle breakpoint subcommands
if [ "$(basename "$0")" = "breakpoint.sh" ]; then
    if [ "${ARGS[0]}" = "set" ]; then
        T81_CMD="$T81_CMD set $ADDRESS"
        if [ -n "$CONDITION" ]; then
            T81_CMD="$T81_CMD --condition \"$CONDITION\""
        fi
        if [ -n "$POLICY_ONLY" ]; then
            T81_CMD="$T81_CMD $POLICY_ONLY"
        fi
    elif [ "${ARGS[0]}" = "remove" ]; then
        if [ -n "$ALL" ]; then
            T81_CMD="$T81_CMD remove --all"
        else
            T81_CMD="$T81_CMD remove $ADDRESS"
        fi
    elif [ "${ARGS[0]}" = "list" ]; then
        T81_CMD="$T81_CMD list"
        if [ -n "$ENABLED_ONLY" ]; then
            T81_CMD="$T81_CMD $ENABLED_ONLY"
        fi
    fi
fi

if [ -n "$COUNT" ] && [ "$(basename "$0")" = "step.sh" ]; then
    T81_CMD="$T81_CMD --count $COUNT"
fi

if [ -n "$STEP_TYPE" ] && [ "$(basename "$0")" = "step.sh" ]; then
    T81_CMD="$T81_CMD $STEP_TYPE"
fi

if [ -n "$TO_ADDRESS" ] && [ "$(basename "$0")" = "continue.sh" ]; then
    T81_CMD="$T81_CMD --to $TO_ADDRESS"
fi

if [ -n "$UNTIL_CONDITION" ] && [ "$(basename "$0")" = "continue.sh" ]; then
    T81_CMD="$T81_CMD --until \"$UNTIL_CONDITION\""
fi

if [ -n "$STATE_TYPE" ] && [ "$(basename "$0")" = "inspect.sh" ]; then
    T81_CMD="$T81_CMD --state $STATE_TYPE"
fi

if [ -n "$ADDRESS" ] && [ "$(basename "$0")" = "inspect.sh" ]; then
    T81_CMD="$T81_CMD --address $ADDRESS"
fi

if [ -n "$RANGE" ] && [ "$(basename "$0")" = "inspect.sh" ]; then
    T81_CMD="$T81_CMD --range $RANGE"
fi

if [ -n "$START_TRACE" ] && [ "$(basename "$0")" = "trace.sh" ]; then
    T81_CMD="$T81_CMD $START_TRACE"
fi

if [ -n "$STOP_TRACE" ] && [ "$(basename "$0")" = "trace.sh" ]; then
    T81_CMD="$T81_CMD $STOP_TRACE"
fi

if [ -n "$TRACE_FILE" ] && [ "$(basename "$0")" = "trace.sh" ]; then
    T81_CMD="$T81_CMD --save \"$TRACE_FILE\""
fi

if [ -n "$FILTER" ] && [ "$(basename "$0")" = "trace.sh" ]; then
    T81_CMD="$T81_CMD --filter $FILTER"
fi

if [ -n "$DETAILED" ] && [ "$(basename "$0")" = "analyze-violations.sh" ]; then
    T81_CMD="$T81_CMD $DETAILED"
fi

if [ -n "$SUGGEST_FIXES" ] && [ "$(basename "$0")" = "analyze-violations.sh" ]; then
    T81_CMD="$T81_CMD $SUGGEST_FIXES"
fi

if [ -n "$REPORT_FILE" ] && [ "$(basename "$0")" = "analyze-violations.sh" ]; then
    T81_CMD="$T81_CMD --output \"$REPORT_FILE\""
fi

if [ -n "$START_PROFILE" ] && [ "$(basename "$0")" = "profile.sh" ]; then
    T81_CMD="$T81_CMD $START_PROFILE"
fi

if [ -n "$STOP_PROFILE" ] && [ "$(basename "$0")" = "profile.sh" ]; then
    T81_CMD="$T81_CMD $STOP_PROFILE"
fi

if [ -n "$PROFILE_FILE" ] && [ "$(basename "$0")" = "profile.sh" ]; then
    T81_CMD="$T81_CMD --output \"$PROFILE_FILE\""
fi

if [ -n "$GRANULARITY" ] && [ "$(basename "$0")" = "profile.sh" ]; then
    T81_CMD="$T81_CMD --granularity $GRANULARITY"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 debug command
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Debug command completed successfully" >&2
    fi
else
    echo "Error: Debug command failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
