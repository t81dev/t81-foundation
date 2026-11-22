#!/usr/bin/env bash
# drivers/qemu/scripts/boot_demo.sh
#
# T81 / Axion — QEMU AArch64 Boot Demo
#
# Builds the slice6 EFI binary (if not already present), assembles a FAT32
# GPT disk image, and boots it under QEMU (TCG cortex-a57 + EDK2 AArch64)
# with serial output live on the terminal.
#
# Prerequisites — Linux (Ubuntu 24.04):
#   sudo apt-get install -y \
#     qemu-system-arm qemu-efi-aarch64 mtools parted \
#     cmake ninja-build clang-18 lld-18
#
# Prerequisites — macOS (Homebrew):
#   brew install qemu mtools cmake ninja llvm coreutils
#   # GNU parted must also be available on PATH for the GPT image setup.
#
# Usage:
#   ./drivers/qemu/scripts/boot_demo.sh
#
# Environment overrides:
#   BUILD_DIR   path to CMake build directory  (default: <repo>/build)
#   TIMEOUT     seconds before QEMU is stopped  (default: 60)
#   EDK2_CODE   optional path to the AArch64 firmware code image
#   EDK2_VARS   optional path to the AArch64 firmware variables image
#
# Serial output is also written to BUILD_DIR/boot-demo.serial.log.
# A timeout or a missing Axion shell prompt makes the demo fail.
#
# Press Ctrl-A X to exit QEMU early.

set -euo pipefail

# ── Locate repo root ──────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
TIMEOUT="${TIMEOUT:-60}"
IMG="$BUILD_DIR/demo_boot.img"
SERIAL_LOG="$BUILD_DIR/boot-demo.serial.log"

# ── Colour helpers ────────────────────────────────────────────────────────────
bold()  { printf '\033[1m%s\033[0m\n' "$*"; }
info()  { printf '\033[34m▶\033[0m  %s\n' "$*"; }
ok()    { printf '\033[32m✔\033[0m  %s\n' "$*"; }
die()   { printf '\033[31m✗\033[0m  %s\n' "$*" >&2; exit 1; }

check_cmd() {
  command -v "$1" &>/dev/null || die "Missing: $1 — see script header for install instructions"
}

resolve_timeout_cmd() {
  if command -v timeout &>/dev/null; then
    echo "timeout"
    return
  fi
  if command -v gtimeout &>/dev/null; then
    echo "gtimeout"
    return
  fi
  die "Missing: timeout (Linux) or gtimeout from coreutils (macOS) — the boot demo needs a bounded QEMU runtime"
}

# ── Prerequisites ─────────────────────────────────────────────────────────────
check_cmd qemu-system-aarch64
check_cmd mformat
check_cmd mmd
check_cmd mcopy
check_cmd cmake
check_cmd parted
check_cmd python3
TIMEOUT_CMD="$(resolve_timeout_cmd)"

# ── EDK2 firmware detection ───────────────────────────────────────────────────
find_edk2() {
  local candidates=(
    /usr/share/AAVMF/AAVMF_CODE.fd
    /usr/share/qemu/edk2-aarch64-code.fd
    /opt/homebrew/share/qemu/edk2-aarch64-code.fd
    /usr/local/share/qemu/edk2-aarch64-code.fd
  )
  for f in "${candidates[@]}"; do
    [[ -f "$f" ]] && { echo "$f"; return; }
  done
  die "EDK2 AArch64 firmware not found. Install qemu-efi-aarch64 (Linux) or brew install qemu (macOS)."
}

find_edk2_vars() {
  local candidates=(
    /usr/share/AAVMF/AAVMF_VARS.fd
    /usr/share/qemu/edk2-arm-vars.fd
    /opt/homebrew/share/qemu/edk2-arm-vars.fd
    /usr/local/share/qemu/edk2-arm-vars.fd
  )
  for f in "${candidates[@]}"; do [[ -f "$f" ]] && { echo "$f"; return; }; done
  die "EDK2 VARS firmware not found."
}

EDK2_CODE="${EDK2_CODE:-$(find_edk2)}"
EDK2_VARS="${EDK2_VARS:-$(find_edk2_vars)}"
[[ -f "$EDK2_CODE" && -r "$EDK2_CODE" ]] || die "Cannot read EDK2 code: $EDK2_CODE"
[[ -f "$EDK2_VARS" && -r "$EDK2_VARS" ]] || die "Cannot read EDK2 variables: $EDK2_VARS"

# ── Header ────────────────────────────────────────────────────────────────────
echo ""
bold "T81 / Axion — QEMU AArch64 Boot Demo"
bold "======================================"
echo ""

# ── Build EFI binary ──────────────────────────────────────────────────────────
EFI="$BUILD_DIR/ternaryos/qemu_slice6/BOOTAA64.EFI"
if [[ -f "$EFI" ]]; then
  ok "EFI binary already built: $(du -sh "$EFI" | cut -f1)  →  $(basename "$EFI")"
else
  info "Configuring CMake build…"
  cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DT81_ENABLE_TERNARYOS=ON \
    -DT81_BUILD_TESTS=OFF \
    -DT81_BUILD_EXAMPLES=OFF \
    -DT81_BUILD_BENCHMARKS=OFF \
    -DT81_BUILD_FUZZ_TESTS=OFF \
    -Wno-dev

  info "Building BOOTAA64.EFI…"
  cmake --build "$BUILD_DIR" \
    --target t81_ternaryos_qemu_slice6_efi \
    -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
  ok "Build complete."
fi
echo ""

# ── CanonFS block store image ─────────────────────────────────────────────────
CANON_IMG="$BUILD_DIR/canon_store.img"
info "Creating CanonFS raw block store (4 MiB, virtio-mmio slot 30 @ 0x0A003C00)…"
dd if=/dev/zero of="$CANON_IMG" bs=1048576 count=4 status=none

# Write a valid CanonFS superblock at byte offset 0 (729-byte LBA 0 block).
# Layout: magic "CST1" | entry_count uint32-LE | 680 B entries | overflow uint64-LE | 33 B pad.
python3 - "$CANON_IMG" <<'PYEOF'
import struct
import sys
magic          = b'CST1'
entry_count    = struct.pack('<I', 0)
entries        = bytes(17 * 40)
overflow_count = struct.pack('<Q', 0)
padding        = bytes(33)
superblock     = magic + entry_count + entries + overflow_count + padding
assert len(superblock) == 729
img = sys.argv[1]
with open(img, 'r+b') as f:
    f.write(superblock)
print(f"CST1 superblock written to {img}")
PYEOF

ok "CanonFS store: $(du -sh "$CANON_IMG" | cut -f1)"
echo ""

# ── Assemble FAT32 GPT disk image ─────────────────────────────────────────────
info "Assembling boot disk image (virtio-mmio slot 31 @ 0x0A003E00)…"
OFFSET=1048576  # 1 MiB — start of EFI partition

dd if=/dev/zero of="$IMG" bs=1048576 count=64 status=none
parted -s "$IMG" mklabel gpt
parted -s "$IMG" mkpart EFI fat32 1MiB 63MiB
parted -s "$IMG" set 1 esp on

mformat -i "${IMG}@@${OFFSET}" -F -v AXIONBOOT ::
mmd    -i "${IMG}@@${OFFSET}" ::/EFI ::/EFI/BOOT ::/TERNOS
mcopy  -i "${IMG}@@${OFFSET}" "$EFI" ::/EFI/BOOT/BOOTAA64.EFI

TEMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/t81-qemu-demo.XXXXXX")"
PROFILE_TMP="$TEMP_DIR/profile.txt"
VARS_TMP="$TEMP_DIR/vars.fd"
cleanup() {
  rm -f "$PROFILE_TMP" "$VARS_TMP"
  rmdir "$TEMP_DIR"
}
trap cleanup EXIT

COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
printf 'profile=qemu-armv8:AArch64/EDK2/slice6-demo\ngit_commit=%s\n' "$COMMIT" \
  > "$PROFILE_TMP"
mcopy -i "${IMG}@@${OFFSET}" "$PROFILE_TMP" ::/TERNOS/profile.txt

ok "Disk image: $(du -sh "$IMG" | cut -f1)"
echo ""

# ── Boot ──────────────────────────────────────────────────────────────────────
cp "$EDK2_VARS" "$VARS_TMP"

info "Booting under QEMU (timeout ${TIMEOUT}s — press Ctrl-A X to exit)…"
echo ""
echo "────────────────────────────────────────────────────────────────────────────────"

# Keep QEMU in the terminal foreground so its serial console can read stdin.
if "$TIMEOUT_CMD" --foreground "$TIMEOUT" qemu-system-aarch64 \
  -nodefaults \
  -machine virt,gic-version=3 \
  -cpu cortex-a57 \
  -m 512M \
  -nographic \
  -serial mon:stdio \
  -global virtio-mmio.force-legacy=off \
  -drive if=pflash,format=raw,readonly=on,file="$EDK2_CODE" \
  -drive if=pflash,format=raw,file="$VARS_TMP" \
  -drive id=boot0,if=none,format=raw,file="$IMG" \
  -device virtio-blk-device,drive=boot0,bootindex=0 \
  -drive id=canon0,if=none,format=raw,file="$CANON_IMG" \
  -device virtio-blk-device,drive=canon0,bootindex=1 | tee "$SERIAL_LOG"; then
  :
else
  statuses=("${PIPESTATUS[@]}")
  if (( statuses[0] != 0 )); then
    printf 'QEMU failed with exit status %s. Serial log: %s\n' "${statuses[0]}" "$SERIAL_LOG" >&2
    exit "${statuses[0]}"
  fi
  die "Could not write serial log: $SERIAL_LOG"
fi

LC_ALL=C grep -a -Eq '^\[axion@T81 tier=[0-9]+\]\$[[:blank:]]' "$SERIAL_LOG" ||
  die "Guest shell prompt was not observed. Serial log: $SERIAL_LOG"

echo "────────────────────────────────────────────────────────────────────────────────"
echo ""
ok "Boot verified. Serial log: $SERIAL_LOG"
