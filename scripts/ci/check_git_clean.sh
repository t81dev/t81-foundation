#!/bin/bash
set -e

REPORT_DIR="artifacts/ci_reports"
mkdir -p "$REPORT_DIR"
REPORT_FILE="$REPORT_DIR/git_clean_report.json"

if [ -n "$(git status --porcelain)" ]; then
  echo "Error: Git working directory is dirty."
  git status
  git diff

  # Create JSON
  echo '{ "status": "fail", "message": "Working directory dirty" }' > "$REPORT_FILE"
  exit 1
else
  echo "Git working directory is clean."
  echo '{ "status": "pass" }' > "$REPORT_FILE"
  exit 0
fi
