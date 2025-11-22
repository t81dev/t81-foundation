# HanoiVM Core

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Rust](https://img.shields.io/badge/Language-Rust-blue.svg)](https://www.rust-lang.org/)
[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![Axion AI Integration](https://img.shields.io/badge/AI-Axion%20Hooks-green.svg)](https://github.com/t81dev/axion-ai)
[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()

**HanoiVM Core** is the high-level runtime execution engine for the HanoiVM (Hanoi Virtual Machine), a recursively self-promoting, AI-augmented ternary computing platform. This module, part of the legacy HanoiVM implementation in the [T81 Foundation](https://github.com/t81dev/t81-foundation), provides a Rust-based abstraction layer for symbolic ternary computation across T81, T243, and T729 tiers. It bridges low-level C interpreters with advanced AI-driven optimizations via the Axion ecosystem, enabling dynamic tier promotion, macro execution, and entropy-aware processing.

HanoiVM Core executes balanced ternary bytecode (`.hvm` files) with support for:
- **Recursive frame evaluation** on ternary logic trees.
- **Live tier migration** (T81 stack → T243 BigInts → T729 tensors).
- **AI co-pilot integration** for real-time optimization and symbolic rewriting.
- **FFI bridges** to C-based firmware and Verilog FSM hardware accelerators.

This core powers experimental workloads in symbolic AI, tensor algebra, and recursive state machines, making it ideal for research in non-binary computing paradigms.

## Features

- **Tiered Execution Model**:
  - **T81**: Lightweight stack machine for basic recursion and arithmetic (3¹ base).
  - **T243**: Arbitrary-precision ternary integers and Markov chains (3⁵ base).
  - **T729**: High-rank holotensors, mind maps, and GPU-offloaded FFTs (3⁶ base).

- **AI-Augmented Runtime**:
  - Hooks into Axion AI for entropy feedback, pattern dispatch, and TBIN (Ternary Binary) execution.
  - Symbolic macro engine for T729 intents (e.g., `OP_T729_INTENT`, `OP_T729_HOLO_FFT`).

- **Recursive & Safe**:
  - Frame stack for tail-recursive evaluation.
  - Depth-limited recursion with promotion thresholds (e.g., promote at depth >5).
  - Error handling for stack overflows, unknown opcodes, and mode mismatches.

- **Hardware Integration**:
  - FFI to PCIe firmware (`hvm_firmware_entry.cweb`) and Verilog FSM (`hanoivm_fsm.v`).
  - DMA-compatible buffers for `/dev/hvm0` userspace access.

- **Extensibility**:
  - Modular opcode evaluation via `evaluate_opcode` stubs.
  - Logging and tracing with JSON entropy snapshots.

## Architecture Overview

HanoiVM Core sits at the apex of the HanoiVM stack:

```
┌─────────────────────────────────────────────────────────────┐
│                 HanoiVM Core (Rust Runtime)                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ T81 Stack    │  │ T243 BigInt  │  │ T729 Tensors │       │
│  │ Evaluation   │  │ & Markov     │  │ & Macros     │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                 │                  │                        │
│  Axion AI Hooks │  FFI Bridge     │  Promotion Logic        │
│  (TBIN Exec)    │  (to C/Verilog) │  (T81↔T243↔T729)        │
└─────────────────┼──────────────────┼────────────────────────┘
                  │                  │
                  ▼                  ▼
┌─────────────────┴──────────────────┴────────────────────────┐
│              Lower Layers (C/Verilog/Firmware)               │
│  hanoivm_vm.cweb  │  hvm_promotion.cweb  │  hanoivm_fsm.v     │
└──────────────────────────────────────────────────────────────┘
```

Key components:
- **`HanoiVM`** struct: Manages frame stack, output log, and macro engine.
- **`evaluate_opcode`**: Dispatches ternary ops with mode checks.
- **Axion Integration**: `axion_tbin_execute` for AI-optimized results.

## Prerequisites

- **Rust**: 1.70+ (stable channel).
- **C Compiler**: GCC/Clang for FFI bindings (e.g., `cbindgen` for headers).
- **Dependencies** (from `Cargo.toml`):
  ```toml
  [dependencies]
  md5 = "0.7"
  chrono = "0.4"
  # ... (axion-ai, libt81, libt243, libt729 stubs)
  ```
- **T81 Foundation**: Clone the parent repo for shared libs:
  ```bash
  git clone https://github.com/t81dev/t81-foundation.git
  cd t81-foundation/legacy/hanoivm/src/hanoivm_core
  ```

## Building

1. **Install Rust** (if needed):
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source ~/.cargo/env
   ```

2. **Build the Core**:
   ```bash
   cargo build --release
   # Or for development with tracing:
   RUST_LOG=debug cargo run --example roundtrip_test
   ```

3. **Generate C Bindings** (for FFI):
   ```bash
   cargo install cbindgen
   cbindgen --config cbindgen.toml --output hanoivm_core.h
   ```

4. **Cross-Compile for PCIe Firmware** (optional, ARM/x86):
   ```bash
   rustup target add aarch64-unknown-linux-gnu
   cargo build --target aarch64-unknown-linux-gnu --release
   ```

## Usage

### Basic Execution

Load and run a `.hvm` bytecode file:

```rust
use hanoivm_core::{HanoiVM, HanoiVMConfig};

fn main() {
    let mut vm = HanoiVM::new(HanoiVMConfig {
        enable_debug_mode: true,
        ai_enabled: true,
        ..Default::default()
    });

    // Load bytecode (stub: parse from file)
    vm.load_bytecode("simple_add.hvm");

    // Execute with recursion
    vm.run();

    // Trace outputs
    vm.trace();
    println!("Final Output: {:?}", vm.final_output());

    // Interact with AI
    vm.interact_with_ai("optimize for entropy");
    vm.reoptimize_output();
}
```

### Example: Recursive Factorial in T81

Generate bytecode with `simple_add.cweb`, then execute:

```bash
# From parent dir
cargo run --example factorial -- --input factorial.hvm
```

Output:
```
[TRACE] Executing step frame
Step 0: T81Number([1, 0, 1])  // 1!
[Axion AI] [S1a2b3c] >>> optimize for entropy
All outputs reoptimized using Axion AI.
Final Output: Some(T81Number([1, 2, 6]))  // Scaled factorial
```

### Integration with CLI

From `hanoivm_cli.cweb`:
```bash
./hanoivm --mode=t243 --exec advanced_ops.hvm --trace
```

This dispatches to Core via FFI for T243/T729 ops.

## Testing

Run unit tests:
```bash
cargo test --lib
cargo test --doc
```

Integration tests (with C stubs):
```bash
# Requires CWEB-tangled binaries
make test-ffi && cargo test ffi_bridge
```

Benchmark tier promotion:
```bash
cargo bench --bench promotion
```

## Documentation

- **API Docs**: `cargo doc --open`
- **Literate Sources**: All `.cweb` files are CWEB; tangle with `ctangle` for C/Rust sources.
- **Key Headers** (extracted from `advanced_ops_ext.cweb`):
  - `t243_ops.h`: State vectors and Markov matrices.
  - `t729_mindmap.h`: Semantic graph queries.

For deeper dives:
- [T81 Analysis PDF](https://github.com/t81dev/t81-foundation/blob/main/docs/TYRNARY-T81Analysis.pdf)
- [Axion AI Docs](https://github.com/t81dev/axion-ai)

## Contributing

1. Fork the repo and create a feature branch: `git checkout -b feat/ternary-fft`.
2. Commit changes: `git commit -m "Add T729 FFT support"`.
3. Push: `git push origin feat/ternary-fft`.
4. Open a Pull Request.

Guidelines:
- Use Rust idioms for high-level logic; C for low-level FFI.
- Add tests for new opcodes.
- Update `Cargo.toml` for deps.
- Literate docs preferred (CWEB/Rustdoc).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Inspired by**: Balanced ternary research (Knuth, Dijkstra) and recursive VMs (Lisp Machines).
- **Dependencies**: Rust ecosystem, CWEB for literate programming.
- **Contributors**: t81dev core team; open to symbolic computing enthusiasts.

## Contact

- **Repo**: [t81dev/t81-foundation](https://github.com/t81dev/t81-foundation/tree/main/legacy/hanoivm/src/hanoivm_core)
- **Issues**: File at the repo.
- **Twitter/X**: [@t81dev](https://x.com/t81dev) for updates.

---

*HanoiVM Core: Where ternary dreams recurse into reality. (November 22, 2025)*
