# RFC-0039 — NTRU-KEM: Ternary Key Encapsulation Mechanism

**Status:** accepted
**Depends-on:** RFC-0038 (POLYMUL / POLYMOD), RFC-0034 (TVecAdd)
**Authors:** T81 Foundation
**Date:** 2026-03-16

---

## 1. Overview

RFC-0039 introduces **NTRU-KEM**, a simplified Key Encapsulation Mechanism built
over the ternary negacyclic ring `Z[x]/(xⁿ + 1)`.  It completes the polynomial
ring arithmetic surface started in RFC-0038 by adding the missing subtraction
primitive (`TVecSub`) and providing three high-level T81Lang builtins:

| Builtin                      | Lowers to                        | Description                          |
|------------------------------|----------------------------------|--------------------------------------|
| `std.crypto.polyadd(a, b)`   | `TVECADD`                        | Polynomial addition (reuses RFC-0005)|
| `std.crypto.polysub(a, b)`   | `TVECSUB` (new)                  | Polynomial subtraction               |
| `std.crypto.ntru_encrypt(h, msg, r, q)` | `POLYMUL`+`TVECADD`+`POLYMOD` | Encrypt one polynomial message |
| `std.crypto.ntru_decrypt(f, c, p)`      | `POLYMUL`+`POLYMOD`           | Recover message from ciphertext|

Key generation (`ntru_keygen`) is provided as a C++ host-side function in
`include/t81/tensor/ntru_kem.hpp` only; it is not a T81Lang builtin because it
requires an entropy source, which is not available inside deterministic TISC
programs.

---

## 2. Motivation

RFC-0038 added negacyclic polynomial multiply (`POLYMUL`) and centered modular
reduction (`POLYMOD`).  The complete polynomial ring `Z[x]/(xⁿ + 1)` requires
addition, subtraction, multiplication, and reduction.  Addition was already
available via the `TVecAdd` opcode from RFC-0005.  Subtraction was absent,
making the ring incomplete and leaving `ntru_encrypt` / `ntru_decrypt` sequences
un-expressible in T81Lang source.

---

## 3. T81Lang Grammar

The four new builtins appear in the `std.crypto` namespace and follow the same
grammar as all stdlib calls (RFC-0011 §3.1):

```text
call_expr ::= "std" "." "crypto" "." crypto_fn "(" arg_list ")"

crypto_fn ::=
    "polyadd"      "(" poly_expr "," poly_expr ")"         -> Tensor
  | "polysub"      "(" poly_expr "," poly_expr ")"         -> Tensor
  | "ntru_encrypt" "(" poly_expr "," poly_expr "," poly_expr "," int_expr ")" -> Tensor
  | "ntru_decrypt" "(" poly_expr "," poly_expr "," int_expr ")"               -> Tensor
```

All `poly_expr` arguments must resolve to `Tensor` (1-D, any length).
The integer arguments (`q`, `p`) must resolve to `i32`.
The return type of all four is `Tensor`.

---

## 4. Semantics

### 4.1 `std.crypto.polyadd(a, b) → Tensor`

Coefficient-wise addition:  `C[k] = A[k] + B[k]`  for all `k ∈ [0, n)`.

No ring reduction; coefficients may exceed `{−1, 0, +1}`.  Use `polymod` if
bounded coefficients are required.

Lowers to the `TVecAdd` opcode (RFC-0005); shares the `tensor_vec_binary_checked`
helper.  Requires `shape(A) == shape(B)`.

### 4.2 `std.crypto.polysub(a, b) → Tensor`

Coefficient-wise subtraction:  `C[k] = A[k] − B[k]`  for all `k ∈ [0, n)`.

Lowers to the new `TVecSub` opcode (§5.1).  Requires `shape(A) == shape(B)`.

### 4.3 `std.crypto.ntru_encrypt(h, msg, r, q) → Tensor`

NTRU-style polynomial encryption:

```
c = polymod( polyadd( polymul(h, r) , msg ) , q )
```

Expanded lowering sequence:

```
POLYMUL  t1, h, r       ; h·r in Z[x]/(xⁿ+1)
TVECADD  t2, t1, msg    ; h·r + msg
POLYMOD  dest, t2, q    ; centered reduction mod q
```

`h` is the public key, `msg` is a ternary message polynomial, `r` is a
random ternary blinding polynomial, and `q` is an odd positive modulus.

### 4.4 `std.crypto.ntru_decrypt(f, c, p) → Tensor`

NTRU-style polynomial decryption:

```
m' = polymod( polymul(f, c) , p )
```

Expanded lowering sequence:

```
POLYMUL  t1, f, c       ; f·c in Z[x]/(xⁿ+1)
POLYMOD  dest, t1, p    ; centered reduction mod p
```

`f` is the secret key polynomial and `p` is the small modulus.

---

## 5. New TISC Opcode: TVecSub

**Opcode byte:** 212 (0xD4)
**Mnemonic:** `TVecSub`
**Encoding:** `TVecSub RD, RA, RB`

Performs element-wise floating-point subtraction of two tensor handles:

```
out[k] = tensor(RA)[k] - tensor(RB)[k]   for all k
```

Shape requirements: `shape(RA) == shape(RB)` (after NumPy-style broadcast
check via `tensor_elementwise_compatible`).  If shapes are incompatible, the VM
raises `ShapeFault`.

**VM dispatch:** delegates to `tensor_vec_sub_checked(RA, RB)` which calls
`t81::ops::sub(lhs, rhs)` (defined in `include/t81/tensor/elementwise.hpp`).

---

## 6. C++ Math Layer (`include/t81/tensor/ntru_kem.hpp`)

### 6.1 `ntru_detail::make_ternary_poly(n, seed)` → `T729DynamicTensor`

Generates a deterministic ternary polynomial of length `n` using a 64-bit
xorshift PRNG seeded with `seed`.  Coefficients are in `{−1, 0, +1}` mapped
from `{0, 1, 2}` via `t − 1`.

### 6.2 `NtruKeyPair`

```cpp
struct NtruKeyPair {
    T729DynamicTensor f;   // secret key  (ternary, degree n−1)
    T729DynamicTensor h;   // public key  h = polymod(polymul(f, g), q)
};
```

### 6.3 `ntru_keygen(n, q, seed_f, seed_g)` → `NtruKeyPair`

Generates a key pair.  Default seeds: `seed_f = 42`, `seed_g = 137`.

```
f = make_ternary_poly(n, seed_f)
g = make_ternary_poly(n, seed_g)
h = polymod(polymul(f, g), q)
```

**Note:** In standard NTRU, `h = g · f⁻¹ mod q`.  This implementation uses
`h = f · g mod q` (no inversion) for pedagogical clarity.  Correct round-trip
is demonstrated with the identity-key test (AC-9).

### 6.4 `ntru_encrypt(h, msg, r, q)` → `T729DynamicTensor`

C++ implementation of the encrypt operation (§4.3).  Uses `t81::ops::polymul`,
`t81::ops::add`, and `t81::ops::polymod`.

### 6.5 `ntru_decrypt(f, c, p)` → `T729DynamicTensor`

C++ implementation of the decrypt operation (§4.4).  Uses `t81::ops::polymul`
and `t81::ops::polymod`.

---

## 7. Opcode Registry

Updated entries in `spec/tisc/opcode-registry.md` §2.22:

| Byte | Hex   | Mnemonic  | RFC     | Description                         |
|------|-------|-----------|---------|-------------------------------------|
| 212  | 0xD4  | TVecSub   | RFC-0039| Elementwise tensor subtraction       |

Reserved range updated: `0xD5–0xFF`.

---

## 8. Acceptance Criteria

| ID    | Criterion                                                            |
|-------|----------------------------------------------------------------------|
| AC-1  | `std.crypto.polyadd` registered; lowers to `TVECADD`                |
| AC-2  | `std.crypto.polysub` registered; lowers to `TVECSUB`                |
| AC-3  | `std.crypto.ntru_encrypt` lowers to `POLYMUL`+`TVECADD`+`POLYMOD`   |
| AC-4  | `std.crypto.ntru_decrypt` lowers to `POLYMUL`+`POLYMOD`             |
| AC-5  | Wrong arity (`polyadd` with 1 arg) is a semantic-analysis error      |
| AC-6  | `polyadd` math: `[1,−1,0] + [0,1,1] = [1,0,1]`                      |
| AC-7  | `polysub` math: `[1,1,0] − [0,1,1] = [1,0,−1]`                      |
| AC-8  | `ntru_encrypt` output has length `n`                                 |
| AC-9  | Identity-key round-trip: `decrypt(f, encrypt(h, msg, r, q), q) = msg`|
| AC-10 | `make_ternary_poly` is deterministic and produces values in `{−1,0,1}`|

---

## 9. Security Notes

This implementation is **pedagogical only**.  It is not suitable for production
cryptographic use because:

1. **No modular inverse** — the standard NTRU public key requires `h = g · f⁻¹ mod q`.
   Computing `f⁻¹` in `Z[x]/(xⁿ+1)` mod `q` requires an extended Euclidean
   algorithm for polynomials, which is not implemented.

2. **No constant-time guarantees** — the inner loops branch on ternary values
   (`snap_trit`, `trit_mul`), which may leak timing information.

3. **No parameter validation for NTRU security levels** — the choice of `n`, `q`,
   and key distributions must follow established NTRU parameter sets (e.g.,
   NTRU-HPS-2048-509) for meaningful security.

For post-quantum security in production, follow the NIST PQC standards
(CRYSTALS-Kyber / FIPS 203 for KEM; CRYSTALS-Dilithium / FIPS 204 for DSA).

---

## 10. Changelog

| Date       | Change                                    |
|------------|-------------------------------------------|
| 2026-03-16 | Initial accepted version                  |
