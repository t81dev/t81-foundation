#!/usr/bin/env bash
set -euo pipefail

# SCG-06 Automated Reproduction Runner
# Bootstraps isolated worktrees on-the-fly and executes the full F1-F7 battery.
# Designed to be executed from any working directory.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)"
WORKTREE_BASE="${REPO_ROOT}/.scg06_worktrees"
PRE_COMMIT="64941caff9372f2e76dd3a0fb79aa6711a48b171"
POST_COMMIT="2c3501e2807bb893076c5037231c3126b7f2bc72"

echo "=== SCG-06: Bootstrapping Isolated Worktrees ==="
mkdir -p "${WORKTREE_BASE}"

clean_worktrees() {
    echo "=== Cleaning up temporary worktrees ==="
    git -C "${REPO_ROOT}" worktree remove --force "${WORKTREE_BASE}/pre_scg03" 2>/dev/null || true
    git -C "${REPO_ROOT}" worktree remove --force "${WORKTREE_BASE}/post_scg03" 2>/dev/null || true
    git -C "${REPO_ROOT}" worktree prune 2>/dev/null || true
    rm -rf "${WORKTREE_BASE}" /tmp/scg06_bin /tmp/scg06_worktrees
}
trap clean_worktrees EXIT

git -C "${REPO_ROOT}" fetch origin "${PRE_COMMIT}" "${POST_COMMIT}" || true

rm -rf "${WORKTREE_BASE}/pre_scg03" "${WORKTREE_BASE}/post_scg03"
git -C "${REPO_ROOT}" worktree add --detach "${WORKTREE_BASE}/pre_scg03" "${PRE_COMMIT}"
git -C "${REPO_ROOT}" worktree add --detach "${WORKTREE_BASE}/post_scg03" "${POST_COMMIT}"

# Symlink to /tmp/scg06_worktrees for reproducible path consistency
mkdir -p /tmp/scg06_worktrees
ln -sfn "${WORKTREE_BASE}/pre_scg03" /tmp/scg06_worktrees/pre_scg03
ln -sfn "${WORKTREE_BASE}/post_scg03" /tmp/scg06_worktrees/post_scg03

echo "=== Running Python Oracle & Reproduction Suite ==="
export PYTHONPATH="${SCRIPT_DIR}:${REPO_ROOT}"
python3 "${SCRIPT_DIR}/reproduce_f1_f7.py"

echo "=== Verifying Evidence SHA-256 Checksums ==="
cd "${SCRIPT_DIR}"
sha256sum -c SHA256SUMS.txt

echo "=== SCG-06 Reproduction Successfully Executed and Verified ==="
