#!/bin/zsh
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <x86-image> <output-dir> [boot-wait-seconds]" >&2
  exit 2
fi

x86_image=$1
output_dir=$2
boot_wait=${3:-12}

qemu_bin=/opt/homebrew/bin/qemu-system-x86_64
edk2_code=/opt/homebrew/share/qemu/edk2-x86_64-code.fd
edk2_vars_template=/opt/homebrew/share/qemu/edk2-i386-vars.fd

for path in "$qemu_bin" "$edk2_code" "$edk2_vars_template" "$x86_image"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required path: $path" >&2
    exit 1
  fi
done

/bin/mkdir -p "$output_dir"
probe_image="$output_dir/qemu-x86_64-guest-probe.img"
vars_copy="$output_dir/edk2-x86_64-vars.fd"
serial_log="$output_dir/qemu-x86_64-guest-serial.log"
pid_file="$output_dir/qemu-x86_64-guest.pid"
summary_file="$output_dir/qemu-x86_64-guest-summary.txt"
marker_copy="$output_dir/efi-ran.txt"
boot_report_copy="$output_dir/boot-report.txt"
startup_status_copy="$output_dir/startup-status.txt"
expected_boot_copy="$output_dir/expected-boot-report.txt"
expected_status_copy="$output_dir/expected-startup-status.txt"

/bin/cp "$x86_image" "$probe_image"
/bin/cp "$edk2_vars_template" "$vars_copy"
/bin/rm -f "$serial_log" "$pid_file" "$summary_file" \
  "$marker_copy" "$boot_report_copy" "$startup_status_copy" \
  "$expected_boot_copy" "$expected_status_copy"

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

"$qemu_bin" \
  -machine q35,accel=tcg \
  -cpu qemu64 \
  -smp 1 \
  -m 512 \
  -nographic \
  -serial "file:$serial_log" \
  -drive if=pflash,format=raw,readonly=on,file="$edk2_code" \
  -drive if=pflash,format=raw,file="$vars_copy" \
  -drive if=ide,format=raw,file="$probe_image" \
  >/dev/null 2>&1 &

qemu_pid=$!
echo "$qemu_pid" > "$pid_file"
/bin/sleep "$boot_wait"
kill "$qemu_pid" >/dev/null 2>&1 || true
wait "$qemu_pid" >/dev/null 2>&1 || true
qemu_pid=""

disk_dev=$(/usr/bin/hdiutil attach -readonly -nomount "$probe_image" | /usr/bin/awk 'NR==1{print $1}')
if [[ -z "$disk_dev" ]]; then
  echo "failed to resolve QEMU x86_64 probe image device" >&2
  exit 1
fi

/usr/sbin/diskutil mountDisk "$disk_dev" >/dev/null
mount_point=$(/usr/sbin/diskutil info "$disk_dev" | /usr/bin/awk -F': *' '/Mount Point/ {print $2}')
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  mount_point=$(/usr/sbin/diskutil info "${disk_dev}s1" 2>/dev/null | /usr/bin/awk -F': *' '/Mount Point/ {print $2}')
fi
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  echo "failed to resolve mount point for $disk_dev" >&2
  exit 1
fi

marker_path="$mount_point/TERNOS/efi-ran.txt"
boot_report_path="$mount_point/TERNOS/boot-report.txt"
startup_status_path="$mount_point/TERNOS/startup-status.txt"
expected_boot_path="$mount_point/TERNOS/expected-boot-report.txt"
expected_status_path="$mount_point/TERNOS/expected-startup-status.txt"

marker_seen=0
boot_report_seen=0
startup_status_seen=0
expected_boot_seen=0
expected_status_seen=0

if [[ -f "$marker_path" ]]; then
  /bin/cp "$marker_path" "$marker_copy"
  marker_seen=1
fi
if [[ -f "$boot_report_path" ]]; then
  /bin/cp "$boot_report_path" "$boot_report_copy"
  boot_report_seen=1
fi
if [[ -f "$startup_status_path" ]]; then
  /bin/cp "$startup_status_path" "$startup_status_copy"
  startup_status_seen=1
fi
if [[ -f "$expected_boot_path" ]]; then
  /bin/cp "$expected_boot_path" "$expected_boot_copy"
  expected_boot_seen=1
fi
if [[ -f "$expected_status_path" ]]; then
  /bin/cp "$expected_status_path" "$expected_status_copy"
  expected_status_seen=1
fi

serial_bytes=0
if [[ -f "$serial_log" ]]; then
  serial_bytes=$(/usr/bin/wc -c < "$serial_log" | /usr/bin/tr -d ' ')
fi

/bin/cat > "$summary_file" <<EOF
probe_image=$probe_image
boot_wait_seconds=$boot_wait
serial_log=$serial_log
serial_bytes=$serial_bytes
efi_marker_seen=$marker_seen
efi_marker_copy=$marker_copy
boot_report_seen=$boot_report_seen
boot_report_copy=$boot_report_copy
startup_status_seen=$startup_status_seen
startup_status_copy=$startup_status_copy
expected_boot_seen=$expected_boot_seen
expected_boot_copy=$expected_boot_copy
expected_status_seen=$expected_status_seen
expected_status_copy=$expected_status_copy
EOF

/usr/bin/hdiutil detach "$disk_dev" >/dev/null 2>&1 || true
disk_dev=""

if [[ "$marker_seen" -ne 1 || "$boot_report_seen" -ne 1 || "$startup_status_seen" -ne 1 || \
      "$expected_boot_seen" -ne 1 || "$expected_status_seen" -ne 1 ]]; then
  echo "QEMU x86_64 guest probe did not observe the staged EFI marker, expected contract files, boot report, and startup status" >&2
  /bin/cat "$summary_file" >&2
  exit 1
fi

/bin/zsh "${0:A:h}/validate_virtualbox_x86_64_handoff.sh" \
  "$expected_boot_copy" \
  "$expected_status_copy" \
  "$output_dir"

echo "QEMU x86_64 guest probe succeeded."
echo "summary: $summary_file"
echo "serial: $serial_log"
