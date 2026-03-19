# T81Lang Feature Registry

Version 1.0\
Status: Active\
Authority: RFC-0029\
Updated: 2026-03-15

This is the single source of truth for T81Lang feature maturity and tier
gating.  The implementation source of truth is
`include/t81/frontend/builtin_registry.hpp` (`kBuiltinTable`).  This document
is the human-readable projection of that table.

______________________________________________________________________

## Tier Model

| Tier | Label | Scope |
| :--- | :--- | :--- |
| 0 | Unrestricted | Available in all DCP builds |
| 1 | Symbolic | Available from @tier(1) upward |
| 2 | Effect | Available from @tier(2) upward; may perform I/O or metadata reflection |
| 3 | Stateful | Available from @tier(3) upward |
| 4 | Distributed | Available from @tier(4) upward; experimental distribution primitives |
| 5 | Infinite | Reserved; not yet assigned |

`@pure` functions additionally block all features where `is_effect_surface =
true`, regardless of tier.

______________________________________________________________________

## Feature Table

### Core (unrestricted)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `core_assert` | `std.core.assert` | — | No | Custom SA check |
| `print` | `std.core.debug` / `std.io.println` | — | Yes | PRINT opcode |
| `option_unwrap_or` | `std.core.unwrap_or` | — | No | Custom SA + IR |

### Option / Result (unrestricted)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `option_is_some` | `std.option.is_some` | — | No | |
| `option_is_none` | `std.option.is_none` | — | No | Custom |
| `option_unwrap` | `std.option.unwrap` | — | No | Custom |
| `result_is_ok` | `std.result.is_ok` | — | No | |
| `result_is_err` | `std.result.is_err` | — | No | Custom |
| `result_unwrap` | `std.result.unwrap` | — | No | Custom |
| `result_unwrap_err` | `std.result.unwrap_err` | — | No | Custom |

### I/O and Async (Tier 2+)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `io_stream` | `std.io.stream` | 2 | Yes | LoadiSym |
| `io_net` | `std.io.net` | 2 | Yes | LoadiSym |
| `async_thread` | `std.async.thread` | 2 | Yes | LoadiSym |
| `async_promise` | `std.async.promise` | 2 | Yes | LoadiSym |

### Sys / Agent (Tier 2+)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `sys_reflect` | `std.sys.reflect` | 2 | Yes | META_REFLECT opcode |
| `agent_self_reflect` | `std.agent.self_reflect` | 2 | Yes | META_REFLECT opcode |

### Tensor (Tier 2+)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `weights.load` | `std.tensor.load` | 2 | Yes | Custom SA + IR |
| `Tensor.matmul` | `std.tensor.matmul` | 2 | Yes | TMATMUL opcode |
| `tensor_dot` | `std.tensor.dot_product` | 2 | Yes | Custom IR |

### Distributed (Tier 4+ — Experimental)

| Canonical Name | T81Lang Call | Tier | Effect Surface | Notes |
| :--- | :--- | :--- | :--- | :--- |
| `dist_gossip` | `std.distributed.gossip` | 4 | Yes | GOSSIP opcode |
| `dist_merge` | `std.distributed.merge` | 4 | Yes | MERGE opcode |
| `dist_sync` | `std.distributed.sync` | 4 | Yes | TICKSYNC opcode |
| `dist_coherence` | `std.distributed.coherence` | 4 | Yes | COHERENCE opcode |
| `dist_seal` | `std.distributed.seal` | 4 | Yes | DISTSEAL opcode |

______________________________________________________________________

## Enforcement

Tier gating is enforced at two points in the compiler pipeline:

1. **Semantic Analyzer** — `minimum_tier_for_call_surface()` reads `min_tier`
   from `kBuiltinTable`.  `collect_tier_violations()` walks the AST and emits
   a compile-time error if a call to a gated feature is made from a function
   whose declared `@tier` is below the required minimum.
   `enforce_active_tier_minimum()` (SA line ~2047) is called for every resolved
   builtin call.

2. **VM dispatch** — Experimental opcodes not enabled for the active profile
   emit `DecodeFault` and halt execution deterministically rather than silently
   falling through.

A standard DCP build has no `@tier` annotation ≥ 2 in core code paths, so
all Tier 2+ and Tier 4+ features are unreachable without an explicit
`@tier(N)` annotation in the source program.

______________________________________________________________________

## Adding a New Feature

1. Add one row to `kBuiltinTable` in `include/t81/frontend/builtin_registry.hpp`.
2. Set `min_tier` to the appropriate tier (or leave `nullopt` for unrestricted).
3. Update this document.
4. Add a test in `tests/cpp/` that confirms the tier gate blocks the feature
   at the correct boundary.
