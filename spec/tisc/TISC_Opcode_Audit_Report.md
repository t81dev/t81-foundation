# TISC Opcode Gap Audit Report

## 1. Executive Recommendation

**Option B:** Freeze after adding minimal missing core opcodes.

Rationale: The current opcode set (v1.1.0) is surprisingly robust for high-level tasks (Tensor, Control Flow, Arithmetic) but exhibits a critical gap in low-level bitwise manipulation. Without these primitives, standard systems programming, cryptography, and efficient data packing are impossible or prohibitively expensive. Freezing now would lock in a deficiency that cannot be cleanly solved by libraries.

## 2. Source Reconciliation Summary

*   **Inventory Totals**: 167 defined opcodes (0 through 166).
*   **Mismatches**: `opcode-semantics.md` covers only ~121 opcodes, missing detailed semantics for 46 newer additions (e.g., specific Float math, Strings/Vectors, Enums).
*   **Missing Semantics**: Significant gaps in `opcode-semantics.md` for `FAdd` family, `FMath` functions (`FSin` etc.), `String`/`Vector` advanced ops, and `Enum` variants.
*   **Status Inconsistencies**:
    *   `NSend`, `NRecv`, `VWait`, `VYield` are marked "Implemented" in `opcode-registry.md` but are functional stubs in `vm.cpp`.
    *   `AxSign`, `AxLineage`, `AxCanon` are marked "Implemented" but are no-op logging stubs.
    *   `TNeuralBwd` is a no-op stub.

## 3. Coverage Audit by Domain

### Execution Control & Traps
*   **Status**: Complete.
*   **Coverage**: `Halt`, `Trap`, `Assert`, `AxHalt` provide sufficient control.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Control Flow
*   **Status**: Complete.
*   **Coverage**: `Jump`, `JumpIfZero`, `JumpIfNotZero`, `Call`, `Ret`, `JumpIfNegative`, `JumpIfPositive`.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Arithmetic
*   **Status**: Adequate for v1.
*   **Coverage**: Full Integer (`Add`..`Mod`) and Float (`FAdd`..`FPow`).
*   **Risk**: Medium (Float determinism relies on `T81_DETERMINISTIC` compile-time flag; implementation uses `std::` math by default).
*   **Recommended Action**: Freeze (address determinism in spec/build config).

### Comparison & Selection
*   **Status**: Complete.
*   **Coverage**: Full relational operators for Int/Float.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Memory & Data Movement
*   **Status**: Complete.
*   **Coverage**: `Load`, `Store`, `LoadImm`, `Mov`, `Push`, `Pop`, `Copy`, `MemZero`.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Type Conversion
*   **Status**: Complete.
*   **Coverage**: `I2F`, `F2I`, `I2Frac`, `Frac2I`, `F2Frac`, `Frac2F` cover all core types.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Bit/Trit Operations
*   **Status**: **Missing / Critical Gap**.
*   **Coverage**: `TAnd`, `TOr`, `TXor`, `TNot` exist but operate on *Trits* (ternary logic), not bits. There are **zero** bitwise operations for `int64` (AND, OR, XOR, NOT, SHL, SHR).
*   **Risk if Frozen Now**: **High**. Renders the ISA unsuitable for hashing, crypto, or bit-packing.
*   **Recommended Action**: **Add Core**.

### Introspection / Capability Queries
*   **Status**: Adequate.
*   **Coverage**: `MetaRead`, `MetaWrite`, `MetaReflect` provide strong introspection.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Canonicalization / Hash / Verification
*   **Status**: Adequate for v1.
*   **Coverage**: `Canon`, `SymCanon`, `TLoadHash`.
*   **Risk**: Low.
*   **Recommended Action**: Freeze.

### Tensor / AI Primitives
*   **Status**: Complete (Extension-like).
*   **Coverage**: Extensive support (`TMatMul`, `TNeuralFwd`, `TRoPE`, etc.).
*   **Risk**: Low.
*   **Recommended Action**: Freeze (Core Extension).

### Concurrency / Messaging / Sync
*   **Status**: Incomplete (Stubs).
*   **Coverage**: `NSend`, `NRecv` are stubs.
*   **Risk**: Low (Can be deferred to v2 or later extensions).
*   **Recommended Action**: Defer (keep stubs).

## 4. Proposed Additions and Classification

| Proposal | Class | Reason | Determinism Impact | Urgency |
| :--- | :--- | :--- | :--- | :--- |
| `BitAnd` | Core | Essential for bit masking/flags. | None (Deterministic) | Now |
| `BitOr` | Core | Essential for bit setting. | None (Deterministic) | Now |
| `BitXor` | Core | Essential for crypto/toggles. | None (Deterministic) | Now |
| `BitNot` | Core | Essential for bit inversion. | None (Deterministic) | Now |
| `BitShl` | Core | Essential for packing/math. | None (Deterministic) | Now |
| `BitShr` | Core | Arithmetic shift right (preserves sign). | None (Deterministic) | Now |
| `BitUShr` | Core | Logical shift right (zero fill). | None (Deterministic) | Now |
| `RotL` / `RotR` | Extension | Useful for crypto, but decomposable. | None | Defer |
| `PopCount` | Extension | Useful for analysis, decomposable. | None | Defer |

## 5. Minimum Add-Before-Freeze Set

1.  `BitAnd` (A = B & C)
2.  `BitOr` (A = B | C)
3.  `BitXor` (A = B ^ C)
4.  `BitNot` (A = ~B)
5.  `BitShl` (A = B << C)
6.  `BitShr` (A = B >> C, arithmetic)
7.  `BitUShr` (A = B >>> C, logical)

## 6. Deferred / Keep Out of ISA

*   **Advanced Network Ops (`NSend`, `NRecv` real impl)**: Keep as stubs. The VM contract for external networking is not yet mature enough for v1 core.
*   **General Purpose Hash (`Sha3`)**: Keep out. Rely on `TLoadHash` for loading content-addressed data or library implementations for hashing data in registers (once Bitwise ops exist).
*   **Complex Math (`Gamma`, `Erf`)**: Keep out. Implement in standard library.

## 7. Spec Follow-up Tasks

*   [ ] **Update `opcode-registry.md`**: Add the 7 Bitwise opcodes to the `Logic` or `Arithmetic` category (assigned to reserved range `0xA7`-`0xAD`).
*   [ ] **Update `opcode-semantics.md`**:
    *   Add rows for the 7 new Bitwise opcodes.
    *   Add rows for the ~46 missing opcodes identified in the unified reference (Float math, String/Vector ops).
*   [ ] **Update `opcode-unified-reference.md`**: Reflect the new additions and reconcile semantics.
*   [ ] **Clarify Float Determinism**: Add explicit note in `opcode-semantics.md` that Float ops rely on `T81_DETERMINISTIC` build flags for bit-exactness.
