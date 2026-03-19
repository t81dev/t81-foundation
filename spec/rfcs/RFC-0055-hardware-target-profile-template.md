# RFC-0055 Hardware Target Profile Template

**Status:** draft
**Type:** hardware-profile
**Applies-To:** [target device or family]
**Created:** [YYYY-MM-DD]
**Updated:** [YYYY-MM-DD]

---

## 1. Vendor and Target Identity

- **Vendor Name:** [Vendor Name]
- **Target Family:** [e.g., TernaryCompute-X1]
- **Target Name/SKU:** [Specific Model or Range]
- **ISA Name:** [e.g., TISA-v1, TISC-Compatible, Proprietary-X]
- **ISA Revision/Version:** [e.g., 1.0.4]
- **Integration Mode:** [Direct TISC / Verified Lowering / Hosted Compatibility]

## 2. Execution and Semantic Model

- **TISC Compatibility:** [Does the hardware execute TISC directly, or does it require lowering?]
- **Lowering Boundary:** [If lowering is required, what software layer is responsible?]
- **Unsupported TISC Features:**
  - [List any TISC opcodes or features that are NOT supported natively]
  - [Describe the fallback or trap behavior for unsupported features]

## 3. Boot and Privilege Rules

- **Boot Entry Format:** [How does the T81 runtime start on this hardware?]
- **Privilege Levels:** [e.g., Ring 0/3 equivalent, Secure/Non-Secure, or Flat]
- **Kernel/User Boundary:** [How is the governed surface protected from userland?]
- **Ethics-First Boot Gating:** [How is the initial Axion policy evaluated before execution?]

## 4. Memory and Addressing Contract

- **Address Space Model:** [Physical / Virtual / Both]
- **Word/Trit Granularity:** [e.g., 81-trit words, 243-trit vectors]
- **Page Size/Alignment:** [e.g., 3^9 trits]
- **Endianness/Packing Rules:** [Canonical representation of ternary values in memory]
- **DMA and Device Memory Rules:** [How are device buffers mapped and protected?]

## 5. Interrupt and Device Semantics

- **Interrupt Translation:** [How are hardware interrupts mapped to T81/Axion governed events?]
- **Trap Classification:** [How are synchronous faults mapped to `DecodeFault`, `MemoryFault`, etc.?]
- **Unhandled Interrupt Behavior:** [Is determinism preserved if an interrupt handler is missing?]

## 6. Trace, Audit, and CanonFS Preservation

- **Canonical Trace Semantics:** [Are TISC step traces perfectly preserved?]
- **Axion Audit Hook Firing:** [Do hardware actions reliably trigger all required Axion policies?]
- **CanonFS Object Identity:** [Are deterministic hashes and object identities preserved without silent mutation?]

## 7. Equivalence and Conformance Claims

- **Backend Equivalence (RFC-0042):** [Has the lowering layer been mathematically or empirically proven against the scalar oracle?]
- **Conformance Matrix (RFC-0043):** [Link to CI/Validation report proving 100% pass on the bounded conformance corpus]
- **Memory Ordering (RFC-0045/0046):** [Does the hardware provide the required deterministic visibility guarantees?]

## 8. Experimental / Promotion Status

- **Current Status:** [Experimental / Governed Non-DCP / DCP-Eligible]
- **Justification for Promotion:** [If requesting promotion beyond Experimental, summarize why all obligations are met]
- **Documentation Link:** [URL to public vendor documentation distinguishing supported vs. unsupported behavior]
