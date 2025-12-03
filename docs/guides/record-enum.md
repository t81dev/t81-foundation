# Records & Enums Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Records & Enums Guide](#records-&-enums-guide)
  - [Record Declarations & Literals](#record-declarations-&-literals)
  - [Enum Declarations & Variants](#enum-declarations-&-variants)
  - [CLI Coverage](#cli-coverage)

<!-- T81-TOC:END -->




Record and enum declarations are now first-class structural types in the C++ frontend. They follow the canonical rules from [`spec/t81lang-spec.md` §2.4](../spec/t81lang-spec.md#2-4-composite-types) and feed precise metadata into later stages of the toolchain.

## Record Declarations & Literals

- **Declaration:** `record` blocks declare a named structure with statically typed fields. Duplicate record names or duplicate field names are reported immediately during semantic analysis.  
- **Literal expressions:** Instantiating `Point { x: 1; y: 2 }` requires every declared field to appear exactly once and enforces the field’s type using the analyzer’s `is_assignable` rules (numeric widening, `Option`, `Result`, etc.). Unknown fields, duplicates, or missing fields emit diagnostics at the literal site.  
- **Field access:** Expressions like `point.x` look up the owning record metadata, confirm the target field exists, and expose the field’s declared type so later phases can emit the right registers or instructions.

## Enum Declarations & Variants

- **Declaration:** `enum` blocks declare variants that optionally carry payloads. Duplicate variant names are rejected, and each payload type is resolved via `analyze_type_expr`.  
- **Literal expressions (future):** When a variant has a payload, the analyzer expects a matching expression (`EnumName.Variant(expr)`), validates the payload’s type against the declaration, and rejects unexpected arguments. Variants without payloads likewise forbid payload expressions.

## CLI Coverage

The structural types guide the new [Structural Types CLI regression](../tests/cpp/cli_structural_types_test.cpp), which compiles and runs a small program that declares both a `record` and an `enum`, builds a record literal, touches a field, and returns a fixed value. The `t81` CLI now serializes record layouts and enum discriminants as metadata in the compiled TISC binary, so downstream tools can reason about structured layout without rerunning the analyzer. Use that test as a reference when extending downstream tooling so every new structural invariant is surfaced in the CLI and VM.

Refer to `docs/guides/vector-literals.md` for tensor literal canonicalization; together, these guides explain how structured, deterministic data enters the IR and the VM.
