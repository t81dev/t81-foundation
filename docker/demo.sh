#!/usr/bin/env sh
# docker/demo.sh — 60-second guided tour of T81 Foundation.
#
# Runs three short programs then drops into the interactive REPL.

set -e

EXAMPLES=/t81/examples

# ── Helpers ───────────────────────────────────────────────────────────────────

banner() {
  printf '\033[1;36m'
  cat <<'EOF'

  ████████╗ █████╗  ██╗
     ██╔══╝██╔══██╗███║
     ██║   ╚█████╔╝╚██║
     ██║   ██╔══██╗ ██║
     ██║   ╚█████╔╝ ██║
     ╚═╝    ╚════╝  ╚═╝   Foundation

  Deterministic ternary computing · bit-exact · governed AI

EOF
  printf '\033[0m'
}

section() {
  printf '\n\033[1;33m── %s\033[0m\n\n' "$1"
}

dim()    { printf '\033[2m%s\033[0m\n' "$1"; }
bold()   { printf '\033[1m%s\033[0m\n' "$1"; }
green()  { printf '\033[32m%s\033[0m\n' "$1"; }

show_source() {
  printf '\033[2m'
  cat "$1"
  printf '\033[0m'
}

run_example() {
  file="$1"
  printf '$ t81 code run %s\n' "$file"
  t81 code run "$file"
}

# ── Demo ──────────────────────────────────────────────────────────────────────

banner

# 1 / 3  Hello World
section "1 / 3  Hello, World"
dim "examples/hello_world.t81:"
show_source "$EXAMPLES/hello_world.t81"
run_example "$EXAMPLES/hello_world.t81"

# 2 / 3  Ternary-native types
section "2 / 3  Ternary-native types"
dim "examples/data_types.t81:"
show_source "$EXAMPLES/data_types.t81"
run_example "$EXAMPLES/data_types.t81"

# 3 / 3  Determinism guarantee
section "3 / 3  Determinism guarantee"
dim "T81's root invariant: identical inputs always produce identical outputs."
dim "Running the same program twice and comparing canonical hashes ...\n"

if t81 determinism verify-run "$EXAMPLES/tisc/hello_world.tisc" > /tmp/t81_verify.out 2>&1; then
  cat /tmp/t81_verify.out
  green "\n  ✓  Bit-exact reproducibility confirmed."
else
  # Fall back to two manual runs if verify-run is unavailable
  H1=$(t81 code run "$EXAMPLES/hello_world.t81" 2>&1 | sha256sum | cut -c1-16)
  H2=$(t81 code run "$EXAMPLES/hello_world.t81" 2>&1 | sha256sum | cut -c1-16)
  printf '  Run 1 output hash: %s\n' "$H1"
  printf '  Run 2 output hash: %s\n' "$H2"
  if [ "$H1" = "$H2" ]; then
    green "\n  ✓  Bit-exact reproducibility confirmed."
  fi
fi

# ── REPL ──────────────────────────────────────────────────────────────────────

printf '\n\033[1;36m────────────────────────────────────────────────────\033[0m\n'
bold "  Your turn — T81Lang REPL"
printf '\033[2m'
cat <<'EOF'
  Type T81Lang expressions and press Enter to run them.
  Try:
    let x: T81BigInt = 729t81;
    print(x);

  Commands:  :help  :symbols  :reset  :quit
EOF
printf '\033[0m'
printf '\033[1;36m────────────────────────────────────────────────────\033[0m\n\n'

exec t81 repl
