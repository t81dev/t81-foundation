#!/bin/bash
set -e

# Path to the t81 CLI (assuming run from repo root)
T81_BIN=./build/t81

if [ ! -f "$T81_BIN" ]; then
    echo "Error: t81 binary not found at $T81_BIN"
    exit 1
fi

# Ensure output dir exists
mkdir -p build

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "--- Verifying T81 System Integration AGI Examples ---"

EXAMPLES=(
    "ethical_accumulator"
    "gated_inference"
    "tier4_refining"
    "multi_agent_consensus"
    "world_model_sim"
    "categorical_governance"
    "full_stack_agent"
    "distributed_sharding"
    "evolutionary_tree"
    "supply_chain"
    "recursive_nas"
    "zk_state_transition"
)

# Compile and Run each example
for example in "${EXAMPLES[@]}"; do
    echo -n "Verifying ${example}.t81 ... "

    # 1. Compile
    if $T81_BIN compile examples/system-integration-agi/${example}.t81 -o build/${example}.tisc > /dev/null 2>&1; then
        # 2. Run
        if $T81_BIN run build/${example}.tisc --policy examples/system-integration-agi/agi_safety.apl > /dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
        else
            echo -e "${RED}FAIL (Run)${NC}"
            echo "Running again for output:"
            $T81_BIN run build/${example}.tisc --policy examples/system-integration-agi/agi_safety.apl
            exit 1
        fi
    else
        echo -e "${RED}FAIL (Compile)${NC}"
        echo "Compiling again for output:"
        $T81_BIN compile examples/system-integration-agi/${example}.t81 -o build/${example}.tisc
        exit 1
    fi
done

echo "--- All Examples Verified Successfully ---"
