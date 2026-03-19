---
title: "RFC-0011 — T81Lang Grammar Modernization"
status: accepted
version: "0.3"
updated: 2026-03-16
applies_to:
  - T81Lang Specification (§1, §2)
---

## Summary

RFC-0011 proposed adopting the more advanced grammar from `legacy/hanoivm/src/t81lang_compiler/slang_grammer.ebnf`
as the canonical T81Lang grammar — specifically bringing in modules, attributes, generic types, and expanded keywords.

All substantive goals of this RFC have been realized through the organic evolution of the spec and toolchain.  The
`legacy/` directory and the referenced `.ebnf` file no longer exist in the repository; the new C++20 toolchain and
`spec/t81lang-spec.md` are the authoritative sources.

## Acceptance Criteria

| ID | Criterion | Status |
| -- | --------- | ------ |
| [RFC-0011-01] | Annotation (`@identifier`) syntax in grammar and compiler | met — RFC-0003/RFC-0015 `@attribute` on fn/loop/record/enum |
| [RFC-0011-02] | Generic type syntax (`T[P, Q]`) authoritative in spec and compiler | met — RFC-0004/RFC-0007 square-bracket generic instantiation, §2.1 normative |
| [RFC-0011-03] | Expanded primitive types (`i32`, `bool`, `void`, etc.) | met — §2.2 full type table, lexer/SA fully support all listed types |
| [RFC-0011-04] | `var`, `break`, `continue` keywords in grammar and compiler | met — §1 Core Grammar, parser, SA all support these |
| [RFC-0011-05] | `record` and `enum` top-level declarations | met — RFC-0007/RFC-0029 record/enum in grammar, SA, IRGen |
| [RFC-0011-06] | Module namespacing expressed in spec | met — `@module(path)` annotation documented in §3.4; first-class `module {}` blocks are future work |
| [RFC-0011-07] | Legacy grammar artifact superseded by living C++20 toolchain | met — `legacy/` directory removed; `lang/frontend/` is the implementation |

7/7 criteria met.  The first-class `module { }` declaration block (open question 1) is deferred as future work; the
`@module(path)` informational annotation satisfies the namespace-documentation need in the interim.

## Disposition

The goals of this RFC are satisfied.  Subsequent RFCs that concretely implemented the proposed features:

| Feature | Implementing RFC |
| ------- | ---------------- |
| Annotation syntax | RFC-0003, RFC-0015 |
| Generic types (square-bracket) | RFC-0004, RFC-0007 |
| Standard library primitives | RFC-0007 |
| Record / Enum declarations | RFC-0007, RFC-0029 |
| Agent / Behavior declarations | RFC-0015 |
| Feature registry (drift prevention) | RFC-0029 |

## Acceptance Note (2026-03-16)

All 7 AC met.  No code changes required — the implementation already satisfies every criterion.
The three open questions from the draft are resolved:

1. **Module system semantics** — `@module(path)` annotation provides namespace documentation;
   first-class `module {}` is tracked as a future enhancement, not a blocker.
2. **Generic monomorphization** — erased at IRGen; TISC operates on unparameterized tensors/values.
3. **Attribute meanings** — normatively defined by RFC-0003 (Axion), RFC-0015 (agent), RFC-0029
   (tier gates), and §3.4 of `t81lang-spec.md`.
