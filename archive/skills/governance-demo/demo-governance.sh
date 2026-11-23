#!/bin/bash

# T81 Governance Demo Skill Wrapper
# Bridges OpenClaw skill to T81 governance demonstrations

set -euo pipefail

# Default values
POLICY_FILE=""
MODEL_FILE=""
INTERACTIVE=""
SCENARIO="basic"
SHOW_VIOLATIONS=""
ITERATIONS=5
COMPARE_RESULTS=""
LEVEL="beginner"
EXAMPLES=""
POLICY_TEMPLATE=""
TEST_SCENARIOS=""
INDUSTRY="finance"
REGULATIONS=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --model)
            ((i++))
            MODEL_FILE="${ARGS[$i]}"
            ;;
        --interactive)
            INTERACTIVE="--interactive"
            ;;
        --scenario)
            ((i++))
            SCENARIO="${ARGS[$i]}"
            ;;
        --show-violations)
            SHOW_VIOLATIONS="--show-violations"
            ;;
        --iterations)
            ((i++))
            ITERATIONS="${ARGS[$i]}"
            ;;
        --compare-results)
            COMPARE_RESULTS="--compare-results"
            ;;
        --level)
            ((i++))
            LEVEL="${ARGS[$i]}"
            ;;
        --examples)
            EXAMPLES="--examples"
            ;;
        --policy-template)
            ((i++))
            POLICY_TEMPLATE="${ARGS[$i]}"
            ;;
        --test-scenarios)
            TEST_SCENARIOS="--test-scenarios"
            ;;
        --industry)
            ((i++))
            INDUSTRY="${ARGS[$i]}"
            ;;
        --regulations)
            REGULATIONS="--regulations"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
    esac
    ((i++))
done

# Validate scenario
case "$SCENARIO" in
    basic|advanced|financial|healthcare)
        ;;
    *)
        echo "Error: Invalid scenario: $SCENARIO. Use 'basic', 'advanced', 'financial', or 'healthcare'" >&2
        exit 1
        ;;
esac

# Validate level
case "$LEVEL" in
    beginner|intermediate|advanced)
        ;;
    *)
        echo "Error: Invalid level: $LEVEL. Use 'beginner', 'intermediate', or 'advanced'" >&2
        exit 1
        ;;
esac

# Validate industry
case "$INDUSTRY" in
    finance|healthcare|government)
        ;;
    *)
        echo "Error: Invalid industry: $INDUSTRY. Use 'finance', 'healthcare', or 'government'" >&2
        exit 1
        ;;
esac

# Check if T81 CLI is available
if ! command -v t81 &> /dev/null; then
    echo "Error: T81 CLI not found. Please install T81 Foundation." >&2
    exit 1
fi

# Validate optional files
for file in "$POLICY_FILE" "$MODEL_FILE" "$POLICY_TEMPLATE"; do
    if [ -n "$file" ] && [ ! -f "$file" ]; then
        echo "Error: File not found: $file" >&2
        exit 1
    fi
done

# Determine command based on script name
case "$(basename "$0")" in
    demo-governance.sh)
        T81_CMD="t81 demo governance"
        ;;
    demo-policy.sh)
        T81_CMD="t81 demo policy"
        ;;
    demo-determinism.sh)
        T81_CMD="t81 demo determinism"
        ;;
    tutorial-policy.sh)
        T81_CMD="t81 demo tutorial-policy"
        ;;
    interactive-policy.sh)
        T81_CMD="t81 demo interactive-policy"
        ;;
    compliance-demo.sh)
        T81_CMD="t81 demo compliance"
        ;;
    *)
        echo "Error: Unknown demo script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$POLICY_FILE" ] && [ "$(basename "$0")" = "demo-governance.sh" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$MODEL_FILE" ] && [ "$(basename "$0")" = "demo-governance.sh" ]; then
    T81_CMD="$T81_CMD --model \"$MODEL_FILE\""
fi

if [ -n "$INTERACTIVE" ] && [ "$(basename "$0")" = "demo-governance.sh" ]; then
    T81_CMD="$T81_CMD $INTERACTIVE"
fi

if [ -n "$SCENARIO" ] && [ "$(basename "$0")" = "demo-policy.sh" ]; then
    T81_CMD="$T81_CMD --scenario $SCENARIO"
fi

if [ -n "$SHOW_VIOLATIONS" ] && [ "$(basename "$0")" = "demo-policy.sh" ]; then
    T81_CMD="$T81_CMD $SHOW_VIOLATIONS"
fi

if [ -n "$ITERATIONS" ] && [ "$(basename "$0")" = "demo-determinism.sh" ]; then
    T81_CMD="$T81_CMD --iterations $ITERATIONS"
fi

if [ -n "$COMPARE_RESULTS" ] && [ "$(basename "$0")" = "demo-determinism.sh" ]; then
    T81_CMD="$T81_CMD $COMPARE_RESULTS"
fi

if [ -n "$LEVEL" ] && [ "$(basename "$0")" = "tutorial-policy.sh" ]; then
    T81_CMD="$T81_CMD --level $LEVEL"
fi

if [ -n "$EXAMPLES" ] && [ "$(basename "$0")" = "tutorial-policy.sh" ]; then
    T81_CMD="$T81_CMD $EXAMPLES"
fi

if [ -n "$POLICY_TEMPLATE" ] && [ "$(basename "$0")" = "interactive-policy.sh" ]; then
    T81_CMD="$T81_CMD --policy-template \"$POLICY_TEMPLATE\""
fi

if [ -n "$TEST_SCENARIOS" ] && [ "$(basename "$0")" = "interactive-policy.sh" ]; then
    T81_CMD="$T81_CMD $TEST_SCENARIOS"
fi

if [ -n "$INDUSTRY" ] && [ "$(basename "$0")" = "compliance-demo.sh" ]; then
    T81_CMD="$T81_CMD --industry $INDUSTRY"
fi

if [ -n "$REGULATIONS" ] && [ "$(basename "$0")" = "compliance-demo.sh" ]; then
    T81_CMD="$T81_CMD $REGULATIONS"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 governance demo
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Governance demo completed successfully" >&2
    fi
else
    echo "Error: Governance demo failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
