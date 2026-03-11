#!/bin/zsh
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <build-dir> <output-dir>" >&2
  exit 2
fi

build_dir=$1
output_dir=$2
script_dir=${0:A:h}

artifact_dir="$build_dir/ternaryos/virtualbox"
bundle_dir="$output_dir/ternos_virtualbox_x86_64_handoff"
archive_path="$output_dir/ternos_virtualbox_x86_64_handoff.tar.gz"
bundle_name=${bundle_dir:t}

required_files=(
  "$artifact_dir/ternos_virtualbox_guest.img"
  "$artifact_dir/ternos_virtualbox_guest.vdi"
  "$artifact_dir/staging/TERNOS/profile.txt"
  "$artifact_dir/staging/TERNOS/demo-output.txt"
  "$script_dir/../virtualbox_x86_64_handoff.md"
)

for path in "${required_files[@]}"; do
  if [[ ! -f "$path" ]]; then
    echo "missing required handoff input: $path" >&2
    exit 1
  fi
done

for tool in /usr/bin/tar /usr/bin/shasum; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "required tool not found: $tool" >&2
    exit 1
  fi
done

/bin/mkdir -p "$output_dir"
/bin/rm -rf "$bundle_dir" "$archive_path" "$archive_path.sha256"
/bin/mkdir -p "$bundle_dir"

/bin/cp "$artifact_dir/ternos_virtualbox_guest.img" "$bundle_dir/"
/bin/cp "$artifact_dir/ternos_virtualbox_guest.vdi" "$bundle_dir/"
/bin/cp "$artifact_dir/staging/TERNOS/profile.txt" "$bundle_dir/"
/bin/cp "$artifact_dir/staging/TERNOS/demo-output.txt" "$bundle_dir/"
/bin/cp "$script_dir/../virtualbox_x86_64_handoff.md" "$bundle_dir/"

{
  print -r -- 'TernOS VirtualBox x86_64 handoff bundle'
  print -r -- '======================================'
  print -r -- ''
  print -r -- 'This bundle is intended for validation on an x86_64 VirtualBox host.'
  print -r -- ''
  print -r -- 'Included:'
  print -r -- '- ternos_virtualbox_guest.vdi'
  print -r -- '- ternos_virtualbox_guest.img'
  print -r -- '- profile.txt'
  print -r -- '- demo-output.txt'
  print -r -- '- virtualbox_x86_64_handoff.md'
  print -r -- ''
  print -r -- 'Use the Markdown runbook as the authoritative execution guide.'
} > "$bundle_dir/README.txt"

/usr/bin/tar -C "$output_dir" -czf "$archive_path" "$bundle_name"
/usr/bin/shasum -a 256 "$archive_path" > "$archive_path.sha256"

echo "wrote:"
echo "  $bundle_dir"
echo "  $archive_path"
