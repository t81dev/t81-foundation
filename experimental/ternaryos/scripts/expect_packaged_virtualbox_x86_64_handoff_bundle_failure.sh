#!/bin/zsh
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <bundle-dir>" >&2
  exit 2
fi

bundle_dir=$1
helper="$bundle_dir/validate_virtualbox_x86_64_handoff.sh"
expected_boot="$bundle_dir/bundle-negative-smoke/expected-boot-report.txt"
expected_status="$bundle_dir/bundle-negative-smoke/expected-startup-status.txt"
artifact_dir="$bundle_dir/bundle-negative-smoke/recovered-artifacts"
stderr_log="$bundle_dir/bundle-negative-smoke/validator-stderr.log"

for path in "$helper" "$expected_boot" "$expected_status" "$artifact_dir/boot-report.txt" "$artifact_dir/startup-status.txt"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required packaged handoff negative-smoke file: $path" >&2
    exit 1
  fi
done

if /bin/zsh "$helper" "$expected_boot" "$expected_status" "$artifact_dir" 2>"$stderr_log"; then
  echo "expected packaged handoff smoke-check to reject mismatched artifacts, but it succeeded" >&2
  exit 1
fi

if ! /usr/bin/grep -q "handoff validation mismatch" "$stderr_log"; then
  echo "packaged handoff smoke-check failed, but did not report an explicit mismatch" >&2
  /bin/cat "$stderr_log" >&2
  exit 1
fi

echo "Packaged VirtualBox x86_64 handoff bundle negative smoke-check succeeded."
