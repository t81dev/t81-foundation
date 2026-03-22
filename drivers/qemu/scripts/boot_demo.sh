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
#     qemu-system-arm qemu-efi-aarch64 mtools \
#     cmake ninja-build clang-18 lld-18
#
# Prerequisites — macOS (Homebrew):
#   brew install qemu mtools cmake ninja llvm
#   # EDK2: brew install --cask utm  OR download AAVMF manually
#
# Usage:
#   ./drivers/qemu/scripts/boot_demo.sh
#
# Environment overrides:
#   BUILD_DIR   path to CMake build directory  (default: <repo>/build)
#   TIMEOUT     seconds before QEMU is killed  (default: 60)
#
# Press Ctrl-A X to exit QEMU early.

set -euo pipefail

# ── Locate repo root ──────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
TIMEOUT="${TIMEOUT:-60}"
IMG="$BUILD_DIR/demo_boot.img"

# ── Colour helpers ────────────────────────────────────────────────────────────
bold()  { printf '\033[1m%s\033[0m\n' "$*"; }
info()  { printf '\033[34m▶\033[0m  %s\n' "$*"; }
ok()    { printf '\033[32m✔\033[0m  %s\n' "$*"; }
die()   { printf '\033[31m✗\033[0m  %s\n' "$*" >&2; exit 1; }

check_cmd() {
  command -v "$1" &>/dev/null || die "Missing: $1 — see script header for install instructions"
}

# ── Prerequisites ─────────────────────────────────────────────────────────────
check_cmd qemu-system-aarch64
check_cmd mtools
check_cmd cmake
check_cmd parted

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
    /usr/share/qemu/edk2-aarch64-code.fd   # macOS: use code fd as vars (read-only copy)
    /opt/homebrew/share/qemu/edk2-aarch64-code.fd
  )
  for f in "${candidates[@]}"; do [[ -f "$f" ]] && { echo "$f"; return; }; done
  die "EDK2 VARS firmware not found."
}

EDK2_CODE="$(find_edk2)"
EDK2_VARS="$(find_edk2_vars)"

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
    -Wno-dev 2>&1 | grep -E '^\-\-' | head -20 || true

  info "Building BOOTAA64.EFI…"
  cmake --build "$BUILD_DIR" \
    --target t81_ternaryos_qemu_slice6_efi \
    -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
  ok "Build complete."
fi
echo ""

# ── CanonFS block store image ─────────────────────────────────────────────────
CANON_IMG="$BUILD_DIR/canon_store.img"
info "Creating CanonFS raw block store (4 MiB, virtio-mmio slot 1 @ 0x0A000200)…"
dd if=/dev/zero of="$CANON_IMG" bs=1M count=4 status=none

# Write a valid CanonFS superblock at byte offset 0 (729-byte LBA 0 block).
# Layout: magic "CST1" | entry_count uint32-LE | 680 B entries | overflow uint64-LE | 33 B pad.
python3 - <<'PYEOF'
import struct
magic          = b'CST1'
entry_count    = struct.pack('<I', 0)
entries        = bytes(17 * 40)
overflow_count = struct.pack('<Q', 0)
padding        = bytes(33)
superblock     = magic + entry_count + entries + overflow_count + padding
assert len(superblock) == 729
import os, sys
img = os.environ.get('CANON_IMG', '')
if not img:
    print("CANON_IMG not set", file=sys.stderr); sys.exit(1)
with open(img, 'r+b') as f:
    f.write(superblock)
print(f"CST1 superblock written to {img}")
PYEOF

ok "CanonFS store: $(du -sh "$CANON_IMG" | cut -f1)"
echo ""

# ── Assemble FAT32 GPT disk image ─────────────────────────────────────────────
info "Assembling boot disk image (slot 0 @ 0x0A000000)…"
OFFSET=1048576  # 1 MiB — start of EFI partition

dd if=/dev/zero of="$IMG" bs=1M count=64 status=none
parted -s "$IMG" mklabel gpt
parted -s "$IMG" mkpart EFI fat32 1MiB 63MiB
parted -s "$IMG" set 1 esp on

mformat -i "${IMG}@@${OFFSET}" -F -v AXIONBOOT ::
mmd    -i "${IMG}@@${OFFSET}" ::/EFI ::/EFI/BOOT ::/TERNOS
mcopy  -i "${IMG}@@${OFFSET}" "$EFI" ::/EFI/BOOT/BOOTAA64.EFI
mcopy  -i "${IMG}@@${OFFSET}" "$EFI" ::/EFI/BOOT/bootaa64.efi

COMMIT="$(git -C "$REPO_ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
printf 'profile=qemu-armv8:AArch64/EDK2/slice6-demo\ngit_commit=%s\n' "$COMMIT" \
  > /tmp/t81_demo_profile.txt
mcopy -i "${IMG}@@${OFFSET}" /tmp/t81_demo_profile.txt ::/TERNOS/profile.txt

ok "Disk image: $(du -sh "$IMG" | cut -f1)"
echo ""

# ── Boot ──────────────────────────────────────────────────────────────────────
VARS_TMP="$(mktemp --suffix=.fd)"
cp "$EDK2_VARS" "$VARS_TMP"
trap 'rm -f "$VARS_TMP"' EXIT

info "Booting under QEMU (timeout ${TIMEOUT}s — press Ctrl-A X to exit)…"
echo ""
echo "────────────────────────────────────────────────────────────────────────────────"

timeout "$TIMEOUT" qemu-system-aarch64 \
  -machine virt \
  -cpu cortex-a57 \
  -m 512M \
  -nographic \
  -drive if=pflash,format=raw,readonly=on,file="$EDK2_CODE" \
  -drive if=pflash,format=raw,file="$VARS_TMP" \
  -drive if=virtio,format=raw,file="$IMG" \
  -drive if=virtio,format=raw,file="$CANON_IMG" \
  2>/dev/null || true

echo "────────────────────────────────────────────────────────────────────────────────"
echo ""
ok "Boot demo complete.  Captured serial log: drivers/qemu/sample-boot-log.txt"
