# lib — HanoiVM Ternary Foundation Libraries  
`https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src/lib`

[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()

The **lib** directory contains the complete, vertically integrated **ternary arithmetic and symbolic execution stack** that powers HanoiVM — the world’s first recursively self-promoting, AI-orchestrated ternary virtual machine.

These libraries implement **T81 → T243 → T729** progression in pure, literate C + Rust, with direct hooks into the Axion AI runtime, GAIA GPU fabric, and PCIe ternary accelerator.

### Core Philosophy
> “Everything is a tensor. Every tensor is a tree. Every tree is a number. Every number is a trit.”  
> — HanoiVM Manifesto, 2025

## Library Overview

| Library                        | Base   | Purpose                                                     | Key Files                                 |
|--------------------------------|--------|-------------------------------------------------------------|-------------------------------------------|
| **libt81**                     | 3¹ = 81| Base-81 balanced ternary arithmetic (Rust core)             | `libt81.cweb`                             |
| **libt243**                    | 3⁵ = 243| Symbolic logic trees, Markov chains, state machines         | `libt243.cweb`, `t243bigint.cweb`         |
| **libt729**                    | 3⁶ = 729| High-rank holotensors, macro engine, JIT dispatch           | `libt729.cweb`, `t729tensor*.cweb`        |
| **t81_types_support**          | —      | Extended opcodes: fractions, floats, polynomials, graphs    | `t81_types_support.cweb`                  |
| **t729tensor** suite           | 729    | Full tensor algebra: transpose, slice, reshape, contract    | `t729tensor_*.cweb`                       |
| **t243_to_t729**               | Bridge | Tree → Macro transformer with entropy scoring & GAIA dispatch | `t243_to_t729.cweb`                     |
| **hvm-trit-util**              | Core   | Safe ternary math, BigInt, parsing, mmap utilities         | `hvm-trit-util.cweb`                      |
| **nist_encryption**            | Crypto | AES-NI + RSA + SHA for secure Axion session transport      | `nist_encryption.cweb`                    |

## Tiered Execution Model

```
T81  (Stack Machine)        → libt81 + t81_types_support
   ↓ recursion depth > 10
T243 (Symbolic Trees/BigInt) → libt243 + t243bigint
   ↓ depth > 20 or entropy spike
T729 (Holotensors & Macros)  → libt729 + t729tensor suite
   ↓ dispatched to GAIA/Axion GPU cluster
```

All layers share a unified `TernaryHandle` opaque pointer system (`ternary_base.h`) allowing **zero-copy promotion/demotion**.

## Key Features

- **Balanced Ternary Throughout** – No binary contamination
- **Literate Programming** – Every file is a `.cweb` — fully documented, beautiful, auditable
- **Live Tier Migration** – Automatic promotion/demotion via `hvm_promotion.cweb`
- **AI-Native** – Axion hooks on every opcode, entropy snapshots, macro reoptimization
- **Hardware Ready** – Direct FFI to `/dev/hvm0` PCIe ternary accelerator
- **Full Tensor Suite** – Reshape, transpose, slice, contract, print, clone
- **Cryptographic Security** – NIST-approved AES-NI + RSA for session keys

## Building

These libraries are tangled from CWEB source:

```bash
# From repo root
cd legacy/hanoivm/src/lib

# Tangling (generate .c/.h/.rs)
ctangle libt81.cweb
ctangle libt243.cweb
ctangle libt729.cweb
ctangle t729tensor.cweb
ctangle t729tensor_transpose.cweb
# ... etc for all .cweb files

# Build shared lib (example)
gcc -shared -fPIC -o libhanoivm.so \
    *.c -lssl -lcrypto -lm

# Or build Rust components
cargo build --release -p libt81 -p libt243 -p libt729
```

## Usage Example (C)

```c
#include "t729tensor.h"
#include "t243_to_t729.h"

int main() {
    // Create a 2×3 test tensor
    TernaryHandle t = make_test_tensor_2x3();

    // Transpose it
    TernaryHandle transposed;
    t729tensor_transpose(t, &transposed);

    // Print result
    char* str = NULL;
    t729tensor_to_string(transposed, &str);
    printf("Transposed:\n%s\n", str);
    free(str);

    // Promote tree → T729 macro → dispatch to GAIA
    T243TreeNode* tree = build_sample_tree();
    T729Macro macro = generate_macro_from_tree(tree, 42, &ctx);
    dispatch_macro_to_gpu(&macro);

    return 0;
}
```

## Documentation

All code is **literate** — run `cweave *.cweb && texi2pdf *.tex` to generate beautiful PDFs.

Or read online (coming soon):
- `T81 Arithmetic Manual`
- `T243 Symbolic Logic Trees`
- `T729 Holotensor Algebra`
- `Axion-GAIA Interface Specification`

## Testing

```bash
# Run built-in test harnesses
gcc -DTEST_T243BIGINT t243bigint.cweb.c -o test_t243 && ./test_t243

# Rust tests
cargo test --lib --all
```

## Contributing

We welcome contributions to the ternary future.

1. Fork the repo
2. Create branch: `feat/t729-broadcast` or `opt/t243-pruning`
3. Write **literate** `.cweb` or Rust with full documentation
4. Submit PR with entropy impact analysis

## License

Dual-licensed:
- **MIT** – For commercial and academic use
- **GPLv3** – For those who believe in copyleft ternary liberation

## Links

- Main Repo: https://github.com/t81dev/t81-foundation
- HanoiVM Core (Rust): `/legacy/hanoivm/src/hanoivm_core`
- Axion AI: https://github.com/t81dev/axion-ai
- GAIA GPU Fabric: (classified)

---

**This is not just a library. This is the foundation of a new computational substrate.**

Welcome to the ternary recursion.

**HanoiVM lib — Where every number dreams in three states.**  
November 22, 2025
