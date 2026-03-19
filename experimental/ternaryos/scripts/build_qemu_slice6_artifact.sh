#!/bin/zsh
set -euo pipefail

# build_qemu_slice6_artifact.sh
#
# Builds the Slice 6 QEMU AArch64 raw disk image from the compiled
# BOOTAA64.EFI.  The image contains:
#
#   EFI/BOOT/BOOTAA64.EFI   — the freestanding EFI application
#   EFI/BOOT/bootaa64.efi   — lowercase alias (EDK2 convention)
#   TERNOS/profile.txt      — image metadata
#
# The output is a raw GUID-partitioned FAT32 .img suitable for QEMU virtio
# block (no VDI required — QEMU accepts raw images directly).
#
# Usage: build_qemu_slice6_artifact.sh <build-dir> <output-dir>
#
# <build-dir>   CMake binary directory (the root build dir).
# <output-dir>  Where to write qemu_slice6_guest.img and qemu_slice6_guest.img.sha256.

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <build-dir> <output-dir>" >&2
  exit 2
fi

build_dir=$1
output_dir=$2
efi_bin="$build_dir/ternaryos/qemu_slice6/BOOTAA64.EFI"

if [[ ! -f "$efi_bin" ]]; then
  echo "missing EFI binary: $efi_bin" >&2
  exit 1
fi

for tool in hdiutil diskutil newfs_msdos shasum; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "required tool not found: $tool" >&2
    exit 1
  fi
done

mkdir -p "$output_dir"
staging_dir="$output_dir/staging"
rm -rf "$staging_dir"
mkdir -p "$staging_dir/EFI/BOOT" "$staging_dir/TERNOS"

cp "$efi_bin" "$staging_dir/EFI/BOOT/BOOTAA64.EFI"
cp "$efi_bin" "$staging_dir/EFI/BOOT/bootaa64.efi"

git_rev=$(git -C "$(dirname "$0")/../../.." rev-parse --short HEAD 2>/dev/null || echo "unknown")
build_date=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

cat > "$staging_dir/TERNOS/profile.txt" <<EOF
profile=qemu-armv8:AArch64/EDK2/slice6-boot-probe
guest_arch=qemu_slice6
validation_lane=qemu-armv8-slice6-probe
git_commit=$git_rev
generated_utc=$build_date
artifact_status=efi-boot-candidate
boot_gap=none
EOF

image_path="$output_dir/qemu_slice6_guest.img"
rm -f "$image_path" "$image_path.sha256"

temp_dmg="$output_dir/qemu_slice6_guest.build.dmg"
temp_raw="$output_dir/qemu_slice6_guest.build.raw"
rm -f "$temp_dmg" "$temp_raw" "${temp_raw}.dmg"

dev=""
cleanup() {
  if [[ -n "${dev:-}" ]]; then
    hdiutil detach "$dev" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

hdiutil create -quiet -size 64m -layout GPTSPUD -partitionType EFI "$temp_dmg"
attach_output=$(hdiutil attach -nomount "$temp_dmg")
dev=$(printf '%s\n' "$attach_output" | awk '/GUID_partition_scheme/{print $1; exit}')
mount_dev=$(printf '%s\n' "$attach_output" | awk '/EFI/{print $1; exit}')
if [[ -z "$dev" || -z "$mount_dev" ]]; then
  echo "failed to resolve slice6 EFI image devices" >&2
  exit 1
fi

/sbin/newfs_msdos -F 32 -v SLICE6GUEST "$mount_dev" >/dev/null
diskutil mount "$mount_dev" >/dev/null
mount_point=$(diskutil info "$mount_dev" | awk -F': *' '/Mount Point/ {print $2}')
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  echo "failed to resolve mount point for $mount_dev" >&2
  exit 1
fi

cp -R "$staging_dir/." "$mount_point/"
sync
hdiutil detach "$dev" >/dev/null
dev=""

hdiutil convert -quiet "$temp_dmg" -format UFBI -o "$temp_raw"
mv "${temp_raw}.dmg" "$image_path"
current_size=$(stat -f '%z' "$image_path")
padded_size=$(( ((current_size + 511) / 512) * 512 ))
if [[ "$padded_size" -ne "$current_size" ]]; then
  /usr/bin/truncate -s "$padded_size" "$image_path"
fi
rm -f "$temp_dmg"

shasum -a 256 "$image_path" > "$image_path.sha256"

echo "wrote:"
echo "  $image_path"
echo "  $image_path.sha256"
