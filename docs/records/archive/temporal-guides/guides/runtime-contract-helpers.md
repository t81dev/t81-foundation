---
layout: page
title: "Guide: Runtime Contract Helpers"
---

# Guide: Runtime Contract Helpers

This guide covers new low-level helpers used to harden determinism and safety checks in runtime flows.

## TISC Opcode Contract Helpers

Header: `include/t81/isa/opcodes.hpp`

Added helpers:

- `t81::tisc::opcode_name(Opcode)`
- `t81::tisc::kAllOpcodes`
- `t81::tisc::is_valid_opcode(std::uint8_t)`

Typical use cases:

- Building complete opcode matrix tests.
- Centralized decode validation.
- Stable opcode name rendering in logs and tooling.

`core/isa/encoding.cpp` now uses `is_valid_opcode(...)` so decode acceptance tracks the full enum range through `MetaRefine`.

## VM Safety Counters

Header: `include/t81/vm/state.hpp`

Added state fields:

- `call_depth`
- `contradiction_events`

Runtime behavior in `vm/vm.cpp`:

- `Call` enforces a hard recursion ceiling and records a security event when exceeded.
- `Ret` decrements tracked depth and increments contradiction count when depth underflows.
- Axion call context now reports recursion depth from `max(stack_frames.size(), call_depth)`.

Related reason keys in `include/t81/axion/reasons.hpp`:

- `kRecursionCeiling`
- `kContradictionDetected`

These checks provide deterministic guardrails while preserving existing VM control-flow semantics.
