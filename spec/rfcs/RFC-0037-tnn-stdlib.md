# RFC-0037 — T81Lang TNN Standard Library

| Field       | Value                                                        |
| :---------- | :----------------------------------------------------------- |
| RFC         | 0037                                                         |
| Title       | T81Lang TNN Standard Library                                 |
| Status      | **Accepted**                                                 |
| Stage       | 3 — Research Ecosystem                                       |
| Depends on  | RFC-0034 (Ternary-Native Inference TISC ops)                 |
| Date        | 2026-03-16                                                   |

---

## 1  Motivation

RFC-0034 introduced six ternary-native inference TISC opcodes
(`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`) at the VM
and ISA level.  Before this RFC, those opcodes were only reachable via
hand-crafted TISC bytecode or `@attention`/`@qmatmul` function annotations —
there was no general-purpose T81Lang API for ternary neural network inference.

This RFC adds six first-class `std.tnn.*` builtin functions to the T81Lang
stdlib, completing the **full-stack** path:

```
T81Lang source (std.tnn.*)
       ↓ IR generator (RFC-0037)
tisc::ir::Opcode (TWMATMUL / TQUANT / TATTN / TWEMBED / TERNACCUM / TACT)
       ↓ VM (RFC-0034)
Ternary-native execution — multiplication-free, Axion-governed
```

---

## 2  API Reference

| T81Lang surface name | Canonical | Signature | TISC opcode | Returns |
| :------------------- | :-------- | :-------- | :---------- | :------ |
| `std.tnn.matmul`     | `tnn_matmul` | `(activations: Tensor, weights: Tensor) -> Tensor` | `TWMATMUL RD, R_ACT, R_WT` | Tensor |
| `std.tnn.quant`      | `tnn_quant`  | `(src: T81Float, threshold: T81Float) -> Tensor`   | `TQUANT RD, R_SRC, R_THR`  | Tensor |
| `std.tnn.attn`       | `tnn_attn`   | `(q: Tensor, k: Tensor, v: Tensor) -> Tensor`      | `TATTN RD, R_Q, PACK(K,V)` | Tensor |
| `std.tnn.embed`      | `tnn_embed`  | `(table: Tensor, idx: T81BigInt) -> Tensor`        | `TWEMBED RD, R_TABLE, R_IDX`| Tensor |
| `std.tnn.accum`      | `tnn_accum`  | `(weights: Tensor, activations: Tensor) -> T81Float`| `TERNACCUM RD, R_WT, R_ACT`| T81Float |
| `std.tnn.act`        | `tnn_act`    | `(src: T81Float, mode: T81BigInt) -> T81BigInt`    | `TACT RD, R_SRC, R_MODE`   | T81BigInt |

All six functions require `@tier(2)` or higher (inference context) on the
calling function.  They are not effect surfaces and may be used inside
`@pure` functions if the tier constraint is satisfied.

---

## 3  Usage Example — Single TNN Layer Forward Pass

```t81lang
// Minimal ternary neural network layer:
//   1. Quantize float input to ternary representation
//   2. Multiplication-free matmul against ternary weights
//   3. Accumulate ternary dot product
//   4. Apply ternary activation

fn tnn_layer_forward(
    input:  T81Float,
    thr:    T81Float,
    weights: Tensor,
    mode:   T81BigInt
) -> T81BigInt {
    let quantized: Tensor   = std.tnn.quant(input, thr);
    let pre_act:   Tensor   = std.tnn.matmul(quantized, weights);
    let scalar:    T81Float = std.tnn.accum(pre_act, quantized);
    return std.tnn.act(scalar, mode);
}
```

### Activation modes (passed as `T81BigInt` to `std.tnn.act`)

| Mode | Name | Mapping |
| :--- | :--- | :------ |
| `1` | TernaryStep | $(−∞, −0.5) → −1$; $[−0.5, 0.5] → 0$; $(0.5, ∞) → +1$ |
| `2` | TanhQuantized | High-fidelity fixed-point ternary approximation of tanh |

---

## 4  IR Lowering

Each `std.tnn.*` call is dispatched in `IRGenerator::visit(CallExpr)` before
the table-driven builtin fallback.  The encoding follows the same patterns as
RFC-0026 AI-M6 ops:

**Two-argument ops** (matmul, quant, embed, accum, act) — three-register
encoding: `dest, src1, src2`:
```cpp
instr.opcode = tisc::ir::Opcode::TWMATMUL;
instr.operands = {dest.reg, act.reg, wt.reg};
```

**Three-argument op** (attn) — packed K/V encoding, identical to RFC-0026
`ATTN`:
```cpp
const int32_t packed_kv = (k.reg.index & 0xFF) | ((v.reg.index & 0xFF) << 8);
instr.opcode = tisc::ir::Opcode::TATTN;
instr.operands = {dest.reg, q.reg, tisc::ir::Immediate{packed_kv}};
```

---

## 5  Registry Entries

Six entries added to `kBuiltinTable` in `include/t81/frontend/builtin_registry.hpp`:

```cpp
{"std.tnn.matmul", "tnn_matmul", 2, Type::Kind::Tensor, {}, {2}, false, Custom, …},
{"std.tnn.quant",  "tnn_quant",  2, Type::Kind::Tensor, {}, {2}, false, Custom, …},
{"std.tnn.attn",   "tnn_attn",   3, Type::Kind::Tensor, {}, {2}, false, Custom, …},
{"std.tnn.embed",  "tnn_embed",  2, Type::Kind::Tensor, {}, {2}, false, Custom, …},
{"std.tnn.accum",  "tnn_accum",  2, Type::Kind::Float,  {}, {2}, false, Custom, …},
{"std.tnn.act",    "tnn_act",    2, Type::Kind::BigInt, {}, {2}, false, Custom, …},
```

Six new opcodes added to `tisc::ir::Opcode` in `include/t81/isa/ir.hpp`
(mirrors opcodes.hpp; required for the IR-level emission pipeline).

---

## 6  Acceptance Criteria

| ID    | Criterion                                                              | Status   |
| :---- | :--------------------------------------------------------------------- | :------- |
| AC-1  | `std.tnn.matmul` registered; `tnn_matmul(act, wt)` lowers to `TWMATMUL` | ✅ Pass |
| AC-2  | `std.tnn.quant` registered; `tnn_quant(x, thr)` lowers to `TQUANT`    | ✅ Pass   |
| AC-3  | `std.tnn.attn` registered; `tnn_attn(q, k, v)` lowers to `TATTN`      | ✅ Pass   |
| AC-4  | `std.tnn.embed` registered; `tnn_embed(tbl, idx)` lowers to `TWEMBED`  | ✅ Pass   |
| AC-5  | `std.tnn.accum` registered; return type is `T81Float`; lowers to `TERNACCUM` | ✅ Pass |
| AC-6  | `std.tnn.act` registered; return type is `T81BigInt`; lowers to `TACT` | ✅ Pass   |
| AC-7  | Wrong arity produces a compile-time SA error                           | ✅ Pass   |
| AC-8  | Forward-pass composition `quant → matmul → accum → act` emits all four opcodes | ✅ Pass |

All 20 assertions verified by `tests/cpp/tnn_stdlib_test.cpp`.

---

## 7  Files Changed

| File | Role |
| :--- | :--- |
| `include/t81/frontend/builtin_registry.hpp` | 6 new `std.tnn.*` builtin entries |
| `lang/frontend/ir_generator.cpp` | Custom IR emit handlers for all 6 TNN ops |
| `include/t81/isa/ir.hpp` | `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT` added to `tisc::ir::Opcode` |
| `tests/cpp/tnn_stdlib_test.cpp` | 8 acceptance tests (20 assertions) |
| `spec/rfcs/RFC-0037-tnn-stdlib.md` | This spec |

---

## 8  Stage 3 Roadmap

This RFC establishes the T81Lang entry point for ternary neural network
research.  Planned follow-on work:

- **Weight serialization** — T81WTN `.t81w` file loading via `std.tnn.embed`
  + `std.tensor.load` to initialize weight tensors from CanonFS blobs
- **Multi-layer composition** — higher-level `tnn.model` record type with
  `forward()` behavior dispatch
- **Training loop** — backward pass using `TNEURAL_BWD` + gradient
  accumulation via `TERNACCUM`
- **Post-quantum crypto** — separate RFC for lattice polynomial multiply over
  {−1, 0, +1}; shares the same ternary arithmetic substrate
