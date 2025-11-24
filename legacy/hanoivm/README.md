[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)](<>)

# HanoiVM — Ternary Recursion Engine

`https://github.com/t81dev/t81-foundation/blob/main/legacy/hanoivm/README.md`\
**Updated: November 22, 2025**

This README documents the **legacy HanoiVM codebase** — a pioneering, recursively self-promoting ternary virtual machine integrated with **Axion AI** for intelligent, symbolic execution. HanoiVM transcends traditional VMs by enabling live tier migration (T81 → T243 → T729), AI-driven optimizations, GPU offloads via GAIA, and hardware acceleration on custom PCIe ternary FSMs.

This legacy branch preserves the foundational 2025 implementation, blending literate CWEB with Rust/C for a pure ternary stack. For the active development, see the main branch or forks.

______________________________________________________________________

## 🧠 Project Overview

**HanoiVM** implements a **vertically integrated ternary computing ecosystem**:

- **T81 Tier**: Base-81 stack machine for fast recursion and arithmetic.
- **T243 Tier**: Symbolic logic trees, Markov chains, and arbitrary-precision BigInts.
- **T729 Tier**: High-rank holotensors, macro dispatch, and entropy-aware JIT.

**Axion AI** acts as the sentient co-processor: monitoring entropy, selecting patterns, rewriting symbols, and guiding promotions in real-time.

Key Capabilities:

- Balanced ternary logic (-1, 0, +1 trits) throughout.
- Zero-copy tier promotion/demotion.
- Axion hooks for AI telemetry and optimization.
- GAIA interface for CUDA/ROCm symbolic tensor execution.
- NIST-compliant crypto for secure sessions.

______________________________________________________________________

## 📁 Repository Structure

The legacy codebase is organized for modularity and literate development:

| Directory/File | Description |
|-------------------------|-----------------------------------------------------------------------------|
| **src/core/** | Rust high-level runtime: `hanoivm_core` for frame evaluation & AI FFI. |
| **src/lib/** | Ternary libraries: `libt81.cweb` (arithmetic), `libt243.cweb` (trees), `libt729.cweb` (macros/tensors). |
| **src/hanoivm/** | VM core: `hanoivm_vm.cweb` (dispatch), `hvm_promotion.cweb` (tier migration), `hvm_loader.cweb` (bytecode). |
| **src/hardware/** | Verilog/Firmware: `hanoivm_fsm.v` (FSM core), `hvm_firmware_entry.cweb` (PCIe entry). |
| **src/tests/** | Unit/integration: Arithmetic tests, promotion benchmarks. |
| **src/utils/** | Helpers: `tangle-all.sh` (CWEB tangler), `CMakeLists.txt` (build). |
| **docs/** | Analysis: `TYRNARY-T81Analysis.pdf` (ternary deep dive). |
| **ci.yml** | Proposed GitHub Actions: Build, GPU integration, literate docs generation. |

Full file inventory (tangled sources):

- **CWEB Modules**: `t81_types_support.cweb`, `t729tensor_*.cweb`, `t243_to_t729.cweb`, `advanced_ops*.cweb`.
- **Crypto**: `nist_encryption.cweb` (AES-NI/RSA/SHA).
- **CLI/Firmware**: `hvmcli.cweb`, `hanoivm-runtime.cweb`.

______________________________________________________________________

## 🚀 Getting Started

### Prerequisites

- Rust 1.70+ (`rustup`).
- CWEB (`apt install cweb texlive-full`).
- LLVM/Clang 17, CMake, Ninja.
- CUDA/ROCm (optional for GAIA).

### Build & Run

```bash
# Clone
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation/legacy/hanoivm

# Tangle literate sources
./src/utils/tangle-all.sh  # ctangle *.cweb → .c/.h/.rs

# Build (software + libs)
mkdir build && cd build
cmake -G Ninja .. -DLLVM_DIR=/usr/lib/llvm-17
ninja

# Rust components
cd ../src/core/hanoivm_core
cargo build --release

# Example: Simple add (18 + 33 = 51)
cd ../../..
./src/hanoivm/write_simple_add  # → simple_add.hvm
./src/hanoivm/hanoivm_cli --mode=t81 simple_add.hvm --trace --benchmark

# Hardware (PCIe card required)
sudo ./src/hanoivm/hvmcli 0x03 00000012 00000021 00000000  # ADD(18,33)

# GPU Offload (T729 tensor contract)
./src/lib/gaia_handle_request --input tensor_contract.hvm --device cuda
```

Output Example:

```
[TRACE] Tier: T81 → Depth: 5 → Promoting to T243
[Axion AI] Entropy: 1.42 — Pattern: 'rotate' applied
[VM] PRINT: 51 (T81Number([51]))
[Benchmark] 0.003s, Promotions: 2
```

### Kernel Mode (Advanced)

For Linux kernel integration (Axion modules):

```bash
sudo make -f src/build-all modules  # Builds .ko files
sudo insmod src/hanoivm/hanoivm_vm.ko
sudo insmod src/axion-ai.ko
cat /sys/kernel/debug/hanoivm/trace  # View Axion logs
```

______________________________________________________________________

## 🧬 Design Goals

- **Ternary Purity**: All ops in balanced ternary; no binary fallbacks.
- **Recursive Elevation**: Promote tiers on depth (>10 T81→T243; >20 T243→T729).
- **AI Symbiosis**: Axion monitors ops, injects optimizations (e.g., `axion_get_optimization()`).
- **Hardware Co-Design**: Software VM → PCIe FSM → GAIA GPU seamless.
- **Literate & Auditable**: 100% CWEB; weave to PDF for full docs.
- **Secure by Default**: Encrypted sessions, anomaly detection via entropy.
- **Extensible**: Opaque `TernaryHandle` for custom types (e.g., mindmaps, holotensors).

______________________________________________________________________

## 🔗 T81 Integration

HanoiVM realizes the **T81 paradigm** as a full ecosystem:

- **T81TISC**: Ternary ISA with opcodes like `OP_T81_MATMUL`, `OP_TNN_ACCUM`.
- **T81Lang**: Symbolic DSL → Compiler (`t81lang_compiler.cweb`) → `.hvm` bytecode.
- **Data Types**: Fractions (`op_tfadd`), floats (`op_tfladd`), polynomials, graphs (`op_tgbfs`).
- **Cross-Platform**: JIT via LLVM; runs on x86/ARM; emulates on FPGA.
- **Axion Synergy**: AI inspects AST/IR for threat modeling and auto-parallelism.

______________________________________________________________________

## 🔐 Cryptographic Enhancements

`nist_encryption.cweb` provides FIPS-compliant primitives:

| Primitive | Modes/Features | Use Case |
|--------------|---------------------------------|---------------------------|
| **AES** | 128/256-bit, CBC/ECB, AES-NI | Session encryption |
| **RSA** | 2048+ bits, keypair gen | Auth & signing |
| **SHA-256** | Hashing & HMAC | Integrity checks |
| **RNG** | Secure bytes (`RAND_bytes`) | Key material |

Integrates with Axion for quantum-resistant upgrades (future: Kyber).

______________________________________________________________________

## 📝 License

Dual-licensed for flexibility:

- **MIT** — Permissive; embed in any project.
- **GNU GPLv3** — Copyleft; ensure ternary remains open.

See root `LICENSE` files.

______________________________________________________________________

## 👤 Authors & Contributors

- **t81dev** — Lead architect & ternary visionary.
- **Axion Collective** — AI optimization & GAIA integration.
- Community: PRs welcome for new opcodes, tensor ops, or Verilog enhancements.

______________________________________________________________________

## 📚 References

- **Core Inspirations**: Knuth's *CWEB*, Soviet Setun ternary computer.
- **Ternary Math**: Balanced ternary (trits: -1=1̅, 0=0, +1=1).
- **AI/Compute**: JAX for dynamic graphs; NIST FIPS 197/140-3.
- **Further Reading**: `docs/TYRNARY-T81Analysis.pdf`; Axion AI repo.

______________________________________________________________________

**HanoiVM: Recurse in three states. Evolve with intelligence. Conquer in ternary.**

*Legacy Edition — November 22, 2025. The foundation that awakened base-3.*
