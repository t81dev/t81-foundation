#!/bin/zsh
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <arm-image> <output-dir> [boot-wait-seconds]" >&2
  exit 2
fi

arm_image=$1
output_dir=$2
boot_wait=${3:-8}

qemu_bin=/opt/homebrew/bin/qemu-system-aarch64
edk2_code=/opt/homebrew/share/qemu/edk2-aarch64-code.fd
edk2_vars_template=/opt/homebrew/share/qemu/edk2-arm-vars.fd

for path in "$qemu_bin" "$edk2_code" "$edk2_vars_template" "$arm_image"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required path: $path" >&2
    exit 1
  fi
done

/bin/mkdir -p "$output_dir"
probe_image="$output_dir/qemu-armv8-guest-probe.img"
vars_copy="$output_dir/edk2-aarch64-vars.fd"
serial_log="$output_dir/qemu-armv8-guest-serial.log"
pid_file="$output_dir/qemu-armv8-guest.pid"
summary_file="$output_dir/qemu-armv8-guest-summary.txt"
boot_report_copy="$output_dir/boot-report.txt"
startup_status_copy="$output_dir/startup-status.txt"
startup_shell_copy="$output_dir/startup-shell.txt"
startup_session_copy="$output_dir/startup-session.txt"
startup_history_copy="$output_dir/startup-history.txt"
startup_store_copy="$output_dir/startup-store.txt"
startup_ref_copy="$output_dir/startup-ref.txt"
startup_report_copy="$output_dir/startup-report.txt"
startup_phase4_copy="$output_dir/startup-phase4.txt"
boot_banner_seen=0

/bin/cp "$arm_image" "$probe_image"
/bin/cp "$edk2_vars_template" "$vars_copy"
/bin/rm -f "$serial_log" "$pid_file" "$summary_file" "$boot_report_copy" "$startup_status_copy" "$startup_shell_copy" "$startup_session_copy" "$startup_history_copy" "$startup_store_copy" "$startup_ref_copy" "$startup_report_copy" "$startup_phase4_copy"

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
  -machine virt,accel=hvf \
  -cpu host \
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

attach_output=$(/usr/bin/hdiutil attach -readonly -nomount "$probe_image")
disk_dev=$(printf '%s\n' "$attach_output" | /usr/bin/awk '/GUID_partition_scheme/{print $1; exit}')
mount_dev=$(printf '%s\n' "$attach_output" | /usr/bin/awk '/EFI/{print $1; exit}')
if [[ -z "$disk_dev" || -z "$mount_dev" ]]; then
  echo "failed to resolve QEMU probe image devices" >&2
  exit 1
fi

/usr/sbin/diskutil mount "$mount_dev" >/dev/null
mount_point=$(/usr/sbin/diskutil info "$mount_dev" | /usr/bin/awk -F': *' '/Mount Point/ {print $2}')
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  echo "failed to resolve mount point for $mount_dev" >&2
  exit 1
fi

startup_marker_path="$mount_point/TERNOS/startup-ran.txt"
ctrl_marker_path="$mount_point/TERNOS/efi-ctrl-ran.txt"
efi_marker_path="$mount_point/TERNOS/efi-ran.txt"
boot_report_path="$mount_point/TERNOS/boot-report.txt"
startup_status_path="$mount_point/TERNOS/startup-status.txt"
startup_shell_path="$mount_point/TERNOS/startup-shell.txt"
startup_session_path="$mount_point/TERNOS/startup-session.txt"
startup_history_path="$mount_point/TERNOS/startup-history.txt"
startup_store_path="$mount_point/TERNOS/startup-store.txt"
startup_ref_path="$mount_point/TERNOS/startup-ref.txt"
startup_report_path="$mount_point/TERNOS/startup-report.txt"
startup_phase4_path="$mount_point/TERNOS/startup-phase4.txt"

startup_seen=0
ctrl_seen=0
efi_seen=0
boot_report_seen=0
startup_status_seen=0
startup_shell_seen=0
startup_session_seen=0
startup_history_seen=0
startup_store_seen=0
startup_ref_seen=0
startup_report_seen=0
startup_phase4_seen=0

[[ -f "$startup_marker_path" ]] && startup_seen=1
[[ -f "$ctrl_marker_path" ]] && ctrl_seen=1
[[ -f "$efi_marker_path" ]] && efi_seen=1
if [[ -f "$boot_report_path" ]]; then
  /bin/cp "$boot_report_path" "$boot_report_copy"
  boot_report_seen=1
fi
if [[ -f "$startup_status_path" ]]; then
  /bin/cp "$startup_status_path" "$startup_status_copy"
  startup_status_seen=1
fi
if [[ -f "$startup_shell_path" ]]; then
  /bin/cp "$startup_shell_path" "$startup_shell_copy"
  startup_shell_seen=1
fi
if [[ -f "$startup_session_path" ]]; then
  /bin/cp "$startup_session_path" "$startup_session_copy"
  startup_session_seen=1
fi
if [[ -f "$startup_history_path" ]]; then
  /bin/cp "$startup_history_path" "$startup_history_copy"
  startup_history_seen=1
fi
if [[ -f "$startup_store_path" ]]; then
  /bin/cp "$startup_store_path" "$startup_store_copy"
  startup_store_seen=1
fi
if [[ -f "$startup_ref_path" ]]; then
  /bin/cp "$startup_ref_path" "$startup_ref_copy"
  startup_ref_seen=1
fi
if [[ -f "$startup_report_path" ]]; then
  /bin/cp "$startup_report_path" "$startup_report_copy"
  startup_report_seen=1
fi
if [[ -f "$startup_phase4_path" ]]; then
  /bin/cp "$startup_phase4_path" "$startup_phase4_copy"
  startup_phase4_seen=1
fi

boot_path_inference="unknown"
if [[ "$efi_seen" -eq 1 && "$startup_seen" -eq 0 && "$ctrl_seen" -eq 0 ]]; then
  boot_path_inference="default-bootaa64-efi"
elif [[ "$efi_seen" -eq 1 && "$ctrl_seen" -eq 1 ]]; then
  boot_path_inference="startup-or-control-chain"
fi

serial_bytes=0
if [[ -f "$serial_log" ]]; then
  serial_bytes=$(/usr/bin/wc -c < "$serial_log" | /usr/bin/tr -d ' ')
  if /usr/bin/grep -q 'Axion ARMv8 EFI stub' "$serial_log"; then
    boot_banner_seen=1
  fi
fi

/bin/cat > "$summary_file" <<EOF
probe_image=$probe_image
boot_wait_seconds=$boot_wait
serial_log=$serial_log
serial_bytes=$serial_bytes
startup_marker_seen=$startup_seen
startup_marker_path=$startup_marker_path
control_marker_seen=$ctrl_seen
control_marker_path=$ctrl_marker_path
efi_marker_seen=$efi_seen
efi_marker_path=$efi_marker_path
boot_report_seen=$boot_report_seen
boot_report_copy=$boot_report_copy
startup_status_seen=$startup_status_seen
startup_status_copy=$startup_status_copy
startup_shell_seen=$startup_shell_seen
startup_shell_copy=$startup_shell_copy
startup_session_seen=$startup_session_seen
startup_session_copy=$startup_session_copy
startup_history_seen=$startup_history_seen
startup_history_copy=$startup_history_copy
startup_store_seen=$startup_store_seen
startup_store_copy=$startup_store_copy
startup_ref_seen=$startup_ref_seen
startup_ref_copy=$startup_ref_copy
startup_report_seen=$startup_report_seen
startup_report_copy=$startup_report_copy
startup_phase4_seen=$startup_phase4_seen
startup_phase4_copy=$startup_phase4_copy
boot_banner_seen=$boot_banner_seen
boot_path_inference=$boot_path_inference
EOF

/usr/bin/hdiutil detach "$disk_dev" >/dev/null 2>&1 || true
disk_dev=""

if [[ "$efi_seen" -ne 1 || "$boot_report_seen" -ne 1 || "$startup_status_seen" -ne 1 || "$startup_shell_seen" -ne 1 || "$startup_session_seen" -ne 1 || "$startup_history_seen" -ne 1 || "$startup_store_seen" -ne 1 || "$startup_ref_seen" -ne 1 || "$startup_report_seen" -ne 1 || "$startup_phase4_seen" -ne 1 || "$boot_banner_seen" -ne 1 ]]; then
  echo "QEMU ARMv8 guest probe did not observe the staged BOOTAA64.EFI marker, startup status, startup shell, startup session, startup history, startup store, startup ref, startup report, startup phase4 report, boot report, and serial banner" >&2
  /bin/cat "$summary_file" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^platform_id=virtualbox-armv8:ARMv8Virtual/developer-lane$' "$boot_report_copy"; then
  echo "QEMU ARMv8 guest probe found boot report, but platform_id did not match" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$boot_report_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^hal_main_result=0$' "$boot_report_copy"; then
  echo "QEMU ARMv8 guest probe found boot report, but hal_main_result was not 0" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$boot_report_copy" >&2
  exit 1
fi

for expected in \
  '^os_name=Axion$' \
  '^phase=5$' \
  '^shell_mode=typed-builtins$' \
  '^storage_binding=virtualbox-ahci$' \
  '^display_binding=virtualbox-vmsvga$' \
  '^network_binding=virtualbox-e1000$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_status_copy"; then
    echo "QEMU ARMv8 guest probe found startup status, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_status_copy" >&2
    exit 1
  fi
done

for expected in \
  '^AXION_PHASE4_STARTUP$' \
  '^storage_binding=virtualbox-ahci$' \
  '^canonstore_index_entries_per_block=17$' \
  '^canonstore_recovered_entries=20$' \
  '^canonstore_second_cycle_entries=20$' \
  '^canonstore_torn_header_entries=20$' \
  '^canonstore_inventory_count=20$' \
  '^canonstore_overflow_active=true$' \
  '^canonstore_lookup_ok=20$' \
  '^canonstore_second_cycle_ok=20$' \
  '^canonstore_torn_header_ok=20$' \
  '^display_binding=virtualbox-vmsvga$' \
  '^display_present_count=1$' \
  '^network_binding=virtualbox-e1000$' \
  '^network_tx_frames=1$' \
  '^network_rx_frames=1$' \
  '^network_roundtrip_words=2$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_phase4_copy"; then
    echo "QEMU ARMv8 guest probe found startup phase4 report, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_phase4_copy" >&2
    exit 1
  fi
done

for expected in \
  '^AXION_STARTUP_SHELL$' \
  '^prompt=axion> $' \
  '^mode=typed-builtins$' \
  '^history_anchor=durable$' \
  '^session_view=local+durable$'
do
  if ! /usr/bin/grep -q "$expected" "$startup_shell_copy"; then
    echo "QEMU ARMv8 guest probe found startup shell, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_shell_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^commands=.*show session.*show ref <canonref>.*history show durable.*$' "$startup_shell_copy"; then
  echo "QEMU ARMv8 guest probe found startup shell, but command surface summary was incomplete" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_shell_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_SESSION' \
  'profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC' \
  'storage=virtualbox-ahci' \
  'display=virtualbox-vmsvga' \
  'session_command_count=6' \
  'durable_ref_count=1' \
  'durable_anchor=present'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_session_copy"; then
    echo "QEMU ARMv8 guest probe found startup session, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_session_copy" >&2
    exit 1
  fi
done

for expected in \
  'AXION_STARTUP_HISTORY' \
  'command=history show durable'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_history_copy"; then
    echo "QEMU ARMv8 guest probe found startup history, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_history_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^result=history durable ' "$startup_history_copy"; then
  echo "QEMU ARMv8 guest probe found startup history, but durable history result prefix was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_history_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^phase5 durable transcript$' "$startup_history_copy"; then
  echo "QEMU ARMv8 guest probe found startup history, but durable payload text was missing" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_history_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_STORE' \
  'command=store ls'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_store_copy"; then
    echo "QEMU ARMv8 guest probe found startup store, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_store_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^result=store refs 1' "$startup_store_copy"; then
  echo "QEMU ARMv8 guest probe found startup store, but store refs summary was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_store_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_REF' \
  'result=show ref '
do
  if ! /usr/bin/grep -F -q "$expected" "$startup_ref_copy"; then
    echo "QEMU ARMv8 guest probe found startup ref, but expected field was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_ref_copy" >&2
    exit 1
  fi
done

if ! /usr/bin/grep -q '^command=show ref ' "$startup_ref_copy"; then
  echo "QEMU ARMv8 guest probe found startup ref, but command line was malformed" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_ref_copy" >&2
  exit 1
fi

if ! /usr/bin/grep -q '^phase5 durable transcript$' "$startup_ref_copy"; then
  echo "QEMU ARMv8 guest probe found startup ref, but durable payload text was missing" >&2
  /bin/cat "$summary_file" >&2
  /bin/cat "$startup_ref_copy" >&2
  exit 1
fi

for expected in \
  'AXION_STARTUP_REPORT' \
  '[session]' \
  '[shell]' \
  '[history]' \
  '[store]' \
  '[ref]'
do
  if ! /usr/bin/grep -F -x -q "$expected" "$startup_report_copy"; then
    echo "QEMU ARMv8 guest probe found startup report, but expected section was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_report_copy" >&2
    exit 1
  fi
done

for expected in \
  'profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC' \
  'history_anchor=durable' \
  'command=store ls' \
  'command=history show durable'
do
  if ! /usr/bin/grep -F -q "$expected" "$startup_report_copy"; then
    echo "QEMU ARMv8 guest probe found startup report, but expected content was missing: $expected" >&2
    /bin/cat "$summary_file" >&2
    /bin/cat "$startup_report_copy" >&2
    exit 1
  fi
done

echo "QEMU ARMv8 guest probe succeeded."
echo "summary: $summary_file"
echo "serial: $serial_log"
