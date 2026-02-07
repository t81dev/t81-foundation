# T81 Foundation: Long-Horizon TODOs

This document tracks long-term strategic goals and ecosystem enhancements that extend beyond the v1.0 milestone. For immediate, actionable tasks, see [`TASKS.md`](./TASKS.md).

## Ecosystem & Tooling [P0]

- [/] **Python Bindings:** Initial `pybind11` wrappers for `T81Tensor`, `T729IntTensor`, and `HanoiVM` implemented; expansion for full cognitive kernel support ongoing.
- [ ] **IDE Support:** Develop language servers (LSP) and syntax highlighters for T81Lang (VS Code, Vim, Emacs).
- [ ] **Package Management:** Implement a canonical package manager for T81Lang libraries and modules.
- [ ] **Integrated Debugger:** Create a debugger for HanoiVM that allows stepping through TISC instructions and inspecting Axion state.
- [/] **CLI Assists:** Expand the `t81` CLI with project scaffolding [DONE], disassembly [DONE], linting, and formatting tools.

## High-Tier Cognition [P1]

- [ ] **Tier 3 Reasoning:** Implement the Tier 3 cognitive layer providing structured symbolic reasoning on the Axion substrate.
- [ ] **Tier 4 Consciousness:** Research and implement Tier 4 layers focusing on self-referential modeling and high-tier cognitive loops.
- [ ] **T81-AGI Alignment:** Formalize and implement alignment policies within Axion for high-tier cognitive agents.

## Performance Optimization [P2]

- [ ] **Numeric Bottlenecks:** Optimize multi-limb `T81BigInt` arithmetic (karatsuba, SIMD acceleration).
- [ ] **I/O Scaling:** Optimize CanonFS for high-throughput ternary data persistence and retrieval.
- [ ] **JIT Compilation:** Explore Just-In-Time compilation for HanoiVM to improve execution speed for compute-intensive workloads.
- [ ] **Large-Scale Tensors:** Implement distributed tensor operations for high-rank ternary tensors.

## Core System & Verification

- [ ] **Formal Verification:** Formally verify the core balanced ternary arithmetic primitives (BigInt, Fraction).
- [ ] **Hardware Backends:** Research and prototype physical ternary hardware backends or FPGA implementations of HanoiVM.
- [ ] **Axion Policy Language:** Develop a domain-specific language for defining Axion safety policies.
- [ ] **Advanced CI Matrix:** Expand CI to include formal proof checking and extensive fuzzing of the entire stack.

## Documentation & Community

- [ ] **Comprehensive Tutorials:** Create end-to-end tutorials for building applications on the T81 stack.
- [ ] **Researcher's Guide:** Document the mathematical foundations of the T81 cognitive layers.
- [ ] **Spec Alignment:** Ensure 100% bidirectional alignment between the implementation and the NewBook specifications.
