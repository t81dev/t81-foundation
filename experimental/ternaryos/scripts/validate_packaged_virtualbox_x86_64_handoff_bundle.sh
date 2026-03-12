#!/bin/zsh
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <bundle-dir>" >&2
  exit 2
fi

bundle_dir=$1
helper="$bundle_dir/validate_virtualbox_x86_64_handoff.sh"
expected_boot="$bundle_dir/bundle-smoke/expected-boot-report.txt"
expected_status="$bundle_dir/bundle-smoke/expected-startup-status.txt"
artifact_dir="$bundle_dir/bundle-smoke/recovered-artifacts"

for path in "$helper" "$expected_boot" "$expected_status" "$artifact_dir/boot-report.txt" "$artifact_dir/startup-status.txt"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required packaged handoff smoke-check file: $path" >&2
    exit 1
  fi
done

/bin/zsh "$helper" "$expected_boot" "$expected_status" "$artifact_dir"
echo "Packaged VirtualBox x86_64 handoff bundle smoke-check succeeded."
