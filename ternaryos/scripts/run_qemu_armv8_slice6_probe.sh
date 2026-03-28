#!/bin/zsh
set -euo pipefail

# run_qemu_armv8_slice6_probe.sh
#
# Boots the Slice 6 QEMU AArch64 EDK2 guest image, waits for the machine to
# complete, mounts the FAT32 partition, and validates the artefacts written
# by qemu_armv8_efi_stub.c.
#
# Usage: run_qemu_armv8_slice6_probe.sh <arm-image> <output-dir> [boot-wait-seconds]
#
# <arm-image>          Path to the QEMU slice6 raw disk image (.img).
# <output-dir>         Directory for log files and extracted artefacts.
# [boot-wait-seconds]  How long to let QEMU run before killing it (default: 8).
#
# Environment overrides:
#   T81_QEMU_BIN    Path to qemu-system-aarch64
#   T81_EDK2_CODE   Path to edk2-aarch64-code.fd
#   T81_EDK2_VARS   Path to edk2-arm-vars.fd template
#   T81_QEMU_ACCEL  Accelerator to use: hvf or tcg

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <arm-image> <output-dir> [boot-wait-seconds]" >&2
  exit 2
fi

arm_image=$1
output_dir=$2
boot_wait=${3:-8}

qemu_bin=${T81_QEMU_BIN:-/opt/homebrew/bin/qemu-system-aarch64}
edk2_code=${T81_EDK2_CODE:-/opt/homebrew/share/qemu/edk2-aarch64-code.fd}
edk2_vars_template=${T81_EDK2_VARS:-/opt/homebrew/share/qemu/edk2-arm-vars.fd}

for path in "$qemu_bin" "$edk2_code" "$edk2_vars_template" "$arm_image"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required path: $path" >&2
    exit 1
  fi
done

/bin/mkdir -p "$output_dir"
probe_image="$output_dir/qemu-armv8-slice6-probe.img"
vars_copy="$output_dir/edk2-aarch64-vars.fd"
serial_log="$output_dir/qemu-armv8-slice6-serial.log"
pid_file="$output_dir/qemu-armv8-slice6.pid"
summary_file="$output_dir/qemu-armv8-slice6-summary.txt"
report_copy="$output_dir/slice6-boot-report.txt"
marker_copy="$output_dir/efi-slice6-ran.txt"

/bin/cp "$arm_image" "$probe_image"
/bin/cp "$edk2_vars_template" "$vars_copy"
/bin/rm -f "$serial_log" "$pid_file" "$summary_file" "$report_copy" "$marker_copy"

qemu_pid=""
disk_dev=""
cleanup() {
  if [[ -n "$qemu_pid" ]]; then
    kill "$qemu_pid" >/dev/null 2>&1 || true
    wait "$qemu_pid" >/dev/null 2>&1 || true
  fi
  if [[ -n "$disk_dev" ]]; then
    /usr/bin/hdiutil detach "$disk_dev" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

accel=${T81_QEMU_ACCEL:-tcg}
machine_arg="virt,gic-version=3"
cpu_arg="cortex-a57"
if [[ "$accel" == "hvf" ]]; then
  machine_arg="virt,accel=hvf"
  cpu_arg="host"
fi

"$qemu_bin" \
  -nodefaults \
  -machine "$machine_arg" \
  -cpu "$cpu_arg" \
  -smp 1 \
  -m 512 \
  -nographic \
  -serial "file:$serial_log" \
  -drive if=pflash,format=raw,readonly=on,file="$edk2_code" \
  -drive if=pflash,format=raw,file="$vars_copy" \
  -drive if=virtio,format=raw,file="$probe_image" \
  >/dev/null 2>&1 &

qemu_pid=$!
echo "$qemu_pid" > "$pid_file"
/bin/sleep "$boot_wait"
kill "$qemu_pid" >/dev/null 2>&1 || true
wait "$qemu_pid" >/dev/null 2>&1 || true
qemu_pid=""

# Mount the FAT32 partition and extract artefacts.
attach_output=$(/usr/bin/hdiutil attach -readonly -nomount "$probe_image")
disk_dev=$(printf '%s\n' "$attach_output" | /usr/bin/awk '/GUID_partition_scheme/{print $1; exit}')
mount_dev=$(printf '%s\n' "$attach_output" | /usr/bin/awk '/EFI/{print $1; exit}')
if [[ -z "$disk_dev" || -z "$mount_dev" ]]; then
  echo "failed to resolve QEMU slice6 probe image devices" >&2
  exit 1
fi

/usr/sbin/diskutil mount "$mount_dev" >/dev/null
mount_point=$(/usr/sbin/diskutil info "$mount_dev" | /usr/bin/awk -F': *' '/Mount Point/ {print $2}')
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  echo "failed to resolve mount point for $mount_dev" >&2
  exit 1
fi

efi_marker_path="$mount_point/TERNOS/efi-slice6-ran.txt"
report_path="$mount_point/TERNOS/slice6-boot-report.txt"

efi_seen=0
report_seen=0
boot_banner_seen=0

[[ -f "$efi_marker_path" ]] && efi_seen=1
if [[ -f "$report_path" ]]; then
  /bin/cp "$report_path" "$report_copy"
  report_seen=1
fi

serial_bytes=0
if [[ -f "$serial_log" ]]; then
  serial_bytes=$(/usr/bin/wc -c < "$serial_log" | /usr/bin/tr -d ' ')
  if /usr/bin/grep -q 'Axion QEMU AArch64 EDK2 slice6' "$serial_log"; then
    boot_banner_seen=1
  fi
fi

/bin/cat > "$summary_file" <<EOF
probe_image=$probe_image
boot_wait_seconds=$boot_wait
accel=$accel
serial_log=$serial_log
serial_bytes=$serial_bytes
efi_marker_seen=$efi_seen
efi_marker_path=$efi_marker_path
report_seen=$report_seen
report_copy=$report_copy
boot_banner_seen=$boot_banner_seen
EOF

/usr/bin/hdiutil detach "$disk_dev" >/dev/null 2>&1 || true
disk_dev=""

if [[ "$efi_seen" -ne 1 || "$report_seen" -ne 1 || "$boot_banner_seen" -ne 1 ]]; then
  echo "QEMU AArch64 slice6 probe did not observe all required artefacts" >&2
  /bin/cat "$summary_file" >&2
  exit 1
fi

/bin/zsh "${0:A:h}/validate_qemu_armv8_slice6_reports.sh" "$output_dir"

echo "QEMU AArch64 slice6 probe succeeded."
echo "summary: $summary_file"
echo "serial:  $serial_log"
