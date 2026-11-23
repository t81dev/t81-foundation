#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT_DIR}"

echo "=== [1/4] Architecture Coherence & Root Governance ==="
python3 scripts/ci/check_architecture_coherence.py

echo "=== [2/4] Operational State Contract Tests ==="
python3 -m unittest tests/ci/test_current_state_contract.py

echo "=== [3/4] Core CTest Boundary Suite ==="
ctest --test-dir build --output-on-failure -R "canonfs_policy_boundary_test|t81_vm_fault_test|canonfs_interchange_test|t81_determinism_codec_test"

echo "=== [4/4] Agent Receipt Sidecar + Native C++ MCP Server ==="
bash scripts/ci/run_agent_receipt_dogfood_smoke.sh

echo "ALL FAST VERIFICATION CHECKS PASSED."
