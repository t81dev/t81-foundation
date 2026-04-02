#!/usr/bin/env bash
# drivers/qemu/scripts/boot_demo_x86.sh
#
# T81 / Axion — QEMU x86_64 Boot Demo
#
# Builds BOOTX64.EFI (if not already present), assembles a FAT32 GPT image,
# and boots it under QEMU q35 + OVMF with serial output live on the terminal.
# Uses COM1 (0x3F8) for serial I/O and RDTSC for live uptime counters.
#
# Prerequisites — Linux (Ubuntu 24.04):
#   sudo apt-get install -y \
#     qemu-system-x86 ovmf mtools parted \
#     cmake ninja-build clang-18 lld-18
#
# Prerequisites — macOS (Homebrew):
#   brew install qemu mtools cmake ninja llvm
#   # OVMF: brew install --cask utm  OR download from tianocore.org
#
# Usage:
#   ./drivers/qemu/scripts/boot_demo_x86.sh
#
# Environment overrides:
#   BUILD_DIR   path to CMake build directory  (default: <repo>/build)
#   TIMEOUT     seconds before QEMU is killed  (default: 60)
#
# Press Ctrl-A X to exit QEMU early.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
TIMEOUT="${TIMEOUT:-60}"
IMG="$BUILD_DIR/demo_x86_boot.img"

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

make_temp_fd() {
  mktemp "${TMPDIR:-/tmp}/t81-qemu-x86-demo.XXXXXX.fd"
}

check_cmd qemu-system-x86_64
check_cmd mtools
check_cmd cmake
check_cmd parted
check_cmd python3
TIMEOUT_CMD="$(resolve_timeout_cmd)"

# ── OVMF firmware detection ───────────────────────────────────────────────────
find_ovmf_code() {
  local candidates=(
    /usr/share/OVMF/OVMF_CODE.fd
    /usr/share/edk2/ovmf/OVMF_CODE.fd
    /opt/homebrew/share/qemu/edk2-x86_64-code.fd
    /usr/local/share/qemu/edk2-x86_64-code.fd
  )
  for f in "${candidates[@]}"; do [[ -f "$f" ]] && { echo "$f"; return; }; done
  die "OVMF firmware not found. Install ovmf (Linux) or brew install qemu (macOS)."
}

find_ovmf_vars() {
  local candidates=(
    /usr/share/OVMF/OVMF_VARS.fd
    /usr/share/edk2/ovmf/OVMF_VARS.fd
    /opt/homebrew/share/qemu/edk2-x86_64-code.fd
  )
  for f in "${candidates[@]}"; do [[ -f "$f" ]] && { echo "$f"; return; }; done
  die "OVMF VARS firmware not found."
}

OVMF_CODE="$(find_ovmf_code)"
OVMF_VARS="$(find_ovmf_vars)"

echo ""
bold "T81 / Axion — QEMU x86_64 Boot Demo"
bold "======================================"
echo ""

# ── Build EFI binary ──────────────────────────────────────────────────────────
EFI="$BUILD_DIR/ternaryos/qemu_x86_64/BOOTX64.EFI"
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

  info "Building BOOTX64.EFI…"
  cmake --build "$BUILD_DIR" \
    --target t81_ternaryos_qemu_x86_64_efi \
    -j"$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)"
  ok "Build complete."
fi
echo ""

# ── CanonFS block store image ─────────────────────────────────────────────────
# QEMU q35 uses virtio-blk-pci (PCI transport) for -drive if=virtio.
# The freestanding bridge first tries MMIO probe at slot 1 (0x0A000200);
# on q35 that fails, then falls back to PCI config-space scan which counts
# virtio-blk-pci devices: ≥2 → boot disk + CanonFS disk → (persistent, virtio-blk).
CANON_IMG="$BUILD_DIR/canon_store_x86.img"
info "Creating CanonFS raw block store (4 MiB, virtio-blk-pci slot 2 on q35)…"
dd if=/dev/zero of="$CANON_IMG" bs=1M count=4 status=none

# Write a valid CanonFS superblock at byte offset 0 (729-byte LBA 0 block).
python3 - <<'PYEOF'
import struct, os, sys
magic          = b'CST1'
entry_count    = struct.pack('<I', 0)
entries        = bytes(17 * 40)
overflow_count = struct.pack('<Q', 0)
padding        = bytes(33)
superblock     = magic + entry_count + entries + overflow_count + padding
assert len(superblock) == 729
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
info "Assembling boot disk image…"
OFFSET=1048576

dd if=/dev/zero of="$IMG" bs=1M count=64 status=none
parted -s "$IMG" mklabel gpt
parted -s "$IMG" mkpart EFI fat32 1MiB 63MiB
parted -s "$IMG" set 1 esp on

mformat -i "${IMG}@@${OFFSET}" -F -v AXIONX64 ::
mmd    -i "${IMG}@@${OFFSET}" ::/EFI ::/EFI/BOOT
mcopy  -i "${IMG}@@${OFFSET}" "$EFI" ::/EFI/BOOT/BOOTX64.EFI

ok "Disk image: $(du -sh "$IMG" | cut -f1)"
echo ""

# ── Boot ──────────────────────────────────────────────────────────────────────
VARS_TMP="$(make_temp_fd)"
cp "$OVMF_VARS" "$VARS_TMP"
trap 'rm -f "$VARS_TMP"' EXIT

info "Booting under QEMU x86_64 (timeout ${TIMEOUT}s — press Ctrl-A X to exit)…"
echo ""
echo "────────────────────────────────────────────────────────────────────────────────"

"$TIMEOUT_CMD" "$TIMEOUT" qemu-system-x86_64 \
  -machine q35 \
  -cpu qemu64 \
  -m 512M \
  -nographic \
  -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
  -drive if=pflash,format=raw,file="$VARS_TMP" \
  -drive if=virtio,format=raw,file="$IMG" \
  -drive if=virtio,format=raw,file="$CANON_IMG" \
  2>/dev/null || true

echo "────────────────────────────────────────────────────────────────────────────────"
echo ""
ok "Boot demo complete.  Captured serial log: drivers/qemu/sample-boot-log-x86.txt"
