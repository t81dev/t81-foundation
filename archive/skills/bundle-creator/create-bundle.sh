#!/bin/bash

# T81 Bundle Creator Skill Wrapper
# Bridges OpenClaw skill to T81 bundle creation

set -euo pipefail

# Default values
OUTPUT_FILE=""
POLICY_FILE=""
INCLUDE_PROVENANCE=""
INPUT_DATA=""
BUNDLE_OUTPUT=""
DETAILED=""
VERIFY_INTEGRITY=""
INFO_FILE=""
KEY_FILE=""
CERTIFICATE=""
CHECK_SIGNATURE=""
CHECK_PROVENANCE=""
CHECK_INTEGRITY=""
CONFLICT_RESOLUTION="keep"
COMPONENT=""
JSON_OUTPUT="--json"
VERBOSE=""

# Parse arguments
EXECUTION_RESULT=""
ARGS=("$@")
i=0
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --output)
            ((i++))
            OUTPUT_FILE="${ARGS[$i]}"
            ;;
        --policy)
            ((i++))
            POLICY_FILE="${ARGS[$i]}"
            ;;
        --include-provenance)
            INCLUDE_PROVENANCE="--include-provenance"
            ;;
        --input)
            ((i++))
            INPUT_DATA="${ARGS[$i]}"
            ;;
        --bundle-output)
            ((i++))
            BUNDLE_OUTPUT="${ARGS[$i]}"
            ;;
        --detailed)
            DETAILED="--detailed"
            ;;
        --verify-integrity)
            VERIFY_INTEGRITY="--verify-integrity"
            ;;
        --output)
            ((i++))
            INFO_FILE="${ARGS[$i]}"
            ;;
        --key)
            ((i++))
            KEY_FILE="${ARGS[$i]}"
            ;;
        --certificate)
            ((i++))
            CERTIFICATE="${ARGS[$i]}"
            ;;
        --check-signature)
            CHECK_SIGNATURE="--check-signature"
            ;;
        --check-provenance)
            CHECK_PROVENANCE="--check-provenance"
            ;;
        --conflict-resolution)
            ((i++))
            CONFLICT_RESOLUTION="${ARGS[$i]}"
            ;;
        --component)
            ((i++))
            COMPONENT="${ARGS[$i]}"
            ;;
        --verbose|-v)
            VERBOSE="--verbose"
            ;;
        -*)
            echo "Error: Unknown option ${ARGS[$i]}" >&2
            exit 1
            ;;
        *)
            if [ -z "$EXECUTION_RESULT" ]; then
                EXECUTION_RESULT="${ARGS[$i]}"
            elif [ "$(basename "$0")" = "merge-bundles.sh" ]; then
                # For merge-bundles, collect multiple bundle files
                EXECUTION_RESULT="$EXECUTION_RESULT ${ARGS[$i]}"
            else
                echo "Error: Unexpected argument: ${ARGS[$i]}" >&2
                exit 1
            fi
            ;;
    esac
    ((i++))
done

# Validate conflict resolution
case "$CONFLICT_RESOLUTION" in
    keep|merge|fail)
        ;;
    *)
        echo "Error: Invalid conflict resolution: $CONFLICT_RESOLUTION. Use 'keep', 'merge', or 'fail'" >&2
        exit 1
        ;;
esac

# Validate component
if [ -n "$COMPONENT" ]; then
    case "$COMPONENT" in
        result|provenance|metadata)
            ;;
        *)
            echo "Error: Invalid component: $COMPONENT. Use 'result', 'provenance', or 'metadata'" >&2
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
    create-bundle.sh)
        if [ -z "$EXECUTION_RESULT" ]; then
            echo "Error: No execution result specified" >&2
            echo "Usage: create-bundle <execution_result> [--output <bundle_file>] [--policy <policy_file>]" >&2
            exit 1
        fi
        if [ ! -f "$EXECUTION_RESULT" ]; then
            echo "Error: Execution result not found: $EXECUTION_RESULT" >&2
            exit 1
        fi
        ;;
    bundle-from-execution.sh)
        if [ -z "$EXECUTION_RESULT" ]; then
            echo "Error: No program file specified" >&2
            echo "Usage: bundle-from-execution <program_file> [--input <input_data>] [--policy <policy_file>]" >&2
            exit 1
        fi
        if [ ! -f "$EXECUTION_RESULT" ]; then
            echo "Error: Program file not found: $EXECUTION_RESULT" >&2
            exit 1
        fi
        ;;
    bundle-info.sh|sign-bundle.sh|verify-bundle.sh|extract-bundle.sh)
        if [ -z "$EXECUTION_RESULT" ]; then
            echo "Error: No bundle file specified" >&2
            echo "Usage: $(basename "$0") <bundle_file> [options]" >&2
            exit 1
        fi
        if [ ! -f "$EXECUTION_RESULT" ]; then
            echo "Error: Bundle file not found: $EXECUTION_RESULT" >&2
            exit 1
        fi
        ;;
    merge-bundles.sh)
        if [ -z "$EXECUTION_RESULT" ]; then
            echo "Error: No bundle files specified" >&2
            echo "Usage: merge-bundles <bundle1> <bundle2> [--output <merged_bundle>]" >&2
            exit 1
        fi
        # Check if all bundle files exist
        for bundle in $EXECUTION_RESULT; do
            if [ ! -f "$bundle" ]; then
                echo "Error: Bundle file not found: $bundle" >&2
                exit 1
            fi
        done
        ;;
esac

# Validate optional files
for file in "$POLICY_FILE" "$KEY_FILE" "$CERTIFICATE"; do
    if [ -n "$file" ] && [ ! -f "$file" ]; then
        echo "Error: File not found: $file" >&2
        exit 1
    fi
done

# Determine command based on script name
case "$(basename "$0")" in
    create-bundle.sh)
        T81_CMD="t81 bundle create"
        ;;
    bundle-from-execution.sh)
        T81_CMD="t81 bundle from-execution"
        ;;
    bundle-info.sh)
        T81_CMD="t81 bundle info"
        ;;
    sign-bundle.sh)
        T81_CMD="t81 bundle sign"
        ;;
    verify-bundle.sh)
        T81_CMD="t81 bundle verify"
        ;;
    merge-bundles.sh)
        T81_CMD="t81 bundle merge"
        ;;
    extract-bundle.sh)
        T81_CMD="t81 bundle extract"
        ;;
    *)
        echo "Error: Unknown bundle creator script: $(basename "$0")" >&2
        exit 1
        ;;
esac

# Build T81 command
if [ -n "$EXECUTION_RESULT" ]; then
    T81_CMD="$T81_CMD $EXECUTION_RESULT"
fi

if [ -n "$OUTPUT_FILE" ] && [ "$(basename "$0")" != "merge-bundles.sh" ]; then
    T81_CMD="$T81_CMD --output \"$OUTPUT_FILE\""
fi

if [ -n "$POLICY_FILE" ]; then
    T81_CMD="$T81_CMD --policy \"$POLICY_FILE\""
fi

if [ -n "$INCLUDE_PROVENANCE" ] && [ "$(basename "$0")" = "create-bundle.sh" ]; then
    T81_CMD="$T81_CMD $INCLUDE_PROVENANCE"
fi

if [ -n "$INPUT_DATA" ] && [ "$(basename "$0")" = "bundle-from-execution.sh" ]; then
    T81_CMD="$T81_CMD --input \"$INPUT_DATA\""
fi

if [ -n "$BUNDLE_OUTPUT" ] && [ "$(basename "$0")" = "bundle-from-execution.sh" ]; then
    T81_CMD="$T81_CMD --bundle-output \"$BUNDLE_OUTPUT\""
fi

if [ -n "$DETAILED" ] && [ "$(basename "$0")" = "bundle-info.sh" ]; then
    T81_CMD="$T81_CMD $DETAILED"
fi

if [ -n "$VERIFY_INTEGRITY" ] && [ "$(basename "$0")" = "bundle-info.sh" ]; then
    T81_CMD="$T81_CMD $VERIFY_INTEGRITY"
fi

if [ -n "$INFO_FILE" ] && [ "$(basename "$0")" = "bundle-info.sh" ]; then
    T81_CMD="$T81_CMD --output \"$INFO_FILE\""
fi

if [ -n "$KEY_FILE" ] && [ "$(basename "$0")" = "sign-bundle.sh" ]; then
    T81_CMD="$T81_CMD --key \"$KEY_FILE\""
fi

if [ -n "$CERTIFICATE" ] && [ "$(basename "$0")" = "sign-bundle.sh" ]; then
    T81_CMD="$T81_CMD --certificate \"$CERTIFICATE\""
fi

if [ -n "$CHECK_SIGNATURE" ] && [ "$(basename "$0")" = "verify-bundle.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_SIGNATURE"
fi

if [ -n "$CHECK_PROVENANCE" ] && [ "$(basename "$0")" = "verify-bundle.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_PROVENANCE"
fi

if [ -n "$CHECK_INTEGRITY" ] && [ "$(basename "$0")" = "verify-bundle.sh" ]; then
    T81_CMD="$T81_CMD $CHECK_INTEGRITY"
fi

if [ -n "$CONFLICT_RESOLUTION" ] && [ "$(basename "$0")" = "merge-bundles.sh" ]; then
    T81_CMD="$T81_CMD --conflict-resolution $CONFLICT_RESOLUTION"
fi

if [ -n "$COMPONENT" ] && [ "$(basename "$0")" = "extract-bundle.sh" ]; then
    T81_CMD="$T81_CMD --component $COMPONENT"
fi

# Always output JSON for machine readability
if [ "$JSON_OUTPUT" = "--json" ]; then
    T81_CMD="$T81_CMD $JSON_OUTPUT"
fi

if [ -n "$VERBOSE" ]; then
    echo "Executing: $T81_CMD" >&2
fi

# Execute T81 bundle creator
eval "$T81_CMD"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    if [ -n "$VERBOSE" ]; then
        echo "Bundle operation completed successfully" >&2
    fi
else
    echo "Error: Bundle operation failed with exit code $EXIT_CODE" >&2
    exit $EXIT_CODE
fi
