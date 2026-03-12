#!/bin/zsh
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <expected-boot-report> <expected-startup-status> <artifact-dir>" >&2
  exit 2
fi

expected_boot_report=$1
expected_startup_status=$2
artifact_dir=$3

boot_report_path="$artifact_dir/boot-report.txt"
startup_status_path="$artifact_dir/startup-status.txt"

for path in "$expected_boot_report" "$expected_startup_status" "$boot_report_path" "$startup_status_path"; do
  if [[ ! -f "$path" ]]; then
    echo "missing required handoff validation file: $path" >&2
    exit 1
  fi
done

extract_value() {
  local file=$1
  local key=$2
  /usr/bin/awk -F= -v key="$key" '$1 == key {print substr($0, index($0, "=") + 1); exit}' "$file"
}

compare_key() {
  local expected_file=$1
  local actual_file=$2
  local key=$3
  local expected_value
  local actual_value
  expected_value=$(extract_value "$expected_file" "$key")
  actual_value=$(extract_value "$actual_file" "$key")
  if [[ -z "$expected_value" || -z "$actual_value" ]]; then
    echo "missing comparison field '$key' in expected or actual handoff artifact" >&2
    echo "expected file: $expected_file" >&2
    echo "actual file: $actual_file" >&2
    exit 1
  fi
  if [[ "$expected_value" != "$actual_value" ]]; then
    echo "handoff validation mismatch for '$key': expected '$expected_value' but found '$actual_value'" >&2
    echo "expected file: $expected_file" >&2
    /bin/cat "$expected_file" >&2
    echo "actual file: $actual_file" >&2
    /bin/cat "$actual_file" >&2
    exit 1
  fi
}

for key in \
  platform_id \
  kernel_boot_ready_slice \
  boot_progress_state \
  boot_progress_pending \
  boot_progress_blocked \
  boot_progress_source \
  boot_validation_lane
do
  compare_key "$expected_boot_report" "$boot_report_path" "$key"
done

for key in \
  os_name \
  platform_id \
  phase \
  shell_mode \
  kernel_boot_ready_slice \
  boot_progress_pending \
  boot_progress_blocked \
  boot_validation_lane \
  storage_binding \
  display_binding \
  network_binding
do
  compare_key "$expected_startup_status" "$startup_status_path" "$key"
done

echo "VirtualBox x86_64 handoff contract validation succeeded."
