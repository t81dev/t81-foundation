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
  "$artifact_dir/staging/TERNOS/expected-boot-report.txt"
  "$artifact_dir/staging/TERNOS/expected-startup-status.txt"
  "$artifact_dir/staging/TERNOS/demo-output.txt"
  "$script_dir/validate_virtualbox_x86_64_handoff.sh"
  "$script_dir/validate_packaged_virtualbox_x86_64_handoff_bundle.sh"
  "$script_dir/../dev/virtualbox_x86_64_handoff_recovered_artifacts/README.txt"
  "$script_dir/../dev/virtualbox_x86_64_handoff_bundle_smoke_fixture/expected-boot-report.txt"
  "$script_dir/../docs/virtualbox_x86_64_handoff.md"
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
/bin/cp "$artifact_dir/staging/TERNOS/expected-boot-report.txt" "$bundle_dir/"
/bin/cp "$artifact_dir/staging/TERNOS/expected-startup-status.txt" "$bundle_dir/"
/bin/cp "$artifact_dir/staging/TERNOS/demo-output.txt" "$bundle_dir/"
/bin/cp "$script_dir/validate_virtualbox_x86_64_handoff.sh" "$bundle_dir/"
/bin/cp "$script_dir/validate_packaged_virtualbox_x86_64_handoff_bundle.sh" "$bundle_dir/"
/bin/cp -R "$script_dir/../dev/virtualbox_x86_64_handoff_recovered_artifacts" "$bundle_dir/recovered-artifacts"
/bin/cp -R "$script_dir/../dev/virtualbox_x86_64_handoff_bundle_smoke_fixture" "$bundle_dir/bundle-smoke"
/bin/cp "$script_dir/../docs/virtualbox_x86_64_handoff.md" "$bundle_dir/"

{
  print -r -- 'Axion VirtualBox x86_64 handoff bundle'
  print -r -- '======================================'
  print -r -- ''
  print -r -- 'This bundle is intended for validation on an x86_64 VirtualBox host.'
  print -r -- ''
  print -r -- 'Included:'
  print -r -- '- ternos_virtualbox_guest.vdi'
  print -r -- '- ternos_virtualbox_guest.img'
  print -r -- '- profile.txt'
  print -r -- '- expected-boot-report.txt'
  print -r -- '- expected-startup-status.txt'
  print -r -- '- demo-output.txt'
  print -r -- '- validate_virtualbox_x86_64_handoff.sh'
  print -r -- '- validate_packaged_virtualbox_x86_64_handoff_bundle.sh'
  print -r -- '- recovered-artifacts/'
  print -r -- '- bundle-smoke/'
  print -r -- '- virtualbox_x86_64_handoff.md'
  print -r -- ''
  print -r -- 'Use the Markdown runbook as the authoritative execution guide.'
  print -r -- 'Compare any guest-produced boot-report/startup-status artifacts'
  print -r -- 'against the expected-* contract files in this bundle, or run the'
  print -r -- 'packaged validate_virtualbox_x86_64_handoff.sh helper. For a local'
  print -r -- 'bundle smoke-check, run validate_packaged_virtualbox_x86_64_handoff_bundle.sh .'
} > "$bundle_dir/README.txt"

/usr/bin/tar -C "$output_dir" -czf "$archive_path" "$bundle_name"
/usr/bin/shasum -a 256 "$archive_path" > "$archive_path.sha256"

echo "wrote:"
echo "  $bundle_dir"
echo "  $archive_path"
