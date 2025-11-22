#!/bin/bash
# tangle-all.sh | Tangle all .cweb files in the current directory into .c or Makefiles

set -e

TANGLER=${1:-ctangle}

echo "[tangle-all] Using tangler: $TANGLER"

for file in *.cweb; do
  base="${file%.cweb}"
  echo "[tangle-all] Tangling $file -> $base.c"
  $TANGLER "$file" > "$base.c"

done

echo "[tangle-all] Tangling complete. All .cweb sources have been converted."
