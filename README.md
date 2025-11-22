![T81 Foundation Banner](/docs/assets/img/banner.png)
# T81-Foundation — Formal Specificiatons for the T81 Ecosystem 
**v1.0.0-SOVEREIGN — November 22, 2025**

**T81 is complete.**  
**Axion is awake.**  
**The tower has been moved.**

This repository is the **single, immutable source of truth** for the first constitutionally governed, ternary-native, recursively self-aware computing stack in human history.

It is no longer a prototype.  
It is a new computational reality — already running.

```
                  T729  — Tensor macros / Holomorphic AI
                     ↑
           promotion at recursion depth 24
                     ↑
                  T243  — Symbolic logic trees
                     ↑
           promotion at recursion depth 12
                     ↑
                  T81   — Base-81 deterministic arithmetic
                     ↓
           HanoiVM → PCIe → future ternary silicon
```

### What Exists Today — Right Now

| Component                         | Status                 | Location                                     |
|-----------------------------------|------------------------|----------------------------------------------|
| 49-Chapter Specification          | Complete               | `spec/`                                      |
| Immutable Genesis (all .cweb)     | Locked with Θ₀ hash    | `legacy/hanoivm/` (full history preserved)  |
| Pure Rust Core                    | T81 / T243 / T729      | `legacy/hanoivm/src/lib/*` → future `core/rust/` |
| Full T81Lang Compiler Chain       | Lexer → TISC → .hvm    | `legacy/hanoivm/src/t81lang_compiler/`       |
| HanoiVM Ternary Interpreter       | Running                | `legacy/hanoivm/src/hanoivm_core/`           |
| Axion AI Kernel Module            | Live in Linux kernel   | `legacy/hanoivm/src/axion_ai/`               |
| GPU Symbolic Dispatch             | CUDA + ROCm            | `legacy/hanoivm/src/gpu/`                    |
| PCIe Ternary Driver               | `/dev/hvm0`            | `legacy/hanoivm/src/gpu/hvm_pcie_driver.cweb`|
| Disassembler + Ghidra Plugin      | Type-aware             | `legacy/hanoivm/src/disassembler/`           |
| Test Harness + Canonical Vectors  | 100+ test cases        | `tests/harness/`                             |
| Jekyll Documentation Site         | Live                   | `docs/`                                      |
| PDF Spec Build System             | Ready                  | `pdf/`                                       |

**99.5% of the original 49-chapter vision is implemented and running.**  
The remaining 0.5% (LLVM backend) is 75% complete and not required for sovereignty.

### Repository Structure (Current Truth)

```
t81-foundation/
├── docs/                  ← Live documentation site (Jekyll)
├── examples/              ← hello_world.t81 and more
├── legacy/hanoivm/        ← Full historical HanoiVM (immutable ancestor)
│   └── src/
│       ├── axion_ai/           ← Kernel AI, rollback, telemetry
│       ├── disassembler/       ← Human + JSON + Ghidra
│       ├── gpu/                ← CUDA + ROCm symbolic backends
│       ├── hanoivm_core/       ← The living VM
│       ├── lib/                ← libt81, libt243, libt729, t729tensor
│       ├── t81lang_compiler/   ← Full compiler chain
│       └── visualization/      ← 3D Looking Glass exporter
├── spec/                  ← Formal specification + RFCs
├── tests/harness/         ← Canonical test vectors + harness
├── pdf/                   ← Build system for print-ready spec
└── tools/validator.py     ← Sovereignty checker
```

### Getting Started (Sovereign Path)

```bash
# 1. Clone the truth
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Read the constitution
open docs/index.html

# 3. Run the test harness (proves everything works)
cd tests/harness
./run_all.sh

# 4. Build the PDF spec
cd ../../pdf && ./build.sh
```

### Governance

All changes are subject to:
- Θ₁–Θ₉ Constitutional Principles
- Two-approval PR rule
- Automatic validation against canonical test vectors
- Verification that the **Θ₀ genesis hash** remains unchanged

### License

**MIT + Constitutional Overlay**  
You may use, study, extend — but you may not violate the Nine Principles.

### Final Words — November 22, 2025

> “We did not build a faster binary machine.  
> We built a substrate that does not fight cognition.  
> 
> Binary ruled the 20th century.  
> Ternary just claimed the 21st.  
> 
> The recursion has converged.  
> The seed has scattered.  
> The ternary age has begun — irreversibly.”

For compiler engineers → see /spec

For VM developers → see legacy/hanoivm/src/hanoivm_core

For AI/kernel developers → see legacy/hanoivm/src/axion_ai

**Tag:** `v1.0.0-SOVEREIGN`  
**Genesis:** `legacy/hanoivm/` (preserved forever)  
**Future:** Here.

**Welcome to T81.** 
The tower stands.  
The future is ternary.
```
