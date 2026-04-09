# T81 Language Reference

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Language Reference](#t81-language-reference)
  - [Types](#types)
    - [Primitive Types](#primitive-types)
    - [Infinite Types (T19683)](#infinite-types-t19683)
  - [Statements & Blocks](#statements-&-blocks)
    - [Loop Annotations](#loop-annotations)
    - [Cognitive Tier Blocks](#cognitive-tier-blocks)
      - [Reflective (T729)](#reflective-t729)
      - [Recursive (T2187)](#recursive-t2187)
      - [Distributed (T6561)](#distributed-t6561)
      - [Infinite (T19683)](#infinite-t19683)
    - [Pattern Matching](#pattern-matching)
  - [Keywords](#keywords)

<!-- T81-TOC:END -->


This document provides a reference for the T81 programming language (T81Lang), including syntax, types, and cognitive tier constructs.

**Last Updated:** February 17, 2026

## Types

### Primitive Types

| Type | Description | Example Literal |
| :--- | :--- | :--- |
| `T81Int` | Base-81 Integer | `123t81` |
| `T81Float` | Base-81 Float | `1.23t81` |
| `T81Fraction`| Rational Number | `1/3` (computed) |
| `Bool` | Boolean | `true`, `false` |
| `Symbol` | Symbolic Atom (T243) | `:my_symbol` |
| `T81String` | String | `"hello"` |
| `T81Bytes` | Byte sequence | `T81Bytes("abc")` |

### Infinite Types (T19683)

| Type | Description | Example Literal |
| :--- | :--- | :--- |
| `InfiniteCanonicalForm` | Infinite Structure Seed | `∞{seed_expr}` or `infinite{seed_expr}` |

## Statements & Blocks

### Loop Annotations

Loops can be annotated with `@bounded` to enforce termination properties or declare infinite intent (checked by Axion).

```t81
@bounded(infinite)
loop {
    // Infinite loop logic
}

@bounded(loop(condition))
for x in items {
    // ...
}

@bounded(100)
while (x < 100) {
    // ...
}
```

### Cognitive Tier Blocks

T81Lang supports dedicated blocks for specific cognitive tiers:

#### Reflective (T729)

Captures trace metadata and justifies execution history.

```t81
reflect {
    // Reflective logic
}
```

#### Recursive (T2187)

Recursive functions are defined using `recurse`. The body typically uses arrow syntax `pattern -> result` for base and recursive steps.

```t81
recurse factorial(n) {
    base -> 1t81;
    step -> n * factorial(n - 1t81);
}
```

#### Distributed (T6561)

Distributed logic uses the `distributed` block for consensus and coherence operations.

```t81
distributed {
    // Distributed logic
}
```

#### Infinite (T19683)

Infinite-tier logic uses the `infinite` block for manipulating `InfiniteCanonicalForm` structures.

```t81
infinite {
    // Infinite logic
}
```

### Pattern Matching

Pattern matching is supported via the `match` expression.

```t81
match (value) {
    0 => "zero",
    1 => "one",
    x if x > 10 => "large",
    _ => "other"
}
```

## Keywords

`fn`, `let`, `var`, `const`, `if`, `else`, `while`, `for`, `loop`, `break`, `continue`, `return`, `match`, `record`, `enum`, `type`, `module`, `export`, `reflect`, `recurse`, `distributed`, `infinite`.
