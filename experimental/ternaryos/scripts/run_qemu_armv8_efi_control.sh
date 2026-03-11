#!/bin/zsh
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <arm-image> <output-dir>" >&2
  exit 2
fi

arm_image=$1
output_dir=$2

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
vars_copy="$output_dir/edk2-aarch64-vars.fd"
serial_log="$output_dir/qemu-armv8-control-serial.log"
monitor_log="$output_dir/qemu-armv8-control-monitor.log"
pid_file="$output_dir/qemu-armv8-control.pid"

/bin/cp "$edk2_vars_template" "$vars_copy"
/bin/rm -f "$serial_log" "$monitor_log" "$pid_file"

"$qemu_bin" \
  -machine virt,accel=hvf \
  -cpu host \
  -smp 1 \
  -m 512 \
  -nographic \
  -monitor "unix:$monitor_log,server,nowait" \
  -serial "file:$serial_log" \
  -drive if=pflash,format=raw,readonly=on,file="$edk2_code" \
  -drive if=pflash,format=raw,file="$vars_copy" \
  -drive if=virtio,format=raw,file="$arm_image" \
  >/dev/null 2>&1 &

echo $! > "$pid_file"
echo "started:"
echo "  pid_file=$pid_file"
echo "  serial_log=$serial_log"
echo "  vars_copy=$vars_copy"
