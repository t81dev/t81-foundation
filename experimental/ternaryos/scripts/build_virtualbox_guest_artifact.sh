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
armv8_efi_ctrl_bin="$build_dir/ternaryos/virtualbox_armv8/BOOTAA64_CTRL.EFI"

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

cat > "$staging_dir/STARTUP.NSH" <<'EOF'
echo TernOS VirtualBox guest artifact > fs0:\TERNOS\startup-ran.txt
echo shell-started >> fs0:\TERNOS\startup-ran.txt
echo Inspect \TERNOS\profile.txt and \TERNOS\demo-output.txt for details. >> fs0:\TERNOS\startup-ran.txt
fs0:
\EFI\BOOT\BOOTAA64_CTRL.EFI
\EFI\BOOT\BOOTAA64.EFI
\EFI\BOOT\bootaa64.efi
BOOTAA64_CTRL.EFI
BOOTAA64.EFI
bootaa64.efi
EOF

cp "$staging_dir/STARTUP.NSH" "$staging_dir/EFI/BOOT/STARTUP.NSH"

if [[ "$guest_arch" == "x86_64" && -f "$efi_obj" ]]; then
  cp "$efi_obj" "$staging_dir/EFI/BOOT/BOOTX64.OBJ"
elif [[ "$guest_arch" == "armv8" && -f "$armv8_efi_obj" ]]; then
  cp "$armv8_efi_obj" "$staging_dir/EFI/BOOT/BOOTAA64.OBJ"
fi

if [[ "$guest_arch" == "armv8" && -f "$armv8_efi_bin" ]]; then
  cp "$armv8_efi_bin" "$staging_dir/EFI/BOOT/BOOTAA64.EFI"
  cp "$armv8_efi_bin" "$staging_dir/EFI/BOOT/bootaa64.efi"
  perl -0pi -e 's/artifact_status=staged-not-bootable/artifact_status=efi-boot-candidate/' "$staging_dir/TERNOS/profile.txt"
  perl -0pi -e 's/boot_gap=missing-real-bootaa64-efi/boot_gap=developer-lane-shim-efi-present/' "$staging_dir/TERNOS/profile.txt"
fi

if [[ "$guest_arch" == "armv8" && -f "$armv8_efi_ctrl_bin" ]]; then
  cp "$armv8_efi_ctrl_bin" "$staging_dir/EFI/BOOT/BOOTAA64_CTRL.EFI"
  perl -0pi -e 's/boot_gap=developer-lane-shim-efi-present/boot_gap=developer-lane-control-and-shim-efi-present/' "$staging_dir/TERNOS/profile.txt"
fi

image_path="$output_dir/${artifact_base}.img"
vdi_path="$output_dir/${artifact_base}.vdi"
rm -f "$image_path" "$vdi_path" "$image_path.sha256" "$vdi_path.sha256"
dev=""
cleanup() {
  if [[ -n "${dev:-}" ]]; then
    hdiutil detach "$dev" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if [[ "$guest_arch" == "armv8" ]]; then
  temp_dmg="$output_dir/${artifact_base}.build.dmg"
  temp_raw="$output_dir/${artifact_base}.build.raw"
  rm -f "$temp_dmg" "$temp_raw" "$temp_raw.dmg"

  hdiutil create -quiet -size 64m -layout GPTSPUD -partitionType EFI "$temp_dmg"
  attach_output=$(hdiutil attach -nomount "$temp_dmg")
  dev=$(printf '%s\n' "$attach_output" | awk '/GUID_partition_scheme/{print $1; exit}')
  mount_dev=$(printf '%s\n' "$attach_output" | awk '/EFI/{print $1; exit}')
  if [[ -z "$dev" || -z "$mount_dev" ]]; then
    echo "failed to resolve ARMv8 EFI image devices" >&2
    exit 1
  fi
  /sbin/newfs_msdos -F 32 -v TERNOSGUEST "$mount_dev" >/dev/null
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
else
  dd if=/dev/zero of="$image_path" bs=1m count=64 status=none
  dev=$(hdiutil attach -nomount "$image_path" | awk 'NR==1{print $1}')
  rdev=${dev/\/dev\/disk/\/dev\/rdisk}
  /sbin/newfs_msdos -F 32 -v TERNOSGUEST "$rdev" >/dev/null
  mount_dev="$dev"
  diskutil mountDisk "$dev" >/dev/null
  mount_point=$(diskutil info "$mount_dev" | awk -F': *' '/Mount Point/ {print $2}')
  if [[ -z "$mount_point" || ! -d "$mount_point" ]]; then
    echo "failed to resolve mount point for $mount_dev" >&2
    exit 1
  fi
  cp -R "$staging_dir/." "$mount_point/"
  sync
  hdiutil detach "$dev" >/dev/null
  dev=""
fi

VBoxManage convertfromraw "$image_path" "$vdi_path" --format=VDI >/dev/null
shasum -a 256 "$image_path" > "$image_path.sha256"
shasum -a 256 "$vdi_path" > "$vdi_path.sha256"

echo "wrote:"
echo "  $image_path"
echo "  $vdi_path"
echo "  $demo_output"
