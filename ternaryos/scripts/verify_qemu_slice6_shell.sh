#!/bin/zsh
set -euo pipefail

export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:${PATH:-}"

# verify_qemu_slice6_shell.sh
#
# Builds the slice6 guest image, boots it under QEMU, sends a short serial
# command batch to t81sh, captures the output, and validates that the shell
# reaches the prompt and exposes the current operator command surface.
#
# Usage:
#   verify_qemu_slice6_shell.sh <build-dir> [output-dir] [canon-store-img]

if [[ $# -lt 1 || $# -gt 3 ]]; then
  echo "usage: $0 <build-dir> [output-dir] [canon-store-img]" >&2
  exit 2
fi

build_dir=$1
output_dir=${2:-"$build_dir/ternaryos/qemu_slice6_shell_smoke"}
canon_store_img=${3:-}

script_dir=${0:A:h}
qemu_bin=${T81_QEMU_BIN:-/opt/homebrew/bin/qemu-system-aarch64}
edk2_code=${T81_EDK2_CODE:-/opt/homebrew/share/qemu/edk2-aarch64-code.fd}
edk2_vars_template=${T81_EDK2_VARS:-/opt/homebrew/share/qemu/edk2-arm-vars.fd}

for path in "$qemu_bin" "$edk2_code" "$edk2_vars_template" "$build_dir"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required path: $path" >&2
    exit 1
  fi
done
if [[ -n "$canon_store_img" && ! -e "$canon_store_img" ]]; then
  echo "missing required path: $canon_store_img" >&2
  exit 1
fi

/bin/mkdir -p "$output_dir"
/bin/zsh "$script_dir/build_qemu_slice6_artifact.sh" "$build_dir" "$output_dir"

arm_image="$output_dir/qemu_slice6_guest.img"
vars_copy="$output_dir/edk2-aarch64-vars.fd"
log_file="$output_dir/serial.log"
stdin_fifo="$output_dir/serial.in"

/bin/cp "$edk2_vars_template" "$vars_copy"
/bin/rm -f "$log_file"
/bin/rm -f "$stdin_fifo"
/usr/bin/mkfifo "$stdin_fifo"

cleanup() {
  if [[ -n "${qemu_pid:-}" ]]; then
    /bin/kill "$qemu_pid" >/dev/null 2>&1 || true
    /bin/wait "$qemu_pid" 2>/dev/null || true
  fi
  /bin/rm -f "$stdin_fifo"
}
trap cleanup EXIT

accel=${T81_QEMU_ACCEL:-tcg}
machine_arg="virt,gic-version=3"
cpu_arg="cortex-a57"
if [[ "$accel" == "hvf" ]]; then
  machine_arg="virt,accel=hvf,gic-version=3"
  cpu_arg="host"
fi

qemu_cmd=(
  "$qemu_bin"
  -nodefaults
  -machine "$machine_arg"
  -cpu "$cpu_arg"
  -smp 1
  -m 512
  -nographic
  -monitor none
  -serial stdio
  -global virtio-mmio.force-legacy=off
  -drive "if=pflash,format=raw,readonly=on,file=$edk2_code"
  -drive "if=pflash,format=raw,file=$vars_copy"
  -drive "id=boot0,if=none,format=raw,file=$arm_image"
  -device "virtio-blk-device,drive=boot0,bootindex=0"
)
if [[ -n "$canon_store_img" ]]; then
  qemu_cmd+=(
    -drive "id=canon0,if=none,format=raw,file=$canon_store_img"
    -device "virtio-blk-device,drive=canon0,bootindex=1"
  )
fi

"${qemu_cmd[@]}" <"$stdin_fifo" >"$log_file" 2>&1 &
qemu_pid=$!

{
  /bin/sleep 10
  if [[ -n "$canon_store_img" ]]; then
    printf 'help\rcanonfs\rcanonfs ls\rcanonfs hash proc-stub\rcanonfs hash wait-test-manifest\rcanonfs hash wait-test\rcanonfs run proc-stub\rirq\rel0\rfaults\rgov\r'
  else
    printf 'help\rcanonfs\rirq\rel0\rfaults\rgov\r'
  fi
} >"$stdin_fifo" &
writer_pid=$!

/bin/sleep 14
/bin/kill "$qemu_pid" >/dev/null 2>&1 || true
/bin/wait "$qemu_pid" 2>/dev/null || true
/bin/wait "$writer_pid" 2>/dev/null || true

check() {
  local needle=$1
  if ! /usr/bin/grep -Fq "$needle" "$log_file"; then
    echo "slice6 shell smoke-check failed: missing log line:" >&2
    echo "  $needle" >&2
    echo "log: $log_file" >&2
    exit 1
  fi
}

check "[axion] t81sh: ready (principal=axion, tier=1)"
check "[axion@T81 tier=1]$"
check "help      -- show available commands"
check "[canonfs]"
check "[irq]"
check "[el0]"
check "[faults]"
check "[governance]"
if [[ -n "$canon_store_img" ]]; then
  check "[canonfs inventory]"
  check "alias         : proc-stub"
  check "code_hash     :"
  check "hash_source   : computed payload (T81X v1)"
  check "alias         : wait-test-manifest"
  check "hash_source   : stored header (T81M)"
  check "entry_count   : 1"
  check "launch        : proc-stub"
  check "complete      : returned to shell"
fi

echo "Slice6 shell smoke-check succeeded."
echo "log: $log_file"
