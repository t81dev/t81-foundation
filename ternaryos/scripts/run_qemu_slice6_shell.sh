#!/bin/zsh
set -euo pipefail

export PATH="/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:${PATH:-}"

# run_qemu_slice6_shell.sh
#
# Builds the Slice 6 QEMU AArch64 disk image from the current build tree and
# boots it interactively, attaching the serial console to stdio so the user can
# work directly at the `t81sh` prompt.
#
# Usage:
#   run_qemu_slice6_shell.sh <build-dir> [canon-store-img]
#
# <build-dir>        CMake build directory containing BOOTAA64.EFI.
# [canon-store-img]  Optional raw CanonFS image to attach as the second
#                    virtio-mmio block device.
#
# Environment overrides:
#   T81_QEMU_BIN        Path to qemu-system-aarch64
#   T81_EDK2_CODE       Path to edk2-aarch64-code.fd
#   T81_EDK2_VARS       Path to edk2-arm-vars.fd template
#   T81_QEMU_ACCEL      Force accelerator: hvf or tcg

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <build-dir> [canon-store-img]" >&2
  exit 2
fi

build_dir=$1
canon_store=${2:-}

script_dir=${0:A:h}
repo_root=${script_dir:h:h}

qemu_bin=${T81_QEMU_BIN:-/opt/homebrew/bin/qemu-system-aarch64}
edk2_code=${T81_EDK2_CODE:-/opt/homebrew/share/qemu/edk2-aarch64-code.fd}
edk2_vars_template=${T81_EDK2_VARS:-/opt/homebrew/share/qemu/edk2-arm-vars.fd}

for path in "$qemu_bin" "$edk2_code" "$edk2_vars_template" "$build_dir"; do
  if [[ ! -e "$path" ]]; then
    echo "missing required path: $path" >&2
    exit 1
  fi
done

if [[ -n "$canon_store" && ! -e "$canon_store" ]]; then
  echo "missing canon store image: $canon_store" >&2
  exit 1
fi

output_dir="$build_dir/ternaryos/qemu_slice6_shell"
/bin/mkdir -p "$output_dir"

/bin/zsh "$script_dir/build_qemu_slice6_artifact.sh" "$build_dir" "$output_dir"

arm_image="$output_dir/qemu_slice6_guest.img"
vars_copy="$output_dir/edk2-aarch64-vars.fd"
/bin/cp "$edk2_vars_template" "$vars_copy"

accel=${T81_QEMU_ACCEL:-}
host_arch=$(/usr/bin/uname -m)
if [[ -z "$accel" ]]; then
  # Slice6 has been more reliable under TCG than HVF on local developer
  # laptops, especially through the later EL0 scheduler phases and shell handoff.
  # Keep HVF available as an explicit override, but default the launcher to TCG.
  accel=tcg
fi

machine_arg=""
cpu_arg=""
if [[ "$accel" == "hvf" ]]; then
  machine_arg="virt,accel=hvf,gic-version=3"
  cpu_arg="host"
else
  machine_arg="virt,gic-version=3"
  cpu_arg="cortex-a57"
fi

echo "booting slice6 shell:"
echo "  image: $arm_image"
echo "  vars:  $vars_copy"
if [[ -n "$canon_store" ]]; then
  echo "  canon: $canon_store"
fi
echo "  accel: $accel"
echo "  exit:  Ctrl-a x"

cmd=(
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

if [[ -n "$canon_store" ]]; then
  cmd+=(
    -drive "id=canon0,if=none,format=raw,file=$canon_store"
    -device "virtio-blk-device,drive=canon0,bootindex=1"
  )
fi

exec "${cmd[@]}"
