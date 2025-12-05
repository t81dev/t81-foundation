# T81 Foundation: Actionable Task List

**Last Updated:** December 4, 2025

This document lists the concrete, prioritized tasks for the next development cycle, aligned with the strategic priorities in `ROADMAP.md`.

______________________________________________________________________

## How to Contribute

1.  Read the updated [`ROADMAP.md`](./ROADMAP.md) to understand the high-level goals.
2.  Pick a task from the lists below, starting with **P0**.
3.  Follow the guidelines in [`CONTRIBUTING.md`](./CONTRIBUTING.md).

______________________________________________________________________

### [P0] T81Lang Compiler (Completed)

**Goal:** The C++20 compiler now matches the `t81lang-spec.md`, emits Axion-aligned match/loop metadata, and the CLI (including the REPL) produces deterministic Axion traces. This critical path is complete; the compiler introduces no open blockers toward v1.0.

- **[EPIC] Implement Semantic Analysis & Type System:**
    - **[DONE] [M] Task:** Create the foundational `SemanticAnalyzer` class that traverses the AST.
    - **[DONE] [L] Task:** Implement the core type-checking logic within the `SemanticAnalyzer`.
    - **[DONE] [M] Task:** Implement type checking for generic types, focusing on `Option[T]` and `Result[T, E]`.
    - **[DONE] [S] Task:** Add the Option/Result end-to-end regression that now accompanies the compiler pipeline.

- **[EPIC] Expand Language Feature Support:**
    - **[DONE] [L] Task:** Extend the `Parser` for `loop`/`match`.
    - **[DONE] [L] Task:** Lower match/loop expressions via guard-aware IR with Axion metadata.
    - **[DONE] [M] Task:** Persist guard metadata (variants, payloads, guard expressions) through CLI/VM/Axion traces.

- **[EPIC] Improve Developer Experience:**
    - **[DONE] [M] Task:** Implement robust error reporting with location-aware diagnostics.
    - **[DONE] [M] Task:** Ship the `t81` CLI covering `compile`, `run`, `check`, and `repl`.

With P0 closed, work shifts to the runtime-focused priorities below.

______________________________________________________________________

### [P1] HanoiVM & TISC Runtime

**Goal:** Harden the VM and integrate it with other core systems. These tasks can be worked on in parallel with P0, but are secondary.

- **[EPIC] Implement the T81VM Memory Model:**
    - **[L] Task:** Design and implement the full stack and heap memory model as defined in `spec/t81vm-spec.md`.
    - **[M] Task:** Add VM instructions for stack manipulation (push, pop, stack pointers).

- **[EPIC] Harden the VM:**
    - **[M] Task:** Improve VM fault handling. Ensure all illegal operations (e.g., division by zero, out-of-bounds memory access) result in deterministic, spec-compliant faults.
    - **[L] Task:** Add extensive "negative" tests for the VM that deliberately trigger faults and verify the correct behavior.
    - **[S] Task:** Surface match metadata hints in the Axion/trace log to confirm CLI workloads can replay guard coverage during execution.
- **[M] Task:** Document and verify the deterministic segment-trace strings (`stack frame allocated`, `tensor slot allocated`, `AxRead/AxSet guard …`) via `axion_policy_runner` and updated Axion trace guides so policy runners can replay RFC-0020/RFC-0009 expectations.

______________________________________________________________________

### [P2] Axion Kernel & Documentation

**Goal:** Transform stubs into functional components and improve documentation.

- **[EPIC] Implement the Axion Kernel:**
    - **[M] Task:** Formalize the API between the HanoiVM and the Axion Kernel.
    - **[L] Task:** Implement the first set of safety policies in the Axion Kernel (e.g., Recursion Depth Limiter, Instruction Counter).
    - **[M] Task:** Integrate the specified Axion hooks (`AXREAD`, `AXSET`, etc.) into the VM's main dispatch loop.

- **[EPIC] General Documentation & Good First Issues:**
    - **[S] Task:** Add more unit tests for existing data types (`T81Float`, `Tensor`).
    - **[S] Task:** Improve Doxygen comments on public headers in `/include/t81/`.
    - **[M] Task:** Update the `docs/tensor-guide.md` to reflect the current C++ `Tensor` API.
- **[S] Task:** Publish the `axion_policy_runner` trace output in release docs and CI artifacts to give auditors a reachable example of the required `verdict.reason` strings.
- **[M] Task:** Implement the persistent CanonFS driver with Axion hooks so trace regressions exercise the disk-backed store before policy predicates run.
- **[M] Task:** Document and implement CanonFS policy hooks that intercept `AXSET`/`AXREAD` calls, emit the canonical meta/trace strings for each write, and expose those strings to policy predicates like `(require-axion-event (reason "meta slot axion event segment=meta"))` so auditors can tie CanonFS persistence to Axion enforcement.
