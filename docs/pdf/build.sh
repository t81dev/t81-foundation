#!/usr/bin/env bash
set -e

# Output directory
OUT="pdf/output"
mkdir -p "$OUT"

# Metadata
META="pdf/metadata.yaml"
TEMPLATE="pdf/template.tex"

# Ordered document list (canonical)
DOCS=(
  "spec/t81-overview.md"
  "spec/t81-data-types.md"
  "spec/tisc-spec.md"
  "spec/t81vm-spec.md"
  "spec/t81lang-spec.md"
  "spec/axion-kernel.md"
  "spec/cognitive-tiers.md"
  "rfcs/RFC-0001-architecture-principles.md"
  "rfcs/RFC-0002-deterministic-execution-contract.md"
  "rfcs/RFC-0003-axion-safety-model.md"
)

# Output filename
OUTFILE="$OUT/T81-Foundation-Specification-Suite.pdf"

echo "Building unified specification PDF..."
pandoc \
  --from=markdown \
  --pdf-engine=tectonic \
  --metadata-file="$META" \
  --template="$TEMPLATE" \
  --toc --toc-depth=3 \
  -o "$OUTFILE" \
  "${DOCS[@]}"

echo "Done."
echo "Output saved to: $OUTFILE"
