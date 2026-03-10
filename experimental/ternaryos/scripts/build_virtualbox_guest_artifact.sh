#!/bin/zsh
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <build-dir> <output-dir>" >&2
  exit 2
fi

build_dir=$1
output_dir=$2
demo_bin="$build_dir/t81_ternaryos_demo"

if [[ ! -x "$demo_bin" ]]; then
  echo "missing demo binary: $demo_bin" >&2
  exit 1
fi

for tool in hdiutil diskutil newfs_msdos VBoxManage dd shasum; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "required tool not found: $tool" >&2
    exit 1
  fi
done

mkdir -p "$output_dir"
staging_dir="$output_dir/staging"
rm -rf "$staging_dir"
mkdir -p "$staging_dir/EFI/BOOT" "$staging_dir/TERNOS"

demo_output="$staging_dir/TERNOS/demo-output.txt"
"$demo_bin" > "$demo_output"

git_rev=$(git -C "$(dirname "$0")/../../.." rev-parse --short HEAD 2>/dev/null || echo "unknown")
build_date=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

cat > "$staging_dir/README.txt" <<EOF
TernOS VirtualBox guest artifact
================================

This image is a reproducible VirtualBox-first guest package generated from the
current hosted simulation path. It contains:

- the current guest profile summary
- captured demo output
- a placeholder EFI shell startup script

Important: this artifact is not yet a true EFI-bootable guest image. The real
VBox EFI guest stub described in RFC-00B0 still needs to be implemented.
EOF

cat > "$staging_dir/TERNOS/profile.txt" <<EOF
profile=VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC
git_commit=$git_rev
generated_utc=$build_date
artifact_status=staged-not-bootable
boot_gap=missing-real-bootx64-efi
EOF

cat > "$staging_dir/EFI/BOOT/STARTUP.NSH" <<'EOF'
echo TernOS VirtualBox guest artifact
echo This disk stages the first guest profile and captured demo evidence.
echo The real BOOTX64.EFI guest stub has not been implemented yet.
echo Inspect \TERNOS\profile.txt and \TERNOS\demo-output.txt for details.
EOF

image_path="$output_dir/ternos_virtualbox_guest.img"
vdi_path="$output_dir/ternos_virtualbox_guest.vdi"
rm -f "$image_path" "$vdi_path" "$image_path.sha256" "$vdi_path.sha256"

dd if=/dev/zero of="$image_path" bs=1m count=64 status=none

dev=$(hdiutil attach -nomount "$image_path" | awk 'NR==1{print $1}')
rdev=${dev/\/dev\/disk/\/dev\/rdisk}
cleanup() {
  if [[ -n "${dev:-}" ]]; then
    hdiutil detach "$dev" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

/sbin/newfs_msdos -F 32 -v TERNOSGUEST "$rdev" >/dev/null
diskutil mountDisk "$dev" >/dev/null
mount_point=$(diskutil info "$dev" | awk -F': *' '/Mount Point/ {print $2}')
if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
  echo "failed to resolve mount point for $dev" >&2
  exit 1
fi

cp -R "$staging_dir/." "$mount_point/"
sync
hdiutil detach "$dev" >/dev/null
dev=""
trap - EXIT

VBoxManage convertfromraw "$image_path" "$vdi_path" --format=VDI >/dev/null
shasum -a 256 "$image_path" > "$image_path.sha256"
shasum -a 256 "$vdi_path" > "$vdi_path.sha256"

echo "wrote:"
echo "  $image_path"
echo "  $vdi_path"
echo "  $demo_output"
