# TISC Opcode Freeze PR Plan

## 1. Decision Baseline

**Recommendation:** Option B (Freeze after adding minimal missing core opcodes).

**Scope:**
- **Add:** 7 Bitwise instructions (`BitAnd`, `BitOr`, `BitXor`, `BitNot`, `BitShl`, `BitShr`, `BitUShr`) to the Core ISA to resolve critical low-level gaps.
- **Reconcile:** Document semantics for ~46 existing opcodes currently missing from `opcode-semantics.md`.
- **Freeze:** Lock the v1.1.0 ISA (167 existing + 7 new = 174 total opcodes) as Canonical.
- **Defer:** All other extensions (Rotates, PopCount, Advanced Math) and network/async implementations (keep as stubs).

## 2. PR Strategy

This sequence minimizes risk and separates "fix what we have" from "add what is missing".

*   **PR 1: Spec Reconciliation (Docs Only)**
    *   **Goal:** Bring `opcode-semantics.md` to parity with the implemented registry.
    *   **Why:** ~46 opcodes exist in code/registry but have no semantic specs. Fixing this first ensures the baseline is clean before adding new features.
    *   **Content:** Add missing sections for Float Math, String/Vector, Enums, and Tier 3-5 opcodes.

*   **PR 2: Core Bitwise Opcode Additions (Spec + Code)**
    *   **Goal:** Implement the 7 critical Bitwise opcodes.
    *   **Why:** Essential for systems programming/crypto; cannot be efficiently polyfilled.
    *   **Content:**
        *   Update Registry (`0xA7`-`0xAD`).
        *   Update Semantics (Bitwise logic, shift masking).
        *   C++ Implementation (`vm.cpp`, `opcodes.hpp`).
        *   New Test Suite (`tests/cpp/test_bitwise.cpp`).

*   **PR 3: Freeze & Roadmap (Docs Only)**
    *   **Goal:** Formally mark the ISA as "Frozen/Canonical".
    *   **Why:** Signals stability to toolchain developers.
    *   **Content:**
        *   Update Status fields to "Canonical".
        *   Add "Deferred Extensions" section to `opcode-unified-reference.md`.
        *   Add explicit "Implementation Status" notes for stubs (Net, Axion).

## 3. File-by-File Edit Plan

### 3.1 `spec/tisc/opcode-registry.md`

*   [ ] **Add Rows:** Insert the following 7 rows at the end of the table (indices 167-173):
    *   `BitAnd` (0xA7)
    *   `BitOr` (0xA8)
    *   `BitXor` (0xA9)
    *   `BitNot` (0xAA)
    *   `BitShl` (0xAB)
    *   `BitShr` (0xAC)
    *   `BitUShr` (0xAD)
*   [ ] **Update Status:** Change "Status: Active" to "Status: Canonical / Frozen (v1.1.0)".
*   [ ] **Reserved Range:** Update reserved note to "Opcodes 174 (0xAE) through 255 (0xFF) are reserved".
*   [ ] **Implementation Location:** Ensure all 174 opcodes point to `vm/vm.cpp`.

### 3.2 `spec/tisc/opcode-semantics.md`

*   [ ] **Add Section:** `## Bitwise Operations`
    *   Define `BitAnd`, `BitOr`, `BitXor`, `BitNot` (standard 64-bit integer logic).
    *   Define `BitShl`, `BitShr` (Arithmetic), `BitUShr` (Logical).
    *   **Critical Detail:** Specify that shift amounts are masked by `0x3F` (AND 63) to ensure deterministic behavior for out-of-range shifts.
*   [ ] **Add Missing Sections:**
    *   `## Float Math` (FExp, FLog, FPow, etc.) - note reliance on `T81_DETERMINISTIC`.
    *   `## String & Vector Operations` (StrLen, StrConcat, VecPush, etc.).
    *   `## Enums & Options` (MakeOptionSome, EnumIsVariant, etc.).
    *   `## Cognitive Tier 3-5` (Recurse, Gossip, InfSeed).
*   [ ] **Clarify Traps:** Add "Traps: DecodeFault, TypeFault" to relevant new sections.

### 3.3 `spec/tisc/opcode-unified-reference.md`

*   [ ] **Regenerate Table:** Update the master table to include the 7 new Bitwise opcodes.
*   [ ] **Status Column:** Mark all Core ops as "Frozen".
*   [ ] **Stub Note:** Add a column or footnote indicating `NSend`, `NRecv`, `AxSign` are "Functional Stubs".

## 4. Core Opcode Change Tasks (if applicable)

| Opcode | Spec Work | VM Work (`vm/vm.cpp`) | Tests | Determinism Notes | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `BitAnd` | Add to Registry/Semantics | Add `Opcode::BitAnd` case | `test_bitwise.cpp` | Pure integer logic | Pending |
| `BitOr` | Add to Registry/Semantics | Add `Opcode::BitOr` case | `test_bitwise.cpp` | Pure integer logic | Pending |
| `BitXor` | Add to Registry/Semantics | Add `Opcode::BitXor` case | `test_bitwise.cpp` | Pure integer logic | Pending |
| `BitNot` | Add to Registry/Semantics | Add `Opcode::BitNot` case | `test_bitwise.cpp` | Pure integer logic | Pending |
| `BitShl` | Add to Registry/Semantics | Add `Opcode::BitShl` case | `test_bitwise.cpp` | **Mask shift amt (B & 0x3F)** | Pending |
| `BitShr` | Add to Registry/Semantics | Add `Opcode::BitShr` case | `test_bitwise.cpp` | **Mask shift amt (B & 0x3F)** | Pending |
| `BitUShr`| Add to Registry/Semantics | Add `Opcode::BitUShr` case| `test_bitwise.cpp` | **Mask shift amt (B & 0x3F)** | Pending |

**Implementation Notes:**
*   Update `include/t81/isa/opcodes.hpp` enum.
*   Update `core/isa/encoding.cpp` string-to-opcode map (if applicable).
*   Ensure `T81_DETERMINISTIC` compliance is irrelevant here (integer only), but verify no undefined behavior on shifts.

## 5. Deferred Extensions and Non-ISA Items

**Deferred (Candidate Extensions):**
*   `RotL` / `RotR` (Bitwise Rotate)
*   `PopCount` (Population Count)
*   `Sha3` (Opcode for hashing - rely on library or `TLoadHash` for now)

**Library / Runtime Only (No Opcode):**
*   Complex Math (`Gamma`, `Erf`) - Implement in `std.math` library.
*   Filesystem IO (beyond `TLoadHash`) - Host function / Syscall boundary.

## 6. Freeze Gate Checklist

*   [ ] `opcode-registry.md` contains exactly 174 opcodes.
*   [ ] `opcode-semantics.md` covers all 174 opcodes.
*   [ ] `vm.cpp` implements all 174 opcodes (no missing switch cases).
*   [ ] New Bitwise opcodes pass unit tests (positive, negative, edge cases).
*   [ ] Shift operations verified for determinism (masking).
*   [ ] No "Active" or "Draft" status remains in Spec; all are "Canonical" or "Reserved".
*   [ ] Stubs (`NSend`, etc.) are clearly documented as such.

## 7. Risks / Open Questions

*   **Shift Determinism:** Confirmed masking (0x3F) approach? (Assumed YES for this plan).
*   **Register Width:** Confirmed 64-bit integer registers in `vm.cpp`.
