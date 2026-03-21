# vscode-t81 — T81 Foundation VS Code Extension

Syntax highlighting and snippets for **T81Lang** — the balanced ternary
language targeting the T81 Foundation VM.

## Features

### Syntax Highlighting

- **Keywords** — control flow (`if`, `else`, `loop`, `match`, `return`, …),
  declarations (`fn`, `let`, `record`, `enum`, `agent`, `behavior`, `foreign`, …),
  modifiers (`mut`, `as`, `assert`), cognitive tier keywords (`reflect`,
  `recurse`, `distributed`, `infinite`, `infer`, `train`)
- **Annotations** — `@tier(N)`, `@pure`, `@bounded`, `@axion_verify`,
  `@ternary_inference`, `@qmatmul`, `@attention`
- **Types** — full numeric tower (`T81BigInt`, `T81Float`, `T81Fraction`,
  `T81Fixed`, `T81Complex`, `T81Quaternion`, `T81Uint`, `T81Prob`), structural
  types (`Cell`, `T81Qutrit`, `Symbol`), collections (`List`, `Map`, `Set`,
  `Tree`, `Vector`, `Matrix`, `Tensor`, `Graph`), AI/inference types
  (`NativeTensor`, `T81WTN`) and wrappers (`Option`, `Result`)
- **Stdlib namespaces** — `std.core`, `std.math`, `std.tensor`, `std.tnn`,
  `std.collections`, `std.distributed`, `std.crypto`, … highlighted as
  support functions
- **Literals** — ternary (`t012…`), base-81 (`0b81_…`), numeric, string,
  byte string (`b"…"`), infinite (`∞`)
- **Operators** — arithmetic, comparison, logical, bitwise, `->`, `=>`, `..`

### Snippets

| Prefix | Description |
| ------ | ----------- |
| `fn` | Function declaration with `@tier` annotation |
| `fnpure` | `@pure @tier` function |
| `agent` | RFC-0015 agent with behavior |
| `behavior` | RFC-0015 behavior declaration |
| `record` | Record type |
| `enum` | Enum type |
| `loop` | `@bounded` loop (required by T81Lang semantics) |
| `for` | For-in loop |
| `match` | Match expression with wildcard arm |
| `let` / `letmut` | Immutable / mutable binding |
| `tmatmul` | `std.tensor.matmul` call |
| `tnnmatmul` | RFC-0034 `std.tnn.matmul` with annotations |
| `tnnattn` | RFC-0034 `std.tnn.attn` |
| `infer` | Full RFC-0034 inference function scaffold |
| `module` | Module declaration with exported main |
| `gossip` | Tier-4 distributed gossip pattern |

## File Association

`.t81` and `.tisc` files are automatically detected.

## Files

| File | Purpose |
| ---- | ------- |
| `package.json` | Extension manifest |
| `language-configuration.json` | Comment/bracket configuration |
| `syntaxes/t81.tmLanguage.json` | TextMate grammar |
| `snippets/t81.json` | Code snippets |

## Development

Grammar keywords are derived directly from the lexer token table at
`include/t81/frontend/lexer.hpp` and the parser annotation list in
`lang/frontend/parser.cpp`. Keep them in sync when new language features land.

Stdlib completions track `include/t81/frontend/builtin_registry.hpp`.

To test locally: open the `tools/vscode-t81/` folder in VS Code and press
`F5` to launch an Extension Development Host.
