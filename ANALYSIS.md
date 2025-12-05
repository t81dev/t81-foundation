# Analysis: Implementation vs. Specification

**Last Updated:** December 5, 2025

This document provides a technical analysis of the C++ implementation's conformance to the formal specifications in `/spec`. It identifies where the implementation is complete, where it is partial, and where it deviates.

______________________________________________________________________

## 1. Core Numerics (`t81_core`)

- **Specification:** [`spec/t81-data-types.md`](../spec/t81-data-types.md)
- **Status:** `Partial`
- **Analysis:**
  - **`T81Int<N>`:** **Complete.** The fixed-size ternary integer implementation is robust, well-tested, and fully conforms to the spec's requirements for arithmetic, comparison, and overflow behavior.
  - **`T81Float`:** **Complete (double-backed with NaE/∞ handling).** The implementation now exposes the full arithmetic surface (`+`, `-`, `*`, `/`, `fma`) with NaE/∞ detection and side-channel-free fallbacks; conversions to/from `double` (via `from_double`/`to_double`) maintain balanced ternary semantics and enable the high-level helpers (`sin`, `cos`, `sqrt`) required by the spec's geometry/time layers.
  - **`Fraction`:** **Complete.** The rational number type correctly implements canonical reduction and all specified arithmetic operations, and it now consistently relies on the `T81BigInt` façade for numerator/denominator arithmetic.
  - **`T81BigInt`:** **Experimental / Stub.** The public API mirrors the arbitrary-precision spec, but the current backend only supports a single `int64_t` limb and throws when multi-limb state would be required. It is safe for small workloads but not for large integers yet.
  - **`Tensor`:** **Partial.** Tensors now support elementwise `+`, `-`, `*`, `/`, reshaping, and span/linear indexing, and the canonical type aliases (`Vec81`, `Mat81x81`, etc.) are defined, but broadcasting remains a placeholder and helper functions like `transpose` simply return the input rather than reshuffling data, so many spec-defined tensor transformations are still pending.

______________________________________________________________________

## 2. TISC ISA & VM (`t81_tisc`, `t81_vm`)

- **Specification:** [`spec/tisc-spec.md`](../spec/tisc-spec.md), [`spec/t81vm-spec.md`](../spec/t81vm-spec.md)
- **Status:** `Partial`
- **Analysis:**
  - **Instruction Set:** **Partial.** A large subset of the TISC opcodes are defined in `opcodes.hpp` and executed by the interpreter, but opcode families tied to the Axion kernel interface and extended memory primitives still await implementation.
  - **Binary Encoding:** **Complete.** The `BinaryEmitter` correctly encodes the implemented subset of TISC IR into the specified flat binary format, including the two-pass label resolution.
  - **VM Execution Loop:** **Partial.** The interpreter (`src/vm/vm.cpp`) now wires each instruction through `eval_axion_call`, injecting Axion policy evaluation before the dispatch, and the runtime pushes loop/match metadata into `state_.axion_log` so Axion traces can replay guards; traps for denied policies surface as `Trap::SecurityFault`.
  - **Memory Model:** **Partial.** The runtime now derives a `Layout` with dedicated code/stack/heap/tensor/meta regions and `mem_ok` guards, so the interpreter can reject out-of-bounds accesses; however the allocator is still a flat linear space with no segment protection or privilege separation beyond the simple range checks.
  - **Known Deviation:** Faults like division-by-zero or Axion-denied instructions currently map to generic `Trap` values / host exceptions rather than the spec's richer Axion fault taxonomy.

______________________________________________________________________

## 3. T81Lang Frontend (`t81_frontend`)

- **Specification:** [`spec/t81lang-spec.md`](../spec/t81lang-spec.md)
- **Status:** `Implemented`
- **Analysis:**
  - **Lexer & Parser:** The parser now covers the full grammar, including `match`, structural declarations, and generic type syntax, and reports errors with file/line/column information so the CLI can guide fixes immediately.
  - **Type System & Semantic Analysis:** `SemanticAnalyzer` enforces numeric widening, Option/Result constructors, structural generics, record/enum payloads, and match exhaustiveness. `semantic_analyzer_generic_test.cpp`, `semantic_analyzer_option_result_test.cpp`, and `cli_option_result_test.cpp` keep these rules regression-safe, and `cli_check_test.cpp` proves that `t81::cli::check_syntax` reuses the same lex/parse/semantic pipeline as `t81 compile`, surfacing the same diagnostics before IR emission.
  - **IR Generation:** The `IRGenerator` continues emitting TISC instructions for the verified AST; the compiler now has a stable end-to-end path from source text to bytecode metadata.

______________________________________________________________________

## 4. Supporting Systems

- **CanonFS (`t81_core`):** **Experimental / Stub.** The `canonfs::Driver` API supports writing/reading objects and publishing capabilities, and `make_in_memory_driver` plus the Hanoi in-memory kernel use it, but the driver never performs canonical hashing, persistence, or parity verification, and snapshot hashes are reused rather than derived from actual contents.
- **Axion Kernel (`t81_core`):** **Experimental / Stub.** `include/t81/axion/api.hpp` exposes a deterministic stub context with telemetry, and `PolicyEngine`/`Engine` hooks drive the interpreter, yet the kernel still allows every request (or only performs simple loop/match guard checks) instead of enforcing the full Axion safety model.
- **Tooling (`t81` CLI):** **Partial.** The `t81` command-line tool still drives compile/check/run/repl, but it now conserves Axion metadata from the frontend (`axion_policy_text`, `match_metadata_text`) and pushes it into the VM so trace output carries loop bounds and guard hints even though more advanced inspection/debugging commands are missing.
