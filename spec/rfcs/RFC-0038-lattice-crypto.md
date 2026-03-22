# RFC-0038 — Ternary Lattice Cryptography Primitives

| Field       | Value                                 |
|-------------|---------------------------------------|
| RFC         | 0038                                  |
| Title       | Ternary Lattice Cryptography          |
| Status      | proposed                              |
| Stage       | 3 — Post-Quantum Security             |
| Author      | t81dev                                |
| Created     | 2026-03-16                            |
| Depends-on  | RFC-0034, RFC-0037                    |

---

## 1. Summary

Introduces two TISC opcodes — **POLYMUL** and **POLYMOD** — and corresponding
T81Lang builtins **`std.crypto.polymul`** / **`std.crypto.polymod`** for
negacyclic polynomial arithmetic over the ternary coefficient ring
`{−1, 0, +1}` in `Z[x]/(x^n + 1)`.

This is the foundational primitive for lattice-based post-quantum cryptography
(NTRU, RLWE, Kyber-style key encapsulation) on the T81 substrate.

---

## 2. Motivation

Lattice cryptography requires polynomial multiplication in the ring
`Z[x]/(x^n + 1)` followed by coefficient reduction. When coefficients are
restricted to the ternary domain `{−1, 0, +1}`:

- **No integer multiplications are needed** — the product of two trits is
  still a trit (0 if either is 0; +1 if same sign; −1 if opposite sign).
- Only **add / sub / trit-flip** operations are used, matching the T81
  design philosophy perfectly.
- The same `T81BigInt` accumulator pattern used in RFC-0034 (TWMATMUL)
  applies, giving bit-exact, determinism-guaranteed results.

---

## 3. Background: Negacyclic Convolution

Polynomial multiplication in `Z[x]/(x^n + 1)`:

```
C[k] = Σ_{i=0}^{n-1}  A[i] · B[(k−i+n) mod n] · neg(i, k)
```

where `neg(i, k) = −1` if `i + j ≥ n` (the term wraps past degree `n`,
picking up a sign flip from the ring relation `x^n ≡ −1`), else `+1`.

---

## 4. T81Lang Surface API

### 4.1 `std.crypto.polymul`

```
std.crypto.polymul(a: Tensor, b: Tensor) -> Tensor
```

- `a`, `b`: 1-D tensors of the same length `n`, values in `{−1.0, 0.0, +1.0}`
- Returns: 1-D tensor of length `n`, the negacyclic product `A · B` in
  `Z[x]/(x^n + 1)`
- Accumulation uses `T81BigInt` (bit-exact, no overflow)
- Minimum tier: 2

### 4.2 `std.crypto.polymod`

```
std.crypto.polymod(a: Tensor, q: i32) -> Tensor
```

- `a`: 1-D tensor (output of `polymul` or any polynomial)
- `q`: positive odd integer modulus
- Returns: 1-D tensor with every coefficient `c` reduced to the centered
  range `(−q/2, q/2]`:
  ```
  c_red = ((c % q) + q) % q
  if c_red > (q-1)/2 → c_red -= q
  ```

---

## 5. TISC Opcodes

### 5.1 POLYMUL

```
POLYMUL  RD, R_A, R_B
```

| Field    | Value                                        |
|----------|----------------------------------------------|
| Opcode   | `tisc::Opcode::POLYMUL`                      |
| Operands | A = dest handle; B = poly-A handle; C = poly-B handle |
| Semantics | `RD ← polymul(*tensor(R_A), *tensor(R_B))`  |
| Trap     | `InvalidTensorHandle` if R_A or R_B invalid  |
| Tier     | 2+                                           |

### 5.2 POLYMOD

```
POLYMOD  RD, R_A, R_Q
```

| Field    | Value                                        |
|----------|----------------------------------------------|
| Opcode   | `tisc::Opcode::POLYMOD`                      |
| Operands | A = dest handle; B = poly handle; C = q (register value) |
| Semantics | `RD ← polymod(*tensor(R_A), R_Q)`           |
| Trap     | `InvalidTensorHandle` if R_A invalid; `InvalidArgument` if q ≤ 0 |
| Tier     | 2+                                           |

---

## 6. IR Opcodes

Both opcodes are added to `tisc::ir::Opcode` in `include/t81/isa/ir.hpp`:

```cpp
POLYMUL,  // Negacyclic poly multiply: POLYMUL RD, R_A, R_B
POLYMOD,  // Centered reduction mod q: POLYMOD RD, R_A, R_Q
```

---

## 7. Math Layer

`include/t81/tensor/lattice_crypto.hpp` in `t81::ops` namespace:

| Function | Signature                                              |
|----------|--------------------------------------------------------|
| `polymul` | `(const T729DynamicTensor& a, const T729DynamicTensor& b) -> T729DynamicTensor` |
| `polymod` | `(const T729DynamicTensor& a, std::int64_t q) -> T729DynamicTensor` |

Both are pure functions with no side effects, enabling deterministic replay
under CanonHash81.

---

## 8. Acceptance Criteria

| AC  | Description                                            | Status |
|-----|--------------------------------------------------------|--------|
| AC-1 | `std.crypto.polymul` registered; lowers to `POLYMUL` | ✅ |
| AC-2 | `std.crypto.polymod` registered; lowers to `POLYMOD` | ✅ |
| AC-3 | Wrong arity is a SA error                            | ✅ |
| AC-4 | `polymul(A, [1,0,…,0]) = A` (identity)               | ✅ |
| AC-5 | `x^3 · x = x^4 ≡ −1` (negacyclic wrap)              | ✅ |
| AC-6 | `polymod(A, 5)` centered reduction                   | ✅ |
| AC-7 | `polymod(A, 3)` reduces {−1,0,1,2,3,4}               | ✅ |
| AC-8 | Composition emits both `POLYMUL` and `POLYMOD`       | ✅ |

---

## 9. Files Changed

| File | Change |
|------|--------|
| `include/t81/tensor/lattice_crypto.hpp` | New — `polymul` and `polymod` math layer |
| `include/t81/isa/opcodes.hpp` | `POLYMUL`, `POLYMOD` added to `tisc::Opcode` |
| `include/t81/isa/ir.hpp` | `POLYMUL`, `POLYMOD` added to `tisc::ir::Opcode` |
| `vm/vm.cpp` | VM dispatch cases between TACT and FFICall |
| `include/t81/frontend/builtin_registry.hpp` | 2 new entries: `std.crypto.polymul`, `std.crypto.polymod` |
| `lang/frontend/ir_generator.cpp` | Custom emit handlers for `crypto_polymul`, `crypto_polymod` |
| `tests/cpp/lattice_crypto_test.cpp` | 8 ACs / 11 assertions |
| `spec/rfcs/RFC-0038-lattice-crypto.md` | This file |
| `CMakeLists.txt` | `lattice_crypto_test` target |

---

## 10. Stage 3 Roadmap

RFC-0038 is the second Stage 3 feature (after RFC-0037 TNN stdlib). Together
they establish:

- **RFC-0037**: Multiplication-free neural inference (`std.tnn.*`)
- **RFC-0038**: Multiplication-free lattice crypto (`std.crypto.polymul/polymod`)

Next potential Stage 3 items:
- **RFC-0039**: T81Lang spec promotion (normative grammar in spec/)
- **NTRU-style KEM**: Higher-level key encapsulation using RFC-0038 primitives
- **Homomorphic operations**: Leveraging ternary ring arithmetic for FHE

---

## 11. Security Notes

- **Axion policy gate**: Both opcodes are Tier 2+, subject to Axion pre-dispatch
  policy check.
- **No secret-dependent branches**: The ternary multiply uses only comparison
  against 0/sign — constant-time with respect to coefficient values in {−1,0,+1}.
- **Determinism**: T81BigInt accumulators guarantee bit-exact reproducibility
  across platforms, satisfying RFC-0002 deterministic execution contract.
