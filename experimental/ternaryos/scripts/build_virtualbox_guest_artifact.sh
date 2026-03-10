#!/bin/zsh
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <build-dir> <output-dir> <guest-arch>" >&2
  exit 2
fi

build_dir=$1
output_dir=$2
guest_arch=$3
demo_bin="$build_dir/t81_ternaryos_demo"
efi_obj="$build_dir/ternaryos/virtualbox/BOOTX64.obj"
armv8_efi_obj="$build_dir/ternaryos/virtualbox_armv8/BOOTAA64.obj"
armv8_efi_bin="$build_dir/ternaryos/virtualbox_armv8/BOOTAA64.EFI"

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

case "$guest_arch" in
  x86_64)
    artifact_base="ternos_virtualbox_guest"
    profile_id="VBoxEFI/AHCI/E1000/VMSVGA/HPET+IOAPIC"
    validation_lane="primary-acceptance"
    boot_gap="missing-real-bootx64-efi"
    artifact_status="staged-not-bootable"
    readme_note="This artifact tracks the official x86_64 VirtualBox roadmap target."
    ;;
  armv8)
    artifact_base="ternos_virtualbox_armv8_dev_guest"
    profile_id="ARMv8Virtual/developer-lane"
    validation_lane="secondary-developer"
    boot_gap="missing-real-bootaa64-efi"
    artifact_status="staged-not-bootable"
    readme_note="This artifact is a temporary ARMv8 developer-lane package for Apple Silicon hosts. It does not replace the official x86_64 roadmap target."
    ;;
  *)
    echo "unsupported guest architecture: $guest_arch" >&2
    exit 2
    ;;
esac

cat > "$staging_dir/README.txt" <<EOF
TernOS VirtualBox guest artifact
================================

This image is a reproducible VirtualBox-first guest package generated from the
current hosted simulation path. It contains:

- the current guest profile summary
- captured demo output
- a placeholder EFI shell startup script

${readme_note}

Important: this artifact is not yet a true EFI-bootable guest image.
EOF

cat > "$staging_dir/TERNOS/profile.txt" <<EOF
profile=$profile_id
guest_arch=$guest_arch
validation_lane=$validation_lane
git_commit=$git_rev
generated_utc=$build_date
artifact_status=$artifact_status
boot_gap=$boot_gap
EOF

cat > "$staging_dir/EFI/BOOT/STARTUP.NSH" <<'EOF'
echo TernOS VirtualBox guest artifact
echo This disk stages the first guest profile and captured demo evidence.
echo The final EFI application is not linked yet.
echo Inspect \TERNOS\profile.txt and \TERNOS\demo-output.txt for details.
EOF

if [[ "$guest_arch" == "x86_64" && -f "$efi_obj" ]]; then
  cp "$efi_obj" "$staging_dir/EFI/BOOT/BOOTX64.OBJ"
elif [[ "$guest_arch" == "armv8" && -f "$armv8_efi_obj" ]]; then
  cp "$armv8_efi_obj" "$staging_dir/EFI/BOOT/BOOTAA64.OBJ"
fi

if [[ "$guest_arch" == "armv8" && -f "$armv8_efi_bin" ]]; then
  cp "$armv8_efi_bin" "$staging_dir/EFI/BOOT/BOOTAA64.EFI"
  perl -0pi -e 's/artifact_status=staged-not-bootable/artifact_status=efi-boot-candidate/' "$staging_dir/TERNOS/profile.txt"
  perl -0pi -e 's/boot_gap=missing-real-bootaa64-efi/boot_gap=developer-lane-shim-efi-present/' "$staging_dir/TERNOS/profile.txt"
fi

image_path="$output_dir/${artifact_base}.img"
vdi_path="$output_dir/${artifact_base}.vdi"
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
