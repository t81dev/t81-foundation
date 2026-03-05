# IEEE-T81: Standard for Deterministic Ternary Computing Systems

**Front Matter**

**Title Page**  
IEEE-T81: Standard for Deterministic Ternary Computing Systems  
Designation: Draft Standard (Fictional)  
Fictional Approval Date: March 2026  
Sponsor: IEEE Computer Society  
Working Group: IEEE-T81 Working Group on Deterministic Ternary Computing Systems (Fictional)  
Status: Draft for technical review and prototype implementation  

**Intended audience**: CPU/ISA designers, VM/runtime implementers, compiler and tooling authors, verification and conformance engineers  

**Note (informative)**: This is a creative draft standard intended to demonstrate IEEE-style specification structure and rigor. It contains fictional elements (e.g., “IEEE-T81”) while contrasting against real-world determinism challenges in conventional binary computing, including IEEE 754 floating-point variability.

## Abstract

Reproducibility is increasingly required in scientific computing, machine learning inference, safety-critical control, and audit-oriented infrastructure. Nevertheless, conventional binary computing frequently exhibits cross-platform divergence due to floating-point evaluation differences, fused operations, compiler reassociation and vectorization, inconsistent math-library results, ambiguous serialization choices, and language-defined “undefined” or implementation-dependent behaviors.

This draft standard defines a deterministic computing substrate based on balanced ternary, using trits with values {−1, 0, +1}. Balanced ternary provides a symmetric signed-digit domain that supports unique canonical encodings for integers and deterministic normalization rules, facilitating identical results across heterogeneous platforms.

IEEE-T81 specifies:  
(a) canonical balanced-ternary representations and a byte-stable packing strategy,  
(b) deterministic, fully-defined instruction semantics (including edge-case behavior),  
(c) a platform-independent runtime execution model for deterministic sequencing and environmental inputs,  
(d) a canonical serialization format producing identical byte streams for equivalent structures, and  
(e) conformance verification via executable specifications and hash-matched outputs.  

The goal is **D3 determinism**: identical outputs across operating systems, compilers, and CPU architectures for the same program and inputs, subject to conformance requirements.

**Keywords**  
balanced ternary; trit; tryte; deterministic computing; reproducibility; canonical representation; canonical serialization; conformance verification; deterministic runtime; cross-platform trace hashing

## Background and Goals

### Introduction

Modern computational practice increasingly treats bitwise-identical reproducibility not merely as a convenience but as a requirement: scientific replication, regulated audit trails, safety certification, distributed consensus, and ML deployment pipelines all benefit from deterministic outputs. However, typical binary platforms permit many degrees of freedom that can change results without changing source-level intent.

Common sources of nondeterminism include:

- Floating-point non-associativity and evaluation reordering. Reassociation or parallel reductions can change rounding points, producing divergent results even when each individual operation conforms to IEEE 754 formats.
- Fused multiply-add (FMA) contraction and compiler policies. Whether expressions are contracted into fused operations depends on compiler flags and language modes (e.g., -ffp-contract in GCC). This can change rounding and produce different numeric results.
- Divergent math-library function results. Many transcendental functions are not required to be correctly rounded across implementations, so CPU and GPU or different libm implementations may differ slightly.
- Serialization ambiguities. Equivalent data may encode into different byte streams when key ordering, padding, endianness, or schema-dependent encodings vary; even “deterministic” modes in popular encodings are not necessarily canonical across time or implementations.

IEEE-T81 responds by defining determinism as a first-class architectural invariant. It uses balanced ternary trits to ground canonical numeric representation and defines deterministic semantics and serialization to minimize degrees of freedom that otherwise vary by platform. Balanced ternary is a known signed-digit system using digits {−1, 0, +1}.

### Scope

This standard specifies the following normative components:

- Canonical balanced-ternary representation for integers and fixed-point values, including normalization, truncation, and widening rules.
- T81 packing and interchange encoding rules for embedding trits/trytes into octets for platform-neutral storage and transport.
- Deterministic instruction semantics for core arithmetic, comparisons, and ternary logical operations, including precise edge-case behavior.
- Deterministic runtime execution model for single-threaded execution and deterministically modeled environmental inputs.
- Canonical serialization for structured data and execution artifacts, producing identical byte streams across implementations.
- Conformance and verification requirements, including trace hashing and test suites.

This standard explicitly excludes:

- Hardware circuit implementation details (voltage levels, transistor designs, memory cell technologies).
- Concurrency, parallel scheduling, and nondeterministic interleavings (a future extension may address deterministic concurrency).
- Real-time guarantees (timing determinism is distinct from functional determinism).

### Purpose

The purpose of IEEE-T81 is to enable reproducible computation across heterogeneous platforms by making representation, semantics, and serialization canonical.

This standard defines determinism levels as follows (see also “Determinism Levels” in the definitions clause):  
D0 (single-process), D1 (cross-OS), D2 (cross-compiler/toolchain), D3 (cross-architecture), D4 (cross-hardware implementations).  
This draft targets D3 determinism for conforming implementations under the defined runtime model.

## Referenced Documents and Terminology

### Normative References

The following documents are referenced for contrast, alignment, or incorporated normative concepts. Where dated references are used, only the cited edition applies. Where undated, the latest edition applies.

- IEEE, IEEE Standard for Floating-Point Arithmetic (IEEE 754-2019) (used for contrast; not normatively adopted by T81 arithmetic).
- ISO/IEC 60559 (floating-point arithmetic standard aligned with IEEE 754; informational contrast).
- IEEE 1666-2023, SystemC® Language Reference Manual (used as a simulation/conformance analogy for executable specification culture).
- RFC 8785, JSON Canonicalization Scheme (JCS) (canonicalization and “hashable” representation principles).
- RFC 8949, CBOR (deterministic encoding requirements and deterministic key ordering guidance).
- NIST, FIPS 180-4, Secure Hash Standard (SHS) (hash requirements for canonical trace hashing).
- Reproducible Builds Project, “Definition of reproducible builds” (terminology alignment for “bit-for-bit reproducibility”).
- Historical balanced-ternary computing reference: Moscow State University Setun ternary computer background (informative contrast; not required).

### Definitions, Acronyms, and Abbreviations

**Normative language**. The terms *shall*, *should*, and *may* are used to express requirements, recommendations, and permissions, consistent with IEEE standards drafting conventions.

**Definitions**

- **balanced ternary**: A base-3 signed-digit representation using digits in the set {−1, 0, +1}.
- **trit**: A ternary digit. In IEEE-T81, trits are balanced unless otherwise stated.
- **tryte**: A fixed group of four trits. Because 3⁴ = 81, a tryte spans 81 distinct values and is the naming basis for “T81.”
- **canonical representation**: A unique, normalization-constrained encoding of a value such that equivalent values do not have multiple encodings.
- **canonical serialization**: A unique, platform-independent byte encoding that is identical for semantically equivalent values and structures (e.g., via deterministic ordering).
- **determinism (functional)**: For a given program and input stream, the produced outputs (and where specified, the execution trace) are identical across conforming implementations.
- **execution trace**: A canonical record of execution events (instruction steps, I/O events, exceptions) suitable for hashing and replay.
- **T81-CS**: The canonical serialization format defined in this standard.

**Determinism levels**

| Level | Determinism definition                              | Typical interpretation in IEEE-T81                          |
|-------|-----------------------------------------------------|-------------------------------------------------------------|
| D0    | Deterministic within a single process execution     | Same inputs → same outputs on one platform                  |
| D1    | Deterministic across operating systems              | Cross-OS reproducibility for same build and CPU             |
| D2    | Deterministic across compiler toolchains            | Cross-compiler reproducibility for same target              |
| D3    | Deterministic across CPU architectures              | Cross-architecture reproducibility (e.g., x86 vs. Arm)      |
| D4    | Deterministic across hardware implementations       | Cross-implementation equivalence at ISA+microarchitecture level |

This abstraction and tiering are consistent with the determinism model described in the accompanying design draft.

## Architecture Overview

### Overview

IEEE-T81 specifies a layered deterministic substrate. Each layer constrains degrees of freedom that otherwise cause divergence across platforms.

**Layer model**

- **Representation layer** — Defines canonical balanced-ternary encodings, normalization, widening/truncation rules, and tryte packing.
- **Instruction semantics layer** — Defines fully specified behavior of operations, rounding, exceptions, and edge cases (no “undefined behavior”).
- **Runtime execution model** — Defines deterministic sequencing and a controlled interface to environmental inputs.
- **Conformance and verification** — Defines executable reference behavior, tests, and trace hashing to validate D0–D3 determinism targets.

This layered view aligns with typical deterministic-substrate architectural framing described in the design draft (representation, semantics, runtime, conformance).

**Figure 1 — Layered architecture (informative ASCII)**
