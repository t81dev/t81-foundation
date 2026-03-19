# RFC-0036 — T81Lang Foreign Function Interface Grammar

| Field       | Value                                              |
| :---------- | :------------------------------------------------- |
| RFC         | 0036                                               |
| Title       | T81Lang Foreign Function Interface Grammar         |
| Status      | **Accepted**                                       |
| Depends on  | RFC-00B8 (Governed FFI VM layer)                   |
| ISA Impact  | `FFI_CALL` IR opcode (already defined in ir.hpp)   |
| Date        | 2026-03-16                                         |

---

## 1  Motivation

RFC-00B8 introduced the governed FFI *infrastructure* at the VM level
(`FFICall`, `FFIRegister`, `FFIPolicySet` opcodes; `FFILibraryRegistry`;
`FFIDispatcher`).  Before this RFC, T81Lang had no grammar for declaring or
invoking foreign functions — the only path was hand-crafted TISC bytecode.

This RFC adds a first-class `foreign {}` block to T81Lang so that
deterministic, governed, and quarantined foreign functions can be declared
once and called with ordinary dot-call syntax.

---

## 2  Grammar (EBNF)

```ebnf
foreign_decl  = "foreign" [ policy_qualifier ] "{" foreign_fn* "}"
policy_qualifier = "deterministic" | "governed" | "quarantined"
foreign_fn    = "fn" IDENTIFIER "(" param_list? ")" "->" type ";"
```

The policy qualifier is optional; omitting it defaults to the Axion Governance Kernel's
ambient enforcement policy for the current execution context.

### 2.1  Call syntax

Foreign functions are called via the `foreign` pseudo-namespace:

```
foreign.<function_name>(<arg_list>)
```

`foreign` is a contextual namespace token, not a value.  It may also appear
as an ordinary identifier in binding position (`let foreign: T = ...`) without
causing a parse error.

---

## 3  Semantics

### 3.1  Declaration

A `foreign {}` block registers each `fn` signature with the **Semantic
Analyzer** (`SemanticAnalyzer::_foreign_definitions`).  The stored metadata
(`ForeignFnInfo`) captures:

- `name`         — function name (unqualified)
- `policy`       — policy qualifier string
- `param_types`  — resolved `Type` objects for each parameter
- `return_type`  — resolved `Type` for the return value

Duplicate function names across any number of `foreign {}` blocks in the same
compilation unit are a **compile-time error**.

### 3.2  Call-site lowering

When the IR generator encounters `foreign.<name>(args)`:

1. Evaluate and `PUSH` each argument in left-to-right order.
2. Emit `FFI_CALL RD, arg_count` with `text_literal = "<name>"`.

The `text_literal` field carries the unqualified function name to the VM
dispatcher (`FFIDispatcher::call()`), which resolves it in
`FFILibraryRegistry` and applies the governance policy pipeline before
executing.

If the return type is not `void`, the result register (`RD`) is recorded
as the expression result.

---

## 4  Policy enforcement

| Qualifier         | Axion enforcement                                              |
| :---------------- | :------------------------------------------------------------- |
| `deterministic`   | Execution must be bit-reproducible; I/O and randomness banned  |
| `governed`        | Full Axion audit trail; resource quota enforced                |
| `quarantined`     | Sandboxed memory view; capability-restricted execution         |
| *(none)*          | Ambient policy from Axion policy engine                        |

---

## 5  IR opcode

`FFI_CALL` is defined in `include/t81/isa/ir.hpp`:

```cpp
// RFC-0036 — Governed foreign function call
FFI_CALL,  // FFICall RD, arg_count  — text_literal carries the function name
```

---

## 6  Acceptance criteria

| ID    | Criterion                                                              | Status   |
| :---- | :--------------------------------------------------------------------- | :------- |
| AC-1  | `foreign` lexes as `TokenType::Foreign`                               | ✅ Pass   |
| AC-2  | Parser produces a `ForeignDecl` node from `foreign { fn f() -> T; }` | ✅ Pass   |
| AC-3  | `ForeignDecl` captures policy qualifier and function name              | ✅ Pass   |
| AC-4  | Empty `foreign { }` block with no policy qualifier is valid            | ✅ Pass   |
| AC-5  | Multiple `fn` signatures in one block are all captured                 | ✅ Pass   |
| AC-6  | SA registers foreign functions in `foreign_definitions()`              | ✅ Pass   |
| AC-7  | Duplicate foreign function name is a compile-time error                | ✅ Pass   |
| AC-8  | `foreign.<name>(args)` call site lowers to `FFI_CALL` IR instruction  | ✅ Pass   |
| AC-9  | `foreign` usable as variable name in binding position                  | ✅ Pass   |

All 9 ACs verified by `tests/cpp/t81lang_foreign_ffi_test.cpp`.

---

## 7  Files changed

| File                                                          | Role                                 |
| :------------------------------------------------------------ | :----------------------------------- |
| `include/t81/frontend/lexer.hpp`                              | Added `Foreign` to `TokenType` enum  |
| `lang/frontend/lexer.cpp`                                     | Added `"foreign"` keyword entry      |
| `include/t81/frontend/ast.hpp`                                | `ForeignFn`, `ForeignDecl` structs   |
| `include/t81/frontend/parser.hpp`                             | `foreign_declaration()` declaration  |
| `lang/frontend/parser.cpp`                                    | `foreign_declaration()` impl         |
| `include/t81/frontend/semantic_analyzer.hpp`                  | `ForeignFnInfo`, `_foreign_definitions`, `visit(ForeignDecl)` |
| `lang/frontend/semantic_analyzer.cpp`                         | `visit(ForeignDecl)` + call dispatch |
| `include/t81/frontend/ir_generator.hpp`                       | `visit(ForeignDecl)` declaration     |
| `lang/frontend/ir_generator.cpp`                              | `visit(ForeignDecl)` + `FFI_CALL` emission |
| `include/t81/isa/ir.hpp`                                      | `FFI_CALL` opcode                    |
| `tests/cpp/t81lang_foreign_ffi_test.cpp`                      | 9 acceptance tests                   |
| `tests/cpp/test_utils.hpp`                                    | `AstPrinter::visit(ForeignDecl)`     |
| `tests/cpp/test_parser_regression_audit.cpp`                  | Stub `visit(ForeignDecl)`            |

---

## 8  Non-goals (Phase 1)

- Runtime loading of shared libraries via `foreign {}` (requires Phase 2
  of RFC-00B8 `FFILibraryRegistry` dlopen integration).
- `foreign` call syntax in constant expressions or compile-time evaluation.
- Type-coercion between T81 types and C ABI types — handled by
  `FFIDispatcher` in a future RFC.
