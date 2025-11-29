# C++20 T81Lang Frontend

This directory contains the source code for the modern C++20 implementation of the T81Lang compiler frontend. Its primary responsibility is to take raw T81Lang source code and convert it into a well-formed Intermediate Representation (IR) that can be consumed by the TISC binary emitter or other backend toolchains.

## Components

The frontend is organized into a classic compiler pipeline:

-   `lexer.cpp`: The **Lexer** (or scanner) is responsible for reading the source text and converting it into a stream of tokens. It handles the low-level character-by-character processing.

-   `parser.cpp`: The **Parser** consumes the token stream from the lexer and constructs an Abstract Syntax Tree (AST). The AST is a hierarchical representation of the code's structure, defined in `include/t81/frontend/ast.hpp`. This parser is a recursive-descent parser.

-   `semantic_analyzer.cpp`: The **Semantic Analyzer** traverses the AST and enforces the semantic rules of T81Lang. This includes type checking, scope resolution, and other validation tasks that are not captured by the grammar alone. (Note: This component is currently under active development).

-   `ir_generator.cpp`: The **IR Generator** walks the validated AST and emits a linear Intermediate Representation (IR) suitable for code generation. The TISC IR is defined in `include/t81/tisc/ir.hpp`.

-   `symbol_table.cpp`: Provides a symbol table implementation used by the parser and semantic analyzer to track identifiers, types, and scopes.

## Workflow

The typical data flow through the frontend is as follows:

1.  `T81Lang Source Text` -> **Lexer** -> `Token Stream`
2.  `Token Stream` -> **Parser** -> `Abstract Syntax Tree (AST)`
3.  `AST` -> **Semantic Analyzer** -> `Validated & Annotated AST`
4.  `Validated AST` -> **IR Generator** -> `TISC Intermediate Representation`
