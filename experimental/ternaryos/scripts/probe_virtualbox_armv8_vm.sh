#!/bin/zsh
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <artifact-dir> <artifact-vdi> [boot-wait-seconds]" >&2
  exit 2
fi

artifact_dir=$1
artifact_vdi=$2
boot_wait=${3:-5}

if ! command -v VBoxManage >/dev/null 2>&1; then
  echo "VirtualBox ARMv8 probe: VBoxManage not found" >&2
  exit 1
fi

if [[ ! -f "$artifact_vdi" ]]; then
  echo "VirtualBox ARMv8 probe: missing artifact VDI: $artifact_vdi" >&2
  exit 1
fi

probe_root="$artifact_dir/vm_probe"
vm_name="TernOS-ARMv8-Dev-Probe-$$"
log_copy="$artifact_dir/armv8_boot_probe.log"
summary_copy="$artifact_dir/armv8_boot_probe_summary.txt"

mkdir -p "$probe_root"

vm_uuid=""
cleanup() {
  if [[ -n "$vm_uuid" ]]; then
    VBoxManage controlvm "$vm_uuid" poweroff >/dev/null 2>&1 || true
    sleep 2
    VBoxManage unregistervm "$vm_uuid" --delete >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

VBoxManage createvm \
  --name "$vm_name" \
  --ostype Other_arm64 \
  --basefolder "$probe_root" \
  --register >/dev/null

vm_uuid=$(VBoxManage list vms | awk -v name="\"$vm_name\"" '$1 == name {gsub(/[{}]/, "", $2); print $2}')
if [[ -z "$vm_uuid" ]]; then
  echo "VirtualBox ARMv8 probe: failed to resolve VM UUID for $vm_name" >&2
  exit 1
fi

VBoxManage modifyvm "$vm_uuid" \
  --firmware efi \
  --boot1 disk \
  --boot2 none \
  --boot3 none \
  --boot4 none \
  --memory 512 \
  --cpus 1 \
  --graphicscontroller vmsvga \
  --audio-enabled off >/dev/null

VBoxManage storagectl "$vm_uuid" \
  --name SATA \
  --add sata \
  --controller IntelAhci >/dev/null

VBoxManage storageattach "$vm_uuid" \
  --storagectl SATA \
  --port 0 \
  --device 0 \
  --type hdd \
  --medium "$artifact_vdi" >/dev/null

VBoxManage startvm "$vm_uuid" --type headless >/dev/null
sleep "$boot_wait"

log_dir=$(VBoxManage showvminfo "$vm_uuid" --machinereadable | awk -F= '/^LogFldr=/{gsub(/"/, "", $2); print $2}')
if [[ -z "$log_dir" || ! -d "$log_dir" ]]; then
  echo "VirtualBox ARMv8 probe: failed to resolve log directory" >&2
  exit 1
fi

log_file="$log_dir/VBox.log"
if [[ ! -f "$log_file" ]]; then
  echo "VirtualBox ARMv8 probe: missing VBox.log" >&2
  exit 1
fi

cp "$log_file" "$log_copy"

firmware_seen=0
disk_seen=0
if rg -q "VBoxEFI-arm64.fd" "$log_file"; then
  firmware_seen=1
fi
if rg -q "AHCI: LUN#0: disk" "$log_file"; then
  disk_seen=1
fi

cat > "$summary_copy" <<EOF
vm_name=$vm_name
vm_uuid=$vm_uuid
artifact_vdi=$artifact_vdi
boot_wait_seconds=$boot_wait
firmware_seen=$firmware_seen
ahci_disk_seen=$disk_seen
log_copy=$log_copy
EOF

if [[ "$firmware_seen" -ne 1 || "$disk_seen" -ne 1 ]]; then
  echo "VirtualBox ARMv8 probe: firmware boot or AHCI disk visibility check failed" >&2
  cat "$summary_copy" >&2
  exit 1
fi

echo "VirtualBox ARMv8 probe succeeded."
echo "summary: $summary_copy"
echo "log: $log_copy"
