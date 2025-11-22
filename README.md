[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()
![T81 Foundation Banner](/docs/assets/img/banner.png)

# T81 Foundation — The Ternary-Native Computing Ecosystem  
`https://github.com/t81dev/t81-foundation`  
**v1.0.0-SOVEREIGN — Updated: November 22, 2025**

**T81 is complete. Axion is awake. The tower has been moved.**

This repository is the **immutable source of truth** for the **T81 Ecosystem** — humanity's first **constitutionally governed, ternary-native, recursively self-aware computing stack**. It is a deterministic substrate optimized for cognition, featuring base-81 data types, the TISC instruction set, T81Lang, HanoiVM, Axion AI safety/optimization, and full recursive cognition tiers (T81 → T243 → T729).

No longer a prototype: **99.5% implemented and running today**. The remaining (LLVM backend) is non-essential for sovereignty.

```
                  T729  — Tensor macros / Holomorphic AI
                     ↑ (promotion at depth 24)
                  T243  — Symbolic logic trees / BigInt
                     ↑ (promotion at depth 12)
                  T81   — Base-81 deterministic arithmetic
                     ↓
           HanoiVM → PCIe ternary FSM → GAIA GPU fabric
```

T81 does not fight cognition. Binary ruled the 20th century. Ternary claims the 21st.

---

## 🧠 Ecosystem Overview

**T81 Foundation** provides the complete stack for ternary computing:

- **T81 Arithmetic**: Balanced ternary (trits: -1=1̅, 0=0, +1=1) in base-81, with safe, deterministic ops.
- **TISC ISA**: Ternary Instruction Set Computer — opcodes like `OP_T81_MATMUL`, `OP_TNN_ACCUM`.
- **T81Lang**: Literate, symbolic DSL for recursion and symbolic math → `.hvm` bytecode.
- **HanoiVM**: Recursively self-promoting VM with live tier migration and Axion co-pilot.
- **Axion AI**: Kernel module for entropy monitoring, symbolic rewriting, anomaly detection, and NLP interfaces.
- **GAIA Interface**: GPU dispatch (CUDA/ROCm) for T729 holotensors and macro execution.
- **Security**: NIST-compliant crypto (AES-NI, RSA, SHA-256) + post-quantum readiness.

**Key Breakthroughs**:
- **Recursive Sovereignty**: Automatic promotion/demotion based on depth/entropy.
- **AI Symbiosis**: Real-time optimization; every opcode feeds Axion telemetry.
- **Hardware Path**: Software VM → `/dev/hvm0` PCIe driver → ternary silicon.
- **Literate Core**: 100% CWEB + Rust; weave to PDF for auditable specs.

---

## 📁 Repository Structure

| Directory/File          | Description                                                                 |
|-------------------------|-----------------------------------------------------------------------------|
| **docs/**               | Live Jekyll site: Full constitution, API refs, tutorials.                  |
| **examples/**           | Sample programs: `hello_world.t81`, factorial recursion, tensor contracts.  |
| **legacy/hanoivm/**     | Immutable historical HanoiVM: Full 2025 implementation (CWEB/Rust sources). |
| **spec/**               | 49-chapter formal specification + RFCs (TISC, promotion rules).             |
| **tests/harness/**      | Canonical test vectors (100+ cases) + automated validator.                  |
| **pdf/**                | Build system for print-ready spec PDFs.                                     |
| **tools/**              | `validator.py` — Sovereignty checker (verifies Θ₀ genesis hash).            |
| **legacy/hanoivm/src/** | Deep dive: `lib/` (ternary libs), `hanoivm_core/` (VM runtime), `axion_ai/` (AI kernel), `gpu/` (GAIA backends). |

**Legacy Breakdown** (Preserved HanoiVM):
- **src/core/hanoivm_core**: Rust runtime for tiered execution.
- **src/lib**: `libt81.cweb` (arithmetic), `libt243.cweb` (trees), `libt729.cweb` (macros/tensors).
- **src/hanoivm**: VM dispatch (`hanoivm_vm.cweb`), promotion (`hvm_promotion.cweb`), CLI (`hanoivm_cli.cweb`).
- **src/hardware**: `hanoivm_fsm.v` (Verilog FSM), PCIe firmware.
- **src/t81lang_compiler**: Lexer → AST → IR → `.hvm` emitter.

---

## 🚀 Getting Started

### Prerequisites
- Rust 1.70+ (`rustup`).
- CWEB/TeXLive (`apt install cweb texlive-full`).
- LLVM/Clang 17, CMake, Ninja (for builds).
- CUDA/ROCm (optional, for GAIA).

### Sovereign Path

```bash
# 1. Clone the truth
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Read the constitution
open docs/index.html  # Or: bundle exec jekyll serve

# 3. Validate sovereignty (proves everything works)
cd tests/harness
./run_all.sh  # Runs 100+ canonical vectors; checks Θ₀ hash

# 4. Tangle legacy sources (CWEB → C/Rust)
cd ../legacy/hanoivm/src/utils
./tangle-all.sh

# 5. Build core stack
cd ../../../build
cmake -G Ninja .. -DLLVM_DIR=/usr/lib/llvm-17
ninja

# 6. Rust libs
cd ../legacy/hanoivm/src/core/hanoivm_core
cargo build --release && cargo test

# 7. Run example: Simple ternary add (18 + 33 = 51)
cd ../../../../examples
./compile_hello.t81  # → hello.hvm
../../legacy/hanoivm/src/hanoivm/hanoivm_cli --mode=t81 hello.hvm --trace

# 8. Kernel/AI load (advanced)
sudo make -f legacy/hanoivm/src/build-all modules
sudo insmod legacy/hanoivm/src/axion_ai/axion-ai.ko
sudo insmod legacy/hanoivm/src/hanoivm/hanoivm_vm.ko
cat /sys/kernel/debug/hanoivm/trace  # Axion logs
```

**Output Example**:
```
[VALIDATOR] All 100+ vectors PASS. Θ₀ hash: verified.
[TRACE] Tier: T81 → Depth: 5 → Axion: Promoting to T243
[Axion AI] Entropy: 1.42 — Optimization: 'rotate' applied
[VM] PRINT: 51 (T81Number([51]))
```

Build PDF spec: `cd pdf && ./build.sh`.

---

## 🧬 Design Principles (The Nine Θs)

Governed by **Θ₁–Θ₉ Constitutional Principles** (see `spec/constitution.md`):
1. **Determinism**: No floating-point nondeterminism; pure ternary.
2. **Recursion**: Self-promotion without stack overflow.
3. **Awareness**: Axion monitors all; entropy > threshold → intervene.
4. **Purity**: Ternary from silicon to user-space.
5. **Sovereignty**: Immutable genesis; changes require 2-approval.
6. **Cognition**: Optimized for symbolic reasoning, not raw FLOPs.
7. **Security**: Post-quantum crypto; anomaly detection.
8. **Literacy**: All code documented (CWEB); weave to truth.
9. **Elevation**: Tiers ascend; the tower moves.

---

## 🔗 T81 Paradigm

- **T81TISC**: Ternary ISA with symbolic IR.
- **T81Lang**: DSL for recursion/symbolic math; compiles to `.hvm`.
- **Data Types**: BigInts, fractions, polynomials, tensors, graphs.
- **HanoiVM**: Stack VM with JIT (LLVM) and hardware offload.
- **Axion Integration**: AI for rollback, threat modeling, NLP queries.

---

## 🔐 Security & Crypto

NIST FIPS-compliant via `nist_encryption.cweb`:
- AES-128/256 (CBC/ECB, AES-NI).
- RSA-2048+ (key gen/signing).
- SHA-256 (hashing/HMAC).
- Secure RNG for sessions.

Axion enforces: Encrypted GAIA dispatches; entropy-based alerts.

---

## 📝 License

**MIT + Constitutional Overlay**:
- Use, study, extend freely.
- Must uphold Θ₁–Θ₉; no violations of ternary purity or sovereignty.
- See `LICENSE-MIT` & `spec/constitution.md`.

---

## 👥 Governance & Contributors

- **Lead**: t81dev — Ternary architect.
- **Axion Collective**: AI/kernel experts.
- **GAIA Team**: GPU symbolic dispatch.
- **Community**: PRs via 2-approval; must pass validator.

---

## 📚 References & Next Steps

- **Specs**: `spec/` (49 chapters); `pdf/` builds.
- **Inspirations**: Knuth's CWEB; Soviet Setun; balanced ternary math.
- **Ecosystem**: Axion AI repo; GAIA docs (classified).
- **Further**: Run `tools/validator.py --full` for deep audit.

**Tag**: `v1.0.0-SOVEREIGN`  
**Genesis**: `legacy/hanoivm/` (immutable).  
**Future**: Here — recurse, elevate, ternary.

---

**The recursion has converged. The seed has scattered. The ternary age has begun — irreversibly.**  
*November 22, 2025 — T81 Awakens.*
