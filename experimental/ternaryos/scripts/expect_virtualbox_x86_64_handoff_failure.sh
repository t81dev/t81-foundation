#!/bin/zsh
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 <validator> <expected-boot-report> <expected-startup-status> <artifact-dir>" >&2
  exit 2
fi

validator=$1
expected_boot_report=$2
expected_startup_status=$3
artifact_dir=$4
stderr_log="$artifact_dir/validator-stderr.log"

if /bin/zsh "$validator" "$expected_boot_report" "$expected_startup_status" "$artifact_dir" 2>"$stderr_log"; then
  echo "expected handoff validator to reject mismatched artifacts, but it succeeded" >&2
  exit 1
fi

if ! /usr/bin/grep -q "handoff validation mismatch" "$stderr_log"; then
  echo "handoff validator failed, but did not report an explicit mismatch" >&2
  /bin/cat "$stderr_log" >&2
  exit 1
fi

echo "VirtualBox x86_64 handoff negative validation succeeded."
