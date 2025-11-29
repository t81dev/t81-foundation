# T81Lang Frontend Headers

This directory contains the public header files for the C++20 T81Lang compiler frontend. These headers define the primary data structures and interfaces that make up the frontend's Abstract Syntax Tree (AST) and its various components.

## Key Headers

-   `ast.hpp`: This is the most important header in this directory. It defines the hierarchy of C++ classes that represent the nodes of the Abstract Syntax Tree (AST). The AST is the central data structure that the parser builds and that the semantic analyzer and IR generator consume.

-   `lexer.hpp`: Defines the `Lexer` class, which is responsible for lexical analysis (tokenizing) of T81Lang source code.

-   `parser.hpp`: Defines the `Parser` class, which implements the recursive-descent parser that builds the AST from a stream of tokens.

-   `semantic_analyzer.hpp`: Defines the `SemanticAnalyzer` class, which is responsible for the semantic validation of the AST, including type checking.

-   `ir_generator.hpp`: Defines the `IRGenerator` class, which walks the AST and generates the TISC Intermediate Representation (IR).

-   `symbol_table.hpp`: Defines the `SymbolTable` class, a helper data structure used for tracking identifiers and their associated information during parsing and semantic analysis.
