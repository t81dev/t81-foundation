# RFC: Tritwise Operations Extension Investigation (TISC)

## 1. Status

*   **Status:** **Closed / Not Adopted**
*   **Resolution:** Library-level acceleration is sufficient; ISA extension not justified at this time.
*   **Evidence:** See [Tritwise Extension Gate Evidence](../../../docs/process/rfcs/rfc-tritwise-extension-gate-evidence.md).
*   **Core ISA Note:** The TISC v1.1.0 core ISA remains frozen and unchanged. This document explores potential *future* extensions or library patterns and does not propose modifications to the existing canonical opcode set.
*   **Audit Update:** An audit of existing packed representations has been completed. See [Packed-Trit Representation Audit and Comparison](packed-trit-representation-audit-and-comparison.md).

## 2. Motivation

With the recent freezing of the TISC v1.1.0 core ISA, which includes a comprehensive set of binary bitwise operations (`BitAnd`, `BitOr`, etc.) for system-level integer manipulation, a specific gap in the T81 architecture has become apparent. While the core ISA supports scalar ternary logic (`TAnd`, `TOr`, `TXor`, `TNot`) for individual trit values (typically represented as integers `-1`, `0`, `1`), there is no native facility for **packed** or **lane-wise** ternary data manipulation ("tritwise" operations).

As T81 workloads evolve towards more complex ternary neural networks and symbolic processing, the need to manipulate dense arrays or packed representations of trits—analogous to how bitwise operations manipulate packed bits—warrants a disciplined investigation. This RFC aims to explore this design space without prematurely expanding the ISA.

## 3. Problem Statement

Current TISC opcodes cover two distinct domains:
1.  **Scalar Integer/Binary Domain:** Handled by standard arithmetic and the newly added binary bitwise ops (e.g., `BitAnd` on `int64`).
2.  **Scalar Logical Ternary Domain:** Handled by `TAnd`, `TOr`, `TXor`, `TNot`, which operate on individual register values interpreted as logical trits.

**The Gap:** There is no efficient mechanism to perform parallel or packed operations on ternary data. For example, applying a ternary mask to a packed "tryte" buffer, or performing a lane-wise `TAnd` across a tensor slice, currently requires unpacking to scalar registers, executing a loop of scalar `TAnd` instructions, and repacking. This is computationally expensive and semantically obscure.

Library-based solutions (e.g., software functions) can fill this gap but may suffer from performance overheads and lack a standardized canonical representation for packed ternary data, leading to ecosystem fragmentation.

## 4. Terminology and Domain Separation

To avoid confusion, we must strictly define the following terms:

*   **Bitwise (Binary Integer):** Operations on standard 64-bit integers treating them as vectors of 64 bits.
    *   *Examples:* `BitAnd` (`&`), `BitOr` (`|`), `BitXor` (`^`).
    *   *Domain:* `int64_t`.
    *   *Status:* **Core ISA (Frozen).**

*   **Ternary Logic (Scalar/Logical):** Operations on individual values representing a single logical trit (`-1`, `0`, `1`).
    *   *Examples:* `TAnd` (Min), `TOr` (Max), `TXor` (See canonical semantics in `opcode-semantics.md`).
    *   *Domain:* Scalar registers (conceptually `trit`).
    *   *Status:* **Core ISA (Frozen).**

*   **Tritwise (Packed/Lane-wise):** Proposed operations on data structures containing multiple trits, performing logic in parallel (SIMD-like) or on a packed representation.
    *   *Examples (Hypothetical):* `TritAnd` (lane-wise min), `TritShift` (tryte-wise shift), `TritMask`.
    *   *Domain:* Packed Trytes, Tensor Lanes, or specialized `TritVector` types.
    *   *Status:* **Investigation / Extension Candidate.**

**Explicit Warning:** Do not confuse "Bitwise AND on a ternary value" (which is implementation-defined garbage) with "Tritwise AND" (which is semantic intersection).

## 5. Candidate Data Domains for Tritwise Operations

Before defining opcodes, we must determine *what* they operate on.

### 5.1 Fixed-Width Packed Trit Words ("Trytes")
*   **Concept:** A 64-bit register could theoretically hold a packed sequence of trits (e.g., using 2-bit encoding or balanced ternary encoding).
*   **Pros:** Fits in existing `R[...]` registers; distinct from memory blobs.
*   **Cons:** Enforcing valid encoding in general-purpose registers is difficult; unpacking overhead; endianness of trits within a word.
*   **Determinism:** Requires strict canonicalization (e.g., "unused" bits must be zeroed).

### 5.2 Dynamic Trit Vectors (Tensor Lanes)
*   **Concept:** Operations apply to `TensorHandle` or a new `TritVectorHandle`.
*   **Pros:** Aligns with T81's tensor-heavy workload; arbitrary width; SIMD-friendly.
*   **Cons:** Heavyweight for small masks; implies memory traffic rather than register logic.
*   **Determinism:** Easier to enforce via tensor metadata.

### 5.3 Ternary Cell Arrays (Hardware Abstraction)
*   **Concept:** Direct mapping to potential ternary hardware accelerators (e.g., Memristive arrays).
*   **Pros:** High performance potential.
*   **Cons:** Extremely speculative; software fallback would be slow; might break portability if not carefully specified.

## 6. Use Cases and Workload Evidence

We must gather evidence for the following workloads before justifying ISA extensions:

1.  **Ternary Masks / Pattern Matching:** efficiently masking out "unknown" (`0`) values or selecting specific paths in a symbolic graph based on a packed configuration.
2.  **Canonicalization Pipelines:** Rapidly normalizing raw sensor inputs into canonical ternary states (e.g., clamping and aligning).
3.  **Packed Ternary Codecs:** Compression/Decompression of sparse ternary tensors for storage or network transmission (`NSend`).
4.  **Symbolic State Transitions:** Large cellular automata or grid-based simulations where state is natively ternary and dense.

### 6.1 Cases Better Served by Library/Runtime
*   **Sparse logical checks:** If checking one flag, use scalar `TAnd`.
*   **Low-frequency formatting:** formatting a string of trits for `PRINT`.
*   **One-off math:** calculating a checksum once per frame.

## 7. Semantic Design Questions (Must Be Resolved Before Any Opcode Proposal)

1.  **Operand Type:** Do tritwise ops work on `int64` (reinterpreted as packed trits) or a dedicated `TritHandle`?
    *   *Impact:* Reinterpreting `int` creates "trap representations" if the bit-pattern isn't valid ternary.
2.  **Lane Width / Dynamic Length:** Is there a fixed "word size" (e.g., 27 trits) or is it length-agnostic?
3.  **Truth Tables:** Do we strictly follow Kleene Logic (Strong logic of indeterminacy) or a different ternary calculus?
4.  **Neutral/Unknown Handling:** How do ops handle "don't care" states vs explicit `0` (Unknown)?
5.  **Mismatched Widths:** If operating on vectors, what happens on length mismatch? Trap? Pad with `0`? Cyclic repeat?
6.  **Encoding Dependence (Representation Invariance):** Semantics must be defined on logical trits, not the underlying bit-packing. Does `TritAnd` behave identically whether the backing store is 2-bit encoded, balanced-ternary, or unpacked? (It must).
7.  **Determinism:** Can we guarantee bit-exact results across different backing implementations (e.g., FPGA vs CPU emulation)?

## 8. Candidate Solution Spaces

### A. Library/Runtime-Only Tritwise Utilities
*   **Description:** Implement `T81::Tritwise::And(a, b)` in the standard library.
*   **Pros:** No ISA changes; immediate availability; easy to version/fix.
*   **Cons:** Function call overhead (`CALL`/`RET`); inability to leverage potential CPU custom instructions efficiently.
*   **Recommendation:** **Start Here.**

### B. TISC Extension Opcodes (Optional Extension Set)
*   **Description:** Define a block of opcodes (e.g., `0xB0`–`0xBF`) specifically for packed trit ops, available only if the VM advertises the extension.
*   **Pros:** High performance; dense encoding; standardizes the semantics.
*   **Cons:** Fragmented ecosystem (code might not run everywhere); consumes constrained opcode encoding space.
*   **Requirement:** Any extension must include a software-only fallback path for VMs that do not implement the extension.

### C. Accelerator / Hardware-Specific Intrinsics
*   **Description:** Opcodes that map directly to hardware functions, potentially trapping to software emulation if absent.
*   **Pros:** Maximum speed on specialized hardware.
*   **Cons:** High implementation complexity for the VM; potential for "semantic drift" between hardware and emulator.

### D. Hybrid Model (Library API + Optional Acceleration)
*   **Description:** The library function `T81::Tritwise::...` detects available extensions/hardware at runtime and dispatches the optimal path.
*   **Pros:** Best of both worlds; keeps ISA clean while enabling speed.
*   **Cons:** Library complexity.

## 9. Preliminary Gating Criteria for Future Adoption

To move from "Investigation" to "Extension Proposal," the following must be demonstrated:

1.  **Workload Hotspot:** Profiling must show that >5% of cycles in a key target workload (e.g., Large Ternary Model inference) are spent in inefficient scalar ternary emulation.
2.  **Structural Burden:** Evidence that library-only implementations introduce excessive code size, complexity, or duplication that an opcode would eliminate.
3.  **Ambiguity Reduction:** Evidence that developers are implementing conflicting packed representations, creating interoperability silos.
4.  **Stable Semantics:** A prototype library implementation has stabilized and seen adoption.
5.  **Deterministic Conformance:** Proof that the proposed operations can be implemented bit-exactly on standard x64/ARM64 without prohibitive cost.
6.  **Discoverability:** A defined mechanism for the VM to query support for this extension.

**Core ISA inclusion is not the default path.** We prefer keeping the Core small and freezing it.

## 10. Non-Goals

*   **This RFC does NOT:**
    *   Modify the frozen TISC v1.1.0 Core ISA.
    *   Allocate any opcode values (the examples above are hypothetical).
    *   Define the final truth tables for tritwise operations.
    *   Mandate any specific hardware support.
    *   Replace or deprecate `TAnd`, `TOr`, etc.

## 11. Open Questions

*   **Q1:** What is the canonical "packed trit" format for T81? (2-bit pairs? Base-3 integer encoding? Compressed sparse row?)
*   **Q2:** Should tritwise operations apply to `Tensor` objects (rank-agnostic) or only 1D vectors?
*   **Q3:** Is there a need for "Trit Shift" (logical/arithmetic) or "Trit Rotate"?
*   **Q4:** How do these operations interact with Axion policy guards (e.g., checking "taint" on a per-trit basis)?

## 12. Recommended Next Steps

1.  **Review Representation Audit:** Consult [Packed-Trit Representation Audit and Comparison](packed-trit-representation-audit-and-comparison.md) for an analysis of existing PT-5 and Base-81 codecs.
2.  **Prototype Library:** Implement `lib/tritwise.t81` (or C++ native module) providing `PackedTrit` helpers.
3.  **Collect Evidence:** Instrument standard T81 benchmarks to look for patterns resembling manual packed-trit manipulation.
4.  **Draft Semantics:** If demand exists, draft a precise semantic spec for `TritAnd`/`TritOr` on the chosen data domain.

## 13. Appendix (Optional)

*TBD: Placeholder for Truth Tables of packed operations once domain is selected.*
