#!/bin/bash
set -e

REPORT_DIR="artifacts/ci_reports"
mkdir -p "$REPORT_DIR"
REPORT_FILE="$REPORT_DIR/api_diff_report.json"
DIFF_FILE="$REPORT_DIR/api.diff"

# Default to comparing against main, or HEAD^ if main not found
BASE_REF="origin/main"
if ! git rev-parse --verify "$BASE_REF" >/dev/null 2>&1; then
  BASE_REF="HEAD^"
fi

echo "Checking API diff against $BASE_REF..."

# Check if there are changes in include/
if git diff --quiet "$BASE_REF" -- include/; then
  echo "No API changes detected."
  echo '{ "status": "pass" }' > "$REPORT_FILE"
  exit 0
else
  echo "API changes detected!"
  git diff "$BASE_REF" -- include/ > "$DIFF_FILE"

  # Soft fail (exit 0 but report warning)
  echo '{ "status": "warning", "diff_file": "api.diff" }' > "$REPORT_FILE"
  exit 0
fi
