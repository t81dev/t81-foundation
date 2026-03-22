______________________________________________________________________

# RFC-0023 — T81Lang `print` Canonical Runtime Surface

Version 0.1 — Standards Track\
Status: Accepted\
Updated: 2026-03-15\
Author: T81 Foundation\
Applies to: T81Lang, TISC, T81VM

vote: +1

______________________________________________________________________

# 0. Summary

This RFC proposes a formal runtime contract for the T81Lang `print(...)`
builtin:

1. Lower `print(expr)` to a dedicated opcode path.
2. Capture output in deterministic VM state (`printed_output`), not host stdout.
3. Define canonical string formatting for scalar values and supported handles.

______________________________________________________________________

# 1. Motivation

T81Lang already type-checks `print(...)`, but reproducibility requires runtime
observability to be deterministic and testable. Host-side IO is environment
dependent and cannot be a verification surface.

By making `print` part of the VM state transition, tests and auditors can
compare exact output sequences across platforms.

______________________________________________________________________

# 2. Design / Specification

### 2.1 Lowering

- Frontend IR introduces `PRINT`.
- Binary emitter maps `PRINT -> t81::tisc::Opcode::Print`.

### 2.2 Runtime Sink

- VM appends rendered strings to `State::printed_output`.
- `Opcode::Print` MUST NOT require host stdout side effects.

### 2.3 Rendering Rules (Initial Surface)

- `Int` -> decimal string (`42`)
- `Bool` -> `true` / `false`
- `FloatHandle` -> canonical decimal with `t81` suffix
- `FractionHandle` -> `num/den` with `t81` suffix
- `SymbolHandle` -> symbol text

Unsupported/invalid handle combinations trap deterministically.

______________________________________________________________________

# 3. Rationale

- Keeps runtime deterministic and replayable.
- Enables e2e tests without process-level stdout capture.
- Preserves a strict boundary between VM semantics and host integration.

______________________________________________________________________

# 4. Backwards Compatibility

- Existing programs remain valid.
- `print` now has a stable runtime target for conformance testing.

______________________________________________________________________

# 5. Security Considerations

- Avoids host IO channels in core execution paths.
- Output remains part of canonical VM state, simplifying audit/replay.

______________________________________________________________________

# 6. Open Questions

1. Should enum/option/result text rendering become normative in this RFC or a
   follow-on RFC?
2. Should CLI expose `printed_output` directly in deterministic trace mode?

______________________________________________________________________

# 7. Acceptance Criteria

| ID | Criterion | Evidence |
| :--- | :--- | :--- |
| [A-0023-01] | `Opcode::Print` exists in the ISA and is emitted by the binary emitter | `include/t81/isa/opcodes.hpp`; `core/isa/binary_emitter.cpp` |
| [A-0023-02] | `State::printed_output` accumulates rendered strings; VM does not require host stdout | `include/t81/vm/state.hpp`; `vm/vm.cpp` `Opcode::Print` handler |
| [A-0023-03] | T81Lang `print(expr)` lowers to `Opcode::Print` through the IR generator | `lang/frontend/ir_generator.cpp` PRINT IR node → `binary_emitter.cpp` |
| [A-0023-04] | Canonical rendering rules produce deterministic output for Int, Bool, FloatHandle, FractionHandle, SymbolHandle | `tests/cpp/vm_print_test.cpp` |
| [A-0023-05] | End-to-end: T81Lang source with `print` compiles, runs, and yields deterministic `printed_output` | `tests/cpp/e2e_print_runtime_test.cpp` |

______________________________________________________________________

## Acceptance Note (2026-03-15)

All five criteria above are met as of this date.

`Opcode::Print` is defined in `include/t81/isa/opcodes.hpp` and handled in `vm/vm.cpp`. The `State::printed_output` field in `include/t81/vm/state.hpp` accumulates rendered strings with no host stdout dependency. The IR generator lowers T81Lang `print(expr)` nodes to PRINT IR, which `binary_emitter.cpp` encodes as `Opcode::Print`. The rendering rules for Int (decimal), Bool (`true`/`false`), FloatHandle, FractionHandle, and SymbolHandle are implemented in the VM's Print handler. Deterministic output is verified by `vm_print_test.cpp` (9 occurrences of `printed_output` assertions) and `e2e_print_runtime_test.cpp`.

The two open questions in §6 are left for a follow-on RFC: enum/option/result text rendering and CLI trace-mode exposure of `printed_output`.

Suite status at acceptance: **329/332 tests passing** (3 pre-existing TLOADHASH SEGFAULTs excluded).
