#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${REPO_ROOT}"

echo "[audit] running workflow action pinning policy check"
python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0

echo "[audit] running workflow permissions policy check"
python3 scripts/ci/audit_workflow_permissions.py --max-missing 0

echo "[audit] running legacy core numeric include policy check"
python3 scripts/ci/check_legacy_core_numeric_includes.py

echo "[audit] running legacy core numeric type usage policy check"
python3 scripts/ci/check_legacy_core_numeric_type_usage.py

echo "[audit] running legacy v1 numeric include policy check"
python3 scripts/ci/check_legacy_v1_numeric_includes.py

echo "[audit] running core numeric wrapper thinness policy check"
python3 scripts/ci/check_core_numeric_wrapper_thinness.py

echo "[audit] running v1 canonical numeric alias usage policy check"
python3 scripts/ci/check_v1_canonical_numeric_alias_usage.py

echo "[audit] governance workflow audits passed"
