# HanoiVM Source Code  
`https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src`

[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![Rust + C](https://img.shields.io/badge/Languages-Rust%20%2B%20C-orange.svg)](https://www.rust-lang.org/)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()

The **src** directory houses the **complete source implementation** of **HanoiVM** — the pioneering recursively self-promoting, AI-augmented ternary virtual machine. This legacy codebase (as of November 22, 2025) blends literate CWEB documentation with Rust/C for a seamless stack from base T81 arithmetic to T729 tensor macros, integrated with Axion AI hooks and GAIA GPU offloads.

HanoiVM executes balanced ternary bytecode (`.hvm` files) across three tiers, enabling symbolic computation, live promotion/demotion, and hardware-accelerated recursion. This src tree powers experimental workloads in AI, tensor algebra, and non-binary paradigms.

### Core Philosophy
> “Recursion in base-3: Where stacks promote themselves, AI whispers optimizations, and tensors dream of trees.”  
> — HanoiVM Manifesto

## Directory Structure

Based on the project's literate and modular design, the `src` directory is organized as follows (hierarchical overview; tangle `.cweb` files for raw sources):

```
src/
├── core/                  # High-level Rust runtime engine
│   ├── hanoivm_core/      # HanoiVM Core: Frame evaluation, tier migration, Axion FFI
│   │   ├── Cargo.toml
│   │   ├── lib.rs
│   │   └── examples/      # roundtrip_test.rs, factorial.rs
│   └── README.md          # Core-specific docs
├── lib/                   # Ternary foundation libraries (T81/T243/T729)
│   ├── libt81.cweb        # Base-81 arithmetic (Rust core)
│   ├── libt243.cweb       # Symbolic logic trees & BigInt
│   ├── libt729.cweb       # Macro engine & JIT optimizer
│   ├── t81_types_support.cweb  # Extended opcodes (fractions, floats, graphs)
│   ├── t243bigint.cweb    # Arbitrary-precision ternary integers
│   ├── t729tensor.cweb    # Core tensor ops (new, free, size)
│   ├── t729tensor_transpose.cweb  # 2D transpose with error handling
│   ├── t729tensor_to_string.cweb  # Dynamic string serialization
│   ├── t729tensor_slice.cweb     # Subrange extraction
│   ├── t729tensor_reshape.cweb   # Shape reshaping with validation
│   ├── t243_to_t729.cweb         # Tree-to-macro transformer (GAIA dispatch)
│   ├── hvm-trit-util.cweb        # Core ternary utils (BigInt, parsing, mmap)
│   └── nist_encryption.cweb      # AES-NI/RSA/SHA crypto for Axion sessions
│   └── README.md                 # Lib-specific docs
├── hanoivm/               # VM core implementation (CWEB)
│   ├── hvm_loader.cweb           # Bytecode loading with Axion checks
│   ├── hvm_interpreter.cweb      # T81 interpreter with stack safety
│   ├── hanoivm_vm.cweb           # Execution loop (T81/T243/T729 dispatch)
│   ├── hvm_promotion.cweb        # Recursive tier promotion/demotion
│   ├── hvmcli.cweb               # Userspace CLI for /dev/hvm0
│   ├── hvm_firmware_entry.cweb   # PCIe firmware entry (bare-metal)
│   ├── hanoivm-runtime.cweb      # Runtime engine for PCIe accel
│   ├── hanoivm_cli.cweb          # Main CLI with modes & benchmarking
│   ├── advanced_ops.cweb         # Extended opcodes (matmul, accum)
│   └── advanced_ops_ext.cweb     # T243/T729 extensions (morphic tags, FFT)
├── hardware/             # Verilog & firmware
│   ├── hanoivm_fsm.v             # Ternary FSM opcode interpreter
│   └── hvm_firmware_entry.cweb   # (symlink or dupe for hardware path)
├── tests/                # Unit/integration tests
│   ├── test_t81_arithmetic.c
│   ├── test_tier_promotion.cweb
│   └── benchmark_hvmcli.sh
└── utils/                # Build helpers
    ├── tangle-all.sh     # Tangler script for CWEB
    ├── CMakeLists.txt    # CMake for LLVM/CUDA builds
    └── config.h          # Global config (debug, Axion flags)
```

## Key Components

### 1. **Core Runtime (Rust)**
- **hanoivm_core**: High-level abstraction for symbolic execution. Manages `HanoiVM` struct, opcode evaluation, and AI interactions.
  - Features: Tiered modes, recursion frames, entropy feedback.
  - Entry: `HanoiVM::new(config).run()`.

### 2. **Ternary Libraries**
- **libt81/libt243/libt729**: Progressive bases (81 → 243 → 729) with arithmetic, trees, and macros.
- **t729tensor Suite**: Full tensor algebra—transpose, slice, reshape, contract—for T729 holotensors.
- **t243_to_t729**: Bridges symbolic trees to GPU-dispatchable macros with entropy scoring.
- **hvm-trit-util**: Low-level trit math, BigInt normalization, safe allocs.
- **nist_encryption**: Secure key gen and AES/RSA for Axion session transport.

### 3. **VM Implementation (CWEB)**
- **hanoivm_vm.cweb**: Main execution loop with opcode dispatch and promotion hooks.
- **hvm_promotion.cweb**: Dynamic mode switching based on depth/entropy.
- **hvm_loader/interpreter**: Bytecode loading and T81 stack ops.
- **advanced_ops**: AI-specific ops like `T81_MATMUL` and `TNN_ACCUM`.
- **CLI Tools**: `hvmcli` for hardware interaction; `hanoivm_cli` for mode selection/benchmarking.

### 4. **Hardware Layer**
- **hanoivm_fsm.v**: Synthesizable Verilog FSM for ternary opcodes (PUSH/ADD/POP).
- **Firmware**: Bare-metal entry for PCIe accel (`/dev/hvm0`).

## Building from Source

1. **Prerequisites**:
   - Rust 1.70+ (`rustup`).
   - CWEB (`sudo apt install cweb texlive-full`).
   - LLVM/Clang 17, CMake, Ninja.
   - CUDA/ROCm for GPU (optional).

2. **Tangle Literate Sources**:
   ```bash
   cd src
   chmod +x ../utils/tangle-all.sh  # Or run manually: ctangle *.cweb
   ./utils/tangle-all.sh
   ```

3. **Build Libraries (Rust/C)**:
   ```bash
   # Rust cores
   cargo build --release --package hanoivm_core

   # C libs (via CMake)
   mkdir build && cd build
   cmake -G Ninja .. -DLLVM_DIR=/usr/lib/llvm-17
   ninja
   ```

4. **Compile Hardware**:
   ```verilog
   # Verilog FSM (use yosys/iverilog)
   yosys -p "synth_ice40 -top hanoivm_fsm -json hanoivm_fsm.json" hanoivm_fsm.v
   ```

5. **Run Tests**:
   ```bash
   cargo test --all
   cd build && ninja check-all
   ./hanoivm_cli --mode=t81 --exec simple_add.hvm --benchmark
   ```

## Usage Example

```rust
// In hanoivm_core/examples/simple_run.rs
use hanoivm_core::{HanoiVM, HanoiVMConfig};

fn main() {
    let mut vm = HanoiVM::new(HanoiVMConfig {
        mode: MODE_T81,
        ai_enabled: true,
        ..Default::default()
    });
    vm.load_bytecode("simple_add.hvm");  // 18 + 33 = 51
    vm.run();
    println!("Output: {:?}", vm.final_output());  // T81Number(51)
    vm.interact_with_ai("optimize recursion");  // Axion hook
}
```

```bash
# CLI execution
./hanoivm_cli --mode=t729 --exec tensor_contract.hvm --trace
# Output: [T729] Transposed shape [3x2], entropy: 1.58
```

## Integration with Axion/GAIA

- **AI Hooks**: Every opcode logs to `axion_log()`; entropy snapshots trigger reoptimization.
- **GPU Offload**: T729 macros dispatch via `gaia_handle_request()` to CUDA/ROCm backends.
- **Hardware Path**: `./hvmcli 0x03 00000000 00000012 00000021` → Sends ADD(18,33) to PCIe FSM.

## Documentation

- **Literate Sources**: Run `cweave *.cweb && pdflatex *.tex` for PDFs.
- **API Refs**: `cargo doc --open` (Rust); Doxygen for C.
- **Further Reading**:
  - [T81 Analysis](docs/TYRNARY-T81Analysis.pdf)
  - [Axion AI Bridge](https://github.com/t81dev/axion-ai)

## Contributing

1. Fork & branch: `git checkout -b feat/t729-broadcast`.
2. Tangle, build, test: Ensure `ninja check-all` passes.
3. Literate commits: Update `.cweb` with full docs.
4. PR: Include entropy benchmarks for new ops.

Guidelines: Maintain ternary purity; add AI hooks; no binary shortcuts.

## License

Dual-licensed:
- **MIT** – Permissive for research/commercial.
- **GPLv3** – Copyleft for open ternary liberation.

## Acknowledgments

- Inspired by Knuth's CWEB, Lisp recursion, and ternary hardware (e.g., Setun).
- Core team: t81dev contributors.
- Tools: LLVM, CUDA, Rust ecosystem.

---

**HanoiVM src: The beating heart of ternary recursion. Fork it, tangle it, promote it.**

*November 22, 2025 — When base-3 awoke.*
