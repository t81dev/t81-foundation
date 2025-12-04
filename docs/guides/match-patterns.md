# Match Patterns

This guide summarizes the match pattern semantics provided by the T81 compiler frontend so you can write expressive, deterministic arms that descend into nested variants and records.

## Nested Variant Bindings

You can match an enum variant and immediately open another variant's payload without introducing intermediate locals. The parser now recognizes variants that carry another match pattern, so bindings like `Nested(Data({x: px, y}))` walk into the inner payload and reuse the same binding helpers that power simple identifiers, tuples, or records.

```t81
    enum Inner {
        Data(Point);
        Empty;
    };

    enum Outer {
        Nested(Inner);
        Missing;
    };

    fn main() -> i32 {
        var value: Outer;
        return match (value) {
            Nested(Data({x: px, y})) => px + y;
            Nested(Empty) => 0;
            Missing => -1;
        };
    }
```

In the example above the compiler:

1. Resolves `Nested` as the outer variant, verifies it carries an `Inner`, and binds according to metadata retrieved from `EnumInfo`.
2. Descends into the nested `Data` variant and checks that its record payload uses the fields `x` and `y`.
3. Reuses existing binding logic so identifier (`px`/`y`), tuple, and record semantics remain deterministic even inside nested arms.

Variant bindings that lack a payload or expect the wrong shape now produce diagnostics surfaced by `SemanticAnalyzer`, ensuring `match_nested_enum_success` and its siblings remain correct.

## CLI metadata for Axion traces

The CLI prints Axion metadata that reflects these richer match arms. The formatter uses the public `SemanticAnalyzer::match_metadata()` API and `type_name()` helper (see `src/cli/driver.cpp`) so Axion traces include each arm's pattern kind (`Variant`, `Record`, `Tuple`, `Identifier`) plus payload types.

When you enable Axion match metadata you will see output similar to:

```
(match-metadata (match (scrutinee Enum) (type i32) (arms
    (arm (variant Nested) (pattern Variant) (type Inner))
    (arm (variant Nested) (pattern Variant) (type Inner))
    (arm (variant Missing) (pattern None))
)))
```

This trace is compatible with `Axion` tooling, which expects deterministic metadata for loops and match expressions. Writing nested arms and verifying them with the CLI metadata keeps the frontend behavior transparent to future contributors.
