#!/bin/zsh
set -euo pipefail

# validate_qemu_armv8_slice6_reports.sh
#
# Validates the slice6-boot-report.txt and execution marker written by
# qemu_armv8_efi_stub.c during QEMU boot.  Called by
# run_qemu_armv8_slice6_probe.sh after the QEMU machine exits and the FAT32
# partition is mounted.

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <output-dir>" >&2
  exit 2
fi

output_dir=$1
report_copy="$output_dir/slice6-boot-report.txt"
summary_file="$output_dir/qemu-armv8-slice6-summary.txt"

if [[ ! -f "$report_copy" || ! -f "$summary_file" ]]; then
  echo "QEMU AArch64 slice6 report validation is missing required output files" >&2
  exit 1
fi

for expected in \
  '^AXION_QEMU_SLICE6_BOOT_REPORT$' \
  '^platform_id=qemu-armv8:AArch64/EDK2/slice6-boot-probe$' \
  '^slice4_svc_trap_wiring=complete$' \
  '^slice5_user_isolation=complete$' \
  '^runtime_status_ok=true$' \
  '^kernel_boot_ready_slice=slice6-efi-boot$' \
  '^hal_main_result=0$' \
  '^boot_progress_state=ready$' \
  '^boot_progress_pending=false$' \
  '^boot_progress_blocked=false$' \
  '^boot_validation_lane=qemu-armv8-slice6-probe$'
do
  if ! /usr/bin/grep -q "$expected" "$report_copy"; then
    echo "QEMU AArch64 slice6 report validation: expected field missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$report_copy" >&2
    exit 1
  fi
done

echo "QEMU AArch64 slice6 report validation succeeded."
