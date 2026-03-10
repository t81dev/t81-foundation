#!/bin/zsh
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <guest-arch> <output-file>" >&2
  exit 2
fi

guest_arch=$1
output_file=$2

mkdir -p "$(dirname "$output_file")"

efi_status="missing-linker"
efi_detail="No PE/COFF linker found"

if command -v lld-link >/dev/null 2>&1; then
  efi_status="ready"
  efi_detail="Found lld-link in PATH"
elif [[ -x /opt/homebrew/opt/llvm/bin/lld-link ]]; then
  efi_status="ready"
  efi_detail="Found Homebrew lld-link"
elif command -v ld.lld >/dev/null 2>&1; then
  efi_status="ready"
  efi_detail="Found ld.lld in PATH"
elif [[ -x /opt/homebrew/opt/llvm/bin/ld.lld ]]; then
  efi_status="ready"
  efi_detail="Found Homebrew ld.lld"
fi

cat > "$output_file" <<EOF
guest_arch=$guest_arch
efi_link_status=$efi_status
efi_link_detail=$efi_detail
EOF

if [[ "$efi_status" != "ready" ]]; then
  echo "EFI link toolchain check: $efi_detail for $guest_arch" >&2
  exit 0
fi

echo "EFI link toolchain check: ready for $guest_arch"
