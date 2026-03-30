# T81 Architecture — Governed Deterministic Ternary Runtime

## 1. Overview

T81 is a deterministic, policy-gated runtime for auditable AI inference. It operates as a ternary-native execution model built atop binary hardware substrates. The system is designed to provide exact bit-for-bit reproducible execution traces, enforcing governance and policy *before* side effects occur, and treating execution artifacts as immutable, content-addressed records.

While T81 includes an in-progress guest operating system and bare-metal kernel effort (TernaryOS), its current, most stable realization is as a specialized runtime environment. The architecture is defined by its rigorous constraints: it abandons broad binary compatibility and host-dependent undefined behavior in favor of an auditable, ternary-semantic, cryptographically anchored execution envelope.

## 2. Architectural Thesis

The architectural thesis of T81 is that **if an AI system cannot be reproduced, governed, and audited, it should not be trusted to act.**

T81 asserts that determinism and governance cannot be bolted onto an existing operating system or execution engine as an afterthought; they must be properties of the instruction set architecture (ISA) and the runtime itself. T81's thesis is that by shifting execution into a canonical, balanced-ternary data domain and gating every privileged operation through a fail-closed policy engine (Axion) before execution, we can guarantee the provenance and exact semantic behavior of complex AI inference and cognitive algorithms.

Determinism in T81 is load-bearing. Without determinism, the cryptographic guarantees of Axion policy verdicts and CanonFS trace hashes disintegrate. Reproducibility is not merely a debugging convenience—it is the constitutional foundation of the system's security and governance models.

## 3. Layer Stack

T81 is structured as a strict hierarchy of semantic layers. Higher layers depend entirely on the deterministic contracts provided by lower layers.

```text
+-------------------------------------------------------------+
|               Cognitive Tiers (Experimental)                |
|  (Tier 4+: Analytic Reasoning, Tier 5: Metareasoning)       |
+-------------------------------------------------------------+
|                 Axion Governance Kernel                     |
|  (Policy enforcement, trace auditing, capability gating)    |
+-------------------------------------------------------------+
|                      T81Lang / FFI                          |
|  (Agentic constructs, type canonicalization, AST emission)  |
+-------------------------------------------------------------+
|                      T81VM Interpreter                      |
|  (Deterministic step execution, bounds checks, DPE)         |
+-------------------------------------------------------------+
|                        TISC ISA                             |
|  (Ternary instruction vocabulary, opcode decoder)           |
+-------------------------------------------------------------+
|      Ternary Data Types / Base-81 Arithmetic (CanonFS)      |
|  (T81BigInt, T81Float, T81Fraction, 2-bit Packed Trits)     |
+-------------------------------------------------------------+
|             Binary Host Execution Substrate                 |
|  (x86_64 / AArch64 CPUs, SWAR / SIMD kernels, OS host)      |
+-------------------------------------------------------------+
```

## 4. Binary Host Boundary

T81 is a ternary semantic architecture executed on binary hardware. This is an intentional architectural layer, not a compromise. No native ternary hardware exists today, and T81 does not wait for it.

The **binary host execution boundary** is defined by the translation of canonical balanced-ternary semantics into efficient binary operations using 2-bit packed trits (00 = 0, 01 = +1, 11 = -1) and SIMD Within A Register (SWAR). Operations on these packed trits use SWAR techniques to achieve high throughput on modern x86 and ARM processors without sacrificing bit-exact ternary correctness. The host is strictly a dumb execution substrate; all semantic meaning, carry propagation, and overflow policies are governed by the T81 runtime and the canonical arithmetic law defined in RFC-0049, not the host's native ALUs.

## 5. Core Machine Model

The T81 core machine model is a deterministic, stack-based, and register-windowed virtual machine defined as a state transition function.

The abstract machine state `STATE` is formally notationed as:

`STATE = (R, PC, SP, FLAGS, MEM, META)`

Where:
- `R`: The register file, consisting of `R0` (hardwired zero), `R1-R74` (general purpose), and the Axion System Window `R75-R80` (Global Tick, Lineage Hash, active policy mask, etc.).
- `PC`: Program Counter.
- `SP`: Stack Pointer.
- `FLAGS`: Condition flags encoded as a canonical ternary value (NEG=-1, ZERO=0, POS=+1).
- `MEM`: Memory segments securely partitioned into `CODE`, `STACK`, `HEAP`, `TENSOR`, and `META`.
- `META`: Axion-visible metadata, tracing logs, and fault history.

The T81VM evolves this state by applying the TISC instruction at `PC`. Every transition is total and unambiguous.

## 6. Instruction Set Model

The Ternary Instruction Set Computer (TISC) provides the vocabulary for the machine model. It defines exactly how state transitions occur. TISC opcodes are functionally partitioned into three distinct classifications:

1. **Host-Analogous Scaffolding**
   These are instructions that look like classical binary CPU operations but are strictly redefined to preserve ternary canonical semantics.
   *Examples:* `ADD`, `SUB`, `JMP`, `LOAD`, `STORE`, `PUSH`, `POP`. They manage basic control flow and memory, acting as the structural scaffolding of the runtime.

2. **VM-Lifted Composites**
   These are higher-order semantic operations that elevate complex data structures into primitive ISA instructions to guarantee determinism.
   *Examples:* `MAKE_OPTION_SOME`, `RESULT_UNWRAP_OK`, `CHKSHAPE`. These instructions prevent divergent memory layouts by forcing structural data to pass through canonical VM pool allocations and handle deduplication.

3. **Genuinely T81-Native Semantic Instructions**
   These are instructions that have no direct parallel in classical binary ISAs. They exploit ternary logic or serve the specialized AI/Governance thesis of T81.
   *Examples:* `TNOT`, `TAND` (Ternary logic), `TWMATMUL`, `TQUANT`, `TATTN`, `TACT` (Ternary-weight native inference and scaled dot-product attention), `AXREAD`, `AXSET`, `AXVERIFY` (Axion policy syscalls), and `AGENT_INVOKE`.

## 7. Determinism Model

Determinism in T81 is not a best-effort runtime flag; it is a load-bearing architectural pillar. The system requires that identical inputs produce identical traces and identical final states across verified environments (e.g., Linux x86_64, macOS ARM64).

This model enforces:
- **Canonicalization:** Every value has exactly one valid representation. Un-normalized fractions or BigInts with leading zeros trigger decode faults.
- **Backend Equivalence (RFC-0042):** Any optimization (e.g., SWAR, SIMD, JIT) must produce results bit-for-bit identical to a canonical scalar oracle.
- **Zero Undefined Behavior:** Invalid memory accesses or opcodes do not result in silent corruption; they map to deterministic, canonical reason strings and halt normal execution.

Without determinism, the capability to cryptographically hash a runtime trace (CanonHash81) and prove that an AI model adhered to an Axion policy becomes impossible.

## 8. Fault Model

T81 utilizes an explicit, deterministic fault model. It abandons "undefined behavior." When an invariant is broken, the VM immediately transitions into a fault state.

Faults are classified categorically:
- `DecodeFault`: Malformed opcodes or uncanonical data.
- `BoundsFault`: Illegal memory access (e.g., crossing segment boundaries).
- `SecurityFault`: Unprivileged access to the Axion System Window or unapproved policy gates.
- `TierFault`: Exceeding the cognitive or recursion complexity allowed by the current tier.
- `ShapeFault`: Tensor dimension mismatch.

Upon fault, the transition is logged deterministically into the `META` segment, and the Axion kernel dictates whether to terminate, quarantine, or roll back the execution.

## 9. Governance Model (Axion)

The Axion Governance Kernel sits architecturally above the T81VM. It does not execute user code; it supervises *how* code executes. Axion is an ethical and policy enforcement engine that intercepts execution *before* side effects occur.

Axion matters architecturally because policy in traditional systems is reactive—acting after an illegal network call or memory write. In T81, Axion evaluates constraints synchronously at the opcode level. Every `AXSET`, `AXREAD`, `AGENT_INVOKE`, and `WLOAD` instruction halts the VM until Axion verifies the action against the active policy profile (e.g., checking `allowed-tensor-hashes` or `max-recursion`). It provides a fail-closed, pre-side-effect enforcement posture.

## 10. CanonFS and Artifact Model

CanonFS is the deterministic, content-addressed storage subsystem. It is deeply integrated into the VM memory model and Axion tracing.

CanonFS matters architecturally because a deterministic runtime is useless if the input artifacts (weights, models, policies) can be silently altered. CanonFS enforces immutable object identity using CanonHash81. When a model is loaded via `WLOAD` or `TLOADHASH`, the VM and Axion ensure the cryptographic identity of the tensor strictly matches the approved policy profile. Execution traces, fault snippets, and compliance ledgers are written back to CanonFS, cementing a permanent, cryptographically sound provenance chain.

## 11. AI Runtime Model

T81 treats AI inference not as an opaque black box or a secondary library, but as a first-class citizen of the ISA. The decode and runtime inference processes fundamentally change the shape of the machine.

By lifting operations like `TWMATMUL`, `QMATMUL`, and `TATTN` directly into the TISC ISA, T81 allows the Axion kernel to inspect tensor shapes, provenance hashes, and activation limits per instruction. The runtime owns the model state, attention contexts, and execution trace natively, rather than treating them as unmanaged heap allocations passed from a Python script. This shifts inference from a loosely coordinated set of GPU kernels into a deeply integrated, highly auditable state machine.

## 12. Ternary Semantics

Ternary representation (trits: -1, 0, +1) provides mathematical symmetry, removing the sign-bit asymmetry inherent in binary two's-complement arithmetic. This allows for multiplication-free dot products (using conditional additions/subtractions) and zero-drift floating-point truncation.

However, **ternary alone is not enough to make T81 unique**. A ternary CPU without determinism, without immutable storage, and without an inline policy engine would simply be an architectural curiosity. Ternary semantics are a highly efficient, mathematically pure substrate; but it is the strict governance (Axion) and the bit-exact reproducibility applied *over* that substrate that creates T81's true differentiated value.

## 13. Backend and Equivalence Model

To run ternary-native concepts efficiently on binary hardware, T81 uses a tiered backend dispatch model (Scalar -> SWAR -> SIMD AVX2/NEON).

The Backend Equivalence Model (RFC-0042) explicitly forbids silent semantic drift. Any accelerated backend (such as a JIT lowering pass or an explicit vector opcode implementation) must be mathematically identical to the canonical scalar oracle. If a higher-tier backend cannot satisfy the equivalence contract for a given operand size, it deterministically falls back to a verified lower-tier execution path.

## 14. Surface Classification

The repository clearly delineates the maturity of its architectural surfaces:

- **Frozen:** The core TISC ISA opcodes, the canonical Data Types, and the foundational arithmetic semantics. These are locked to prevent breaking changes to execution determinism.
- **Stable (Bounded):** The T81VM interpreter, the Axion fail-closed policy engine, and the RFC-00D1 CanonFS JSON interchange seed. These are ready for production use within the bounds of the Determinism Surface Registry.
- **Experimental / Stubbed:** The bare-metal kernel boot paths, the TCP/IP stack (RFC-00D0), higher-order Cognitive Tiers (Tier 4+), and native ternary hardware interop. These are subject to rapid change and are not yet covered by Deterministic Core Profile (DCP) guarantees.

## 15. TernaryOS / Peripheral Systems

TernaryOS represents an effort to boot T81 directly on bare metal (e.g., via AArch64 EFI), bypassing Linux or macOS entirely.

While the OS path is real—evidenced by the functional QEMU boot sequences and EL0/EL1 exception handling—it is **not necessarily the architectural center of the project.** T81 is overwhelmingly **runtime-first**. The OS effort acts as an aspirational capstone, aiming to provide a completely uncompromised, ground-up host for the runtime. However, the most immediate, load-bearing value of T81 exists in its runtime lane: executing governed inference over existing host operating systems using CanonFS and Axion.

## 16. Relation to Host and Future Hardware

Today, T81 relies on the host OS strictly for memory allocation, basic file I/O, and CPU time. The host is an untrusted hypervisor.

In the future, T81 defines explicit hardware interop contracts (RFC-0055) for hypothetical native ternary hardware targets. Any such hardware must conform to the semantic authority of the TISC ISA. The hardware must map strictly to T81's trap model, memory isolation rules, and Axion visibility. If a hardware accelerator changes the semantic result of an inference operation, it is classified as non-compliant and forbidden from the DCP surface.

## 17. Verification and Conformance

Verification is enforced continuously through the Spec-as-Executable Conformance Model. Determinism is validated by hashing the trace outputs of the VM across different architectures (Linux x86_64 vs. ARM64). Every constraint in the T81 specifications (e.g., arithmetic limits, CanonFS layout) is paired with executable T81Lang compliance scripts (`spec/conformance/`).

## 18. What T81 Is Not

- **T81 is not a microkernel replacement for Linux.** It does not attempt to be a general-purpose desktop operating system for legacy binary applications.
- **T81 is not just an esoteric math experiment.** The balanced ternary logic is strictly a means to achieve zero-drift determinism and efficient inference, not an aesthetic choice.
- **T81 is not hardware.** It is a software runtime that emulates a pristine, governed ternary machine on top of commodity silicon.

## 19. The Load-Bearing Thesis

The strongest, most vital part of the project is the **Deterministic VM integrated with the Axion Policy Engine and CanonFS.** This triad allows a user to run an AI model, mathematically prove exactly which weights were used, guarantee that a specific security policy was enforced at the hardware-instruction level, and generate an irrefutable, bit-exact cryptographic trace of the entire thought process.

Conversely, the bare-metal kernel and OS ambitions are currently broad but fragile. While conceptually aligned with the project's goals, they risk appearing as duplicated infrastructure (rebuilding page tables, drivers, and schedulers) unless the true differentiators—the runtime governance and ternary inference lanes—remain foregrounded.

## 20. Final Architectural Summary

T81 is a rigorous, deeply constrained computational sandbox designed to cage and govern AI. By moving execution into a deterministic, ternary-native ISA and enforcing pre-side-effect policies via Axion, it provides an architectural guarantee of reproducibility that conventional operating systems and software runtimes cannot match. It treats the current era of binary hardware as a temporary substrate, enforcing strict mathematical equivalence to an idealized, cryptographically secure ternary machine.

---

## Appendix: Architectural One-Sentence Definition

T81 is a deterministic, ternary-native virtual machine and policy engine that guarantees bit-exact reproducible execution and enforces cryptographic governance before side effects occur in AI inference workloads.