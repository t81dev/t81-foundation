# Architecture Decision Record — Accountable Computation Runtime Strategy

**Directory:** `experiments/durable_witness/`
**Status:** Decided
**Decision:** **D — Existing solution sufficient (Conventional execution with host capabilities is sufficient)**

---

## Context & Problem Statement

Prior research in T81 Foundation explored whether a custom execution architecture (TISC ISA, Axion policy engine, CanonFS filesystem, ternary-inspired VM) was required to achieve accountable computation—establishing that a protected side effect occurred under valid authority and cannot be falsely replayed.

PR #554 demonstrated that without an independently trusted transactional witness, neither T81VM nor Wasmtime could prevent replay attacks across process restarts or detect fabricated producer evidence.

This experiment evaluated whether adding a durable transactional witness allows T81 to demonstrate any security or accountability property that a conventional runtime (Wasmtime) with host capabilities cannot achieve as effectively.

---

## Evaluated Options

* **Option A — Validated improvement in T81VM**: Claiming custom VM instructions provide a unique accountability property.
* **Option B — Partial improvement**: Claiming partial security gains while essential properties remain unresolved.
* **Option C — Falsified hypothesis**: The proposed witness mechanism fails to stop replay or fabrication.
* **Option D — Existing solution sufficient**: Conventional infrastructure (Wasmtime + host capabilities + transactional witness) provides identical guarantees without T81-specific VM advantages.

---

## Decision Outcome

**Decision: D — Existing solution sufficient**

### Rationale:
1. **Security Equivalence**:
   When coupled with the durable witness (`witness.py`), Wasmtime (Arm B) blocked 10 out of 10 adversarial attacks (100% security parity with T81VM Arm A).
2. **TCB Complexity Reduction**:
   Using Wasmtime with a host capability eliminates 36,600+ lines of custom C++ virtual machine, interpreter, opcode decoding, and filesystem code from the Trusted Computing Base (TCB).
3. **Cross-Platform Portability**:
   Arm A suffered severe storage failures on Windows (100 out of 110 trials failed due to CanonFS Base81 reserved filename characters `<` and `>`). Wasmtime with standard content-addressed files or SQLite storage runs portably across Linux, macOS, and Windows.
4. **Decoupling Recommendation**:
   The research contribution of accountable computation (transactional authority & effect commitment with cryptographic receipts) is valid and valuable, but it is **completely independent of the custom TISC execution surface**. It should be separated from T81VM and packaged as a reusable host capability for standard WebAssembly runtimes.

---

## Consequences & Recommendations

* **Do not merge PR #554** as a justification for keeping custom execution architecture for accountable side effects.
* **Freeze further TISC ISA additions** for accountable computation.
* **Promote Option C from ADR research options**: Treat accountable evidence as a decoupled host capability / transactional protocol usable with conventional WebAssembly or container runtimes.
