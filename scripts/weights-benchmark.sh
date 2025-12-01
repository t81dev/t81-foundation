#/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 ]]; then
  cat <<'EOF'
Usage: scripts/weights-benchmark.sh <safetensors> <q4_model.gguf> <llama-cli> <prompt> [--tokens N]

Runs t81 weights quantize on the SafeTensors file, then benchmarks llama.cpp tokens/sec
for the resulting T3_K GGUF and the provided Q4_K_M model.
Outputs both throughput numbers plus the llama.cpp memory bandwidth line (if present).
EOF
  exit 1
fi

SAFE="$1"
Q4="$2"
LLAMA="$3"
PROMPT="$4"
shift 4
TOKENS=512
while [[ $# -gt 0 ]]; do
  case "$1" in
    --tokens)
      shift
      TOKENS="$1"
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
  shift
done

QUANT_OUT="$(mktemp --tmpdir t81-weights-XXXXXX.gguf)"
trap 'rm -f "$QUANT_OUT"' EXIT

echo "Quantizing $SAFE → $QUANT_OUT ..."
./build/t81 weights quantize "$SAFE" --to-gguf "$QUANT_OUT"

run_llama() {
  local model="$1"
  local label="$2"
  echo
  echo "Running $label inference ($model)..."
  "$LLAMA" -m "$model" -p "$PROMPT" -n "$TOKENS" --color 2>&1 | \
    awk -v label="$label" '
      /tokens\/sec/ { print label ": " $0 }
      /mem txt/ { print label " " $0 }
    '
}

run_llama "$Q4" "Q4_K_M"
run_llama "$QUANT_OUT" "T3_K"
