#!/bin/zsh
set -euo pipefail

soft_fail=0
if [[ "${1:-}" == "--soft" ]]; then
  soft_fail=1
  shift
fi

if ! command -v VBoxManage >/dev/null 2>&1; then
  echo "VirtualBox host check: VBoxManage not found" >&2
  exit $((soft_fail ? 0 : 1))
fi

target_arch=${1:-x86_64}
props=$(VBoxManage list systemproperties)
supported=$(printf '%s\n' "$props" | awk -F': *' '/Supported platform architectures/ {print $2}')

if [[ -z "$supported" ]]; then
  echo "VirtualBox host check: unable to determine supported platform architectures" >&2
  exit $((soft_fail ? 0 : 1))
fi

echo "VirtualBox supported platform architectures: $supported"

case "$target_arch" in
  x86_64)
    if [[ "$supported" == *"x86_64"* || "$supported" == *"AMD64"* ]]; then
      echo "Host can validate the x86_64 VirtualBox guest target."
      exit 0
    fi
    echo "Host cannot validate the x86_64 VirtualBox guest target." >&2
    echo "Current host only reports: $supported" >&2
    exit $((soft_fail ? 0 : 2))
    ;;
  *)
    if [[ "$supported" == *"$target_arch"* ]]; then
      echo "Host can validate the $target_arch VirtualBox guest target."
      exit 0
    fi
    echo "Host cannot validate the $target_arch VirtualBox guest target." >&2
    echo "Current host only reports: $supported" >&2
    exit $((soft_fail ? 0 : 2))
    ;;
esac
