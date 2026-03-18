---
title: "RFC-0015 — First-Class Agents and Agentic Constructs"
status: accepted
version: "0.5"
updated: 2026-03-15
applies_to:
  - T81Lang Specification (§1, §3.5)
  - TISC Specification (AgentInvoke opcode)
  - Axion Governance Kernel Specification (AGENT_INVOKE audit event)
---

## Summary

This RFC introduces `agent` as a first-class top-level declaration in T81Lang,
providing a named collection of stateless `behavior` functions that compile to
`AGENT_INVOKE` rather than ordinary `CALL`.  Every `AGENT_INVOKE` emission causes
the Axion Policy Kernel to record an audit event before dispatch, making agent
invocations fully observable and policy-enforceable.

## Motivation

Prior RFCs introduced tensor primitives and the `infer` expression but provided no
language-level construct for organizing model behaviors into a cohesive, named entity.
`agent` addresses three concrete needs:

1. **Namespace for behaviors** — `SimpleNet.infer(x)` is unambiguous and survives
   rename-refactoring; bare function names do not scale to multi-model programs.
2. **Axion observability** — every behavior invocation must carry enough metadata for
   the Axion kernel to apply tier-based policies.  `AGENT_INVOKE` vs. `CALL` provides
   the necessary discrimination at the TISC level.
3. **`infer` sugar** — `infer Agent(args)` is the idiomatic call form for the most
   common operation; it MUST desugar to a validated `Agent.infer(args)` → `AGENT_INVOKE`.

## Acceptance Criteria

| ID | Criterion | Status |
| -- | --------- | ------ |
| [RFC-0015-01] | `agent` and `behavior` are lexed as distinct keyword tokens | met |
| [RFC-0015-02] | `agent Foo { behavior bar(…) -> T { } }` parses to `AgentDecl` with one `BehaviorDecl` | met |
| [RFC-0015-03] | Semantic analyzer registers the agent; `Foo.bar(…)` call site type-checks against the declared signature | met |
| [RFC-0015-04] | IR generator emits `AGENT_INVOKE` (not `CALL`) for behavior calls | met |
| [RFC-0015-05] | `infer Foo(x)` desugars to `Foo.infer(x)` → `AGENT_INVOKE` | met |
| [RFC-0015-06] | VM dispatches `AGENT_INVOKE` and Axion emits an audit event | met |
| [RFC-0015-07] | Duplicate agent name is a semantic error | met |
| [RFC-0015-08] | Calling an undeclared behavior is handled without crash | met |
| [RFC-0015-09] | `infer` on an agent without an `infer` behavior is a semantic error | met |

All 9/9 criteria verified by `tests/cpp/agent_constructs_test.cpp` (16 checks, 0 failures).

## Specification Changes

### §1 Core Grammar

Added to the `declaration` production and new `agent_decl` / `behavior_decl` rules:

```ebnf
declaration   ::= fn_decl | agent_decl | type_decl | record_decl | enum_decl
                | var_decl | let_decl | statement

agent_decl    ::= "agent" identifier "{" behavior_decl* "}"
behavior_decl ::= "behavior" identifier "(" parameters ")" "->" type block
```

### §3.5 Agent Declarations

See `spec/t81lang-spec.md §3.5` (added 2026-03-15) for the normative semantics,
`infer` sugar desugaring rules, and error conditions table.

### TISC ISA — `AgentInvoke` opcode

A new opcode `AgentInvoke` (value immediately after `GcSafepoint`) is added to
`include/t81/isa/opcodes.hpp`.  Encoding identical to `Call`:

```text
AgentInvoke  RD, R_ADDR   ; push (PC+1), jump to R_ADDR; Axion event emitted
```

The JIT treats `AgentInvoke` as an Axion-boundary exit point (same group as `Call`
with policy check), causing the trace to OSR back to the interpreter which handles
the full tier-check + audit sequence.

### Axion audit event

`vm.cpp` emits `record_axion_event(...)` before dispatching `AgentInvoke`.  The event
is appended to `State::axion_log`, observable via `IVirtualMachine::state().axion_log`.

## Implementation Notes

- **Behavior names as contextual keywords**: `infer` (and other reserved keywords) are
  valid behavior names.  The parser accepts `TokenType::Infer` in the behavior-name
  position as a contextual identifier.
- **Agent call callee type**: `Net.run(42)` produces a `FieldAccessExpr` callee, not a
  `VariableExpr`.  The IRGen agent-dispatch check runs *before* the `VariableExpr` gate.
- **SA agent call dispatch**: Added inside the dotted-method block of `visit(CallExpr)`,
  keyed on `_agent_definitions`.  Returns the behavior's declared return type.

## Acceptance Note (2026-03-15)

All 9 acceptance criteria are met.  Implementation spans:

- `include/t81/frontend/lexer.hpp` — `Agent`, `Behavior` token types
- `lang/frontend/lexer.cpp` — keyword entries
- `include/t81/frontend/ast.hpp` — `BehaviorDecl`, `AgentDecl` structs
- `include/t81/frontend/parser.hpp` / `lang/frontend/parser.cpp` — `agent_declaration()`
- `include/t81/frontend/semantic_analyzer.hpp` / `lang/frontend/semantic_analyzer.cpp` —
  `AgentInfo`, `visit(AgentDecl)`, agent call dispatch in `visit(CallExpr)`, `infer` sugar
- `include/t81/isa/opcodes.hpp` — `AgentInvoke` opcode + `opcode_name()`
- `include/t81/isa/ir.hpp` — `tisc::ir::Opcode::AGENT_INVOKE`
- `lang/frontend/ir_generator.cpp` — agent pre-pass labeling, `visit(AgentDecl)`, agent
  call dispatch (before `VariableExpr` gate), `infer` sugar lowering
- `core/vm/vm.cpp` — `AgentInvoke` dispatch with Axion event
- `runtime/jit/jit_compiler.cpp` — `AgentInvoke` in Axion-boundary exit group
- `tests/cpp/agent_constructs_test.cpp` — 9 test functions, 16/16 assertions passing
- `spec/t81lang-spec.md` — §1 grammar additions, §3.5 new section
