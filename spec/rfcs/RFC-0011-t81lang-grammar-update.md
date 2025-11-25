______________________________________________________________________

title: "RFC-0011 — T81Lang Grammar Modernization"
version: Draft
applies_to:

- T81Lang Specification
- T81 C++ Toolchain

______________________________________________________________________

# Summary

This RFC proposes to officially adopt the modern, feature-rich grammar found in the legacy file `legacy/hanoivm/src/t81lang_compiler/slang_grammer.ebnf` as the new canonical grammar for T81Lang. This update will align the formal `t81lang-spec.md` with the language's evolved design, incorporating critical features such as modules, attributes, and generic types.

# Motivation

The current T81Lang grammar documented in `t81lang-spec.md` is significantly less advanced than the designs explored in the legacy codebase. The analysis in `ANALYSIS.md` revealed that:

1. **`slang_grammer.ebnf`** describes a much more powerful and modern language, with features essential for building complex, modular systems.
2. The legacy Python prototype (`t81_compile.py`) already implements some of these modern features (e.g., annotations), indicating that this is the intended direction of the language.
3. The legacy CWEB frontend is obsolete and does not support any of these modern features.

To build a future-proof C++20 toolchain, we must start from a solid, forward-looking specification. Aligning the formal spec with the more advanced EBNF grammar is the necessary first step.

# Design / Specification

The core of this proposal is to replace the simplified grammar in `t81lang-spec.md §1` with the full, formal grammar from `slang_grammer.ebnf`.

The key new features to be integrated into the specification are:

- **Modules:** A system for organizing code into logical units (`module <identifier> { ... }`).
- **Attributes:** A mechanism for adding metadata to declarations, such as for Axion integration (`@axion { ... }`).
- **Generic Types:** Support for generic collections like `vector<T, N>`, `matrix<T, M, N>`, and `tensor<T, ...>`.
- **Expanded Keywords and Types:** The addition of modern keywords (`var`, `export`, `break`, `continue`) and a richer set of primitive types (`bool`, `void`, `i32`, etc.).

The updated grammar in `t81lang-spec.md` will be a direct, formatted copy of the `slang_grammer.ebnf` content.

# Rationale

- **Unifies the Language Vision:** This change resolves the significant design drift between the formal spec and the more advanced legacy artifacts.
- **Provides a Solid Foundation:** The new C++20 parser will be built against a clear, comprehensive, and modern language specification.
- **Enables Modern Features:** This officially brings features like modules and generics into the T81Lang language, which are critical for its intended use cases.

# Backwards Compatibility

This is a major extension to the T81Lang language. It is not backwards-compatible with the simple grammar currently in the spec or the legacy CWEB implementation. However, as the new C++20 toolchain is a fresh implementation, this is the ideal time to establish the new, modern grammar as the baseline.

# Security Considerations

This is a language grammar change and has no direct security implications.

# Open Questions

1. What are the detailed semantics of the module system (e.g., import/export rules)?
2. How will the new generic types be monomorphized or handled by the TISC backend?
3. What is the complete, official list of supported attributes and their meanings for the Axion kernel?

These questions will need to be addressed in subsequent RFCs or updates to the relevant specifications.
