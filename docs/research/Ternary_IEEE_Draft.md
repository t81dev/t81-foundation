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

```
+---------------------------------------------------------------+
| Conformance + Verification                                    |
| - executable reference semantics                              |
| - test vectors + trace hashing                                |
+---------------------------------------------------------------+
| Runtime Execution Model                                       |
| - deterministic step sequencing                               |
| - deterministic environment inputs (replayable)               |
+---------------------------------------------------------------+
| Instruction Semantics                                         |
| - total, platform-independent operation definitions           |
| - explicit rounding, exceptions, overflow behaviors           |
+---------------------------------------------------------------+
| Representation Layer                                          |
| - canonical balanced trits and trytes                         |
| - canonical packing, normalization, serialization primitives  |
+---------------------------------------------------------------+
```

### Design rationale (informative)

In contrast to approaches that merely “tighten” IEEE 754 usage with compiler flags, IEEE-T81 treats determinism as an architectural contract: it constrains representation choices and defines canonical encodings and operation semantics so that multiple implementations can be validated via identical traces/hashes. This complements real-world observations that compiler FP contraction policies and heterogeneous execution (CPU vs GPU) can change results.

## Technical Specifications

### Representation Layer

#### Balanced trit domain

A balanced trit *t* shall take values in {−1, 0, +1}. For textual interchange in this standard, implementations shall support at least one of the following glyph conventions:

- “−, 0, +” (preferred for clarity), or
- “T, 0, 1” where T ≡ −1, a convention commonly used in balanced ternary descriptions.

#### Integer value interpretation

A balanced-ternary integer is a finite trit sequence:  
**t** = ⟨t_{n-1}, …, t_1, t_0⟩, t_i ∈ {-1,0,+1}.  

Its mathematical value is:  
V(**t**) = ∑_{i=0}^{n-1} t_i · 3^i.

#### Canonical normalization for variable-length integers

A variable-length integer encoding shall be canonical:

- Zero shall be encoded as exactly one trit: ⟨0⟩.
- Nonzero values shall not contain leading zero trits; i.e., t_{n-1} ≠ 0.
- Canonicalization shall not permit alternative encodings for the same integer (e.g., no multiple signed zeros), a property naturally supported by balanced ternary when normalized.

#### Fixed-width integer types

This standard defines a family of fixed-width integer types T81.I(n), where *n* is the number of trits.

A value of type T81.I(n) shall be represented by exactly *n* trits.  
The representable range is [−(3^n−1)/2, (3^n−1)/2].

Conversions between widths shall be deterministic:

- Widening shall prepend leading zero trits.
- Truncation shall remove leading trits beyond the target width and shall apply the overflow mode specified in “Instruction Semantics” (trap/saturate/wrap).

#### Trytes and the base-81 relationship

A tryte is defined as exactly 4 trits. Because 81 = 3^4, trytes provide an 81-state unit (“T81”) analogous to hex nibbles (16 states) in binary systems. The base-81 analysis notes that a base-81 “digit” encapsulates four trits, providing dense grouping for ternary-derived systems.

A tryte (τ = ⟨t_3,t_2,t_1,t_0⟩) represents:  
V(τ) = t_0·3^0 + t_1·3^1 + t_2·3^2 + t_3·3^3,  
and thus spans the range [-40, +40].

#### Canonical tryte byte packing (T81 Tryte-Octet encoding)

For platform-independent storage, IEEE-T81 defines Tryte-Octet packing:

A tryte shall be serialized into one octet (b ∈ [0,80]) by a deterministic mapping:  
b = V(τ) + 40.

Thus:  
(b=0 ↔ V(τ)=-40)  
(b=40 ↔ V(τ)=0)  
(b=80 ↔ V(τ)=+40)

Octet values 81–255 are reserved and shall not be used in canonical encodings.

**Table 1 — Canonical trit to glyph mapping (normative for text form)**

| Trit value | Preferred glyph | Alternate glyph |
|------------|-----------------|-----------------|
| −1         | −               | T               |
| 0          | 0               | 0               |
| +1         | +               | 1               |

#### Binary storage alignment considerations (informative)

Even when using base-81 groupings, storing 81 distinct states in binary requires at least 7 bits; naive byte storage wastes states. These alignment issues and tradeoffs are discussed in the base-81 analysis, motivating an explicit canonical packing choice for interchange.

#### Fixed-point values

A fixed-point number type T81.Q(n, f) shall be defined as:

- A signed balanced integer (X) encoded as T81.I(n)
- A fractional scale (f ≥ 0) (number of fractional trits)

Its value is:  
V = X / 3^f.

Canonical form shall require that:

- The underlying integer (X) uses the canonical T81.I(n) representation.
- The scale (f) is explicit in the type and not inferred from data.

**Examples (informative)**

**Figure 2 — Example conversions (decimal → balanced ternary)**

- 5_{10}: one balanced representation is (1,T,T) because (1·9 + (-1)·3 + (-1)·1 = 5).
- (-4_{10}): (T,T) because ((-1)·3 + (-1)·1 = -4).

### Instruction Semantics

#### General requirements

An IEEE-T81 conforming implementation shall define all specified operations as total functions over their input domains, except where the operation explicitly signals a trap (exception). No behavior shall be left undefined or implementation-defined for conforming programs.

Instruction semantics are defined in terms of a mathematical “ideal” evaluation followed by deterministic rounding/truncation and exception handling.

#### Execution profiles

To enable consistent deployment, IEEE-T81 defines deterministic execution profiles. A conforming runtime shall choose one profile and apply it uniformly for a run:

- **Profile T81-P0 (Strict Trap)**: Overflow, division-by-zero, and invalid operations shall trap.
- **Profile T81-P1 (Saturating)**: Overflow shall saturate; division-by-zero and invalid operations shall trap.
- **Profile T81-P2 (Wrapping)**: Overflow shall wrap modulo (3^n) (for n-trit types) using canonical balanced representation; division-by-zero and invalid operations shall trap.

A runtime claiming D3 determinism shall record the chosen profile in the canonical execution header (see “Canonical Serialization”).

#### Exception conditions and flags

IEEE-T81 uses a status word T81.STATUS with deterministic update rules. This is conceptually similar to IEEE 754 exception flags, but numerically and semantically distinct.

T81 exceptions:

- DIVZ: Division by zero
- OVF: Overflow
- INV: Invalid operation (e.g., operation on reserved encodings)
- INX: Inexact (result rounded)
- IOE: Environment/I/O event (used for trace determinism)

Flags shall be set deterministically:

- Flags shall be sticky within an epoch (until explicit clear).
- Setting a flag shall not depend on platform timing.

#### Arithmetic operations on T81.I(n)

Let inputs (a, b ∈ T81.I(n)). Define (A,B ∈ ℤ) as their mathematical values.

**Addition and subtraction**

ADD(a,b) computes (S = A+B).  
SUB(a,b) computes (D = A-B).

If (S) (or (D)) is out of range, behavior depends on the profile:

- Trap profile: set OVF and trap.
- Saturating profile: clamp to min/max representable.
- Wrapping profile: produce (S mod 3^n) mapped to the canonical balanced representative in range [-(3^n-1)/2, (3^n-1)/2].

**Normative digitwise carry semantics (balanced addition)**

For implementations that operate digitwise, addition may be implemented via local carry (c_i ∈ {-1,0,+1}) at each trit position.

Given (a_i,b_i ∈ {-1,0,+1}) and carry-in (c_i), define:  
s_i = a_i + b_i + c_i

Then choose output trit (r_i) and carry-out (c_{i+1}) such that:  
s_i = r_i + 3 c_{i+1}, r_i ∈ {-1,0,+1}, c_{i+1} ∈ {-1,0,+1}.

This mapping is unique because (s_i ∈ [-3,+3]) and the decomposition is forced.

**Pseudocode (normative)**

```text
function BT_ADD(a[0..n-1], b[0..n-1]) -> (r[0..n-1], carry_out)
    c := 0
    for i in 0..n-1:
        s := a[i] + b[i] + c          // s in [-3..+3]
        if s == -3: r[i] := 0;   c := -1
        if s == -2: r[i] := +1;  c := -1
        if s == -1: r[i] := -1;  c := 0
        if s == 0:  r[i] := 0;   c := 0
        if s == +1: r[i] := +1;  c := 0
        if s == +2: r[i] := -1;  c := +1
        if s == +3: r[i] := 0;   c := +1
    return (r, c)
```

**Multiplication**

MUL(a,b) computes (P = A·B) in (ℤ), then applies overflow behavior per profile.

**Division**

DIV(a,b, mode) computes (Q = A / B) with deterministic rounding.

If (B = 0): set DIVZ and trap.  
Else compute the exact rational (R = A/B).  
Round (R) to an integer (Q) using the rounding mode defined below.  
Produce remainder (Rem = A - Q·B).

**Rounding modes (normative)**

IEEE-T81 defines the following rounding modes for integer division and fixed-point narrowing:

| Mode name | Meaning (toward)     | Tie-breaking rule                              |
|-----------|----------------------|------------------------------------------------|
| RN-T0     | nearest              | ties choose result whose least-significant trit is 0 (“trit-even”) |
| RN-A0     | nearest              | ties away from zero                            |
| RZ        | zero                 | truncation toward 0                            |
| RP        | +∞                   | ceiling                                        |
| RM        | −∞                   | floor                                          |

This “trit-even” tie-breaking generalizes the IEEE 754 “ties to even” idea (choose a canonical least-significant digit), while avoiding platform-defined tie handling.

#### Logical and comparison operations

IEEE-T81 defines a three-valued boolean type T81.B3, encoded as a single trit:

- −1 = FALSE
- 0 = UNKNOWN
- +1 = TRUE

Operations shall use a deterministic three-valued logic algebra; this specification adopts Kleene-style monotonic truth tables (normative) to avoid divergence in “unknown” propagation.

**Table 2 — T81.B3 NOT (normative)**

| x   | NOT(x) |
|-----|--------|
| −1  | +1     |
| 0   | 0      |
| +1  | −1     |

**Table 3 — T81.B3 AND (normative)**

| AND | −1 | 0 | +1 |
|-----|----|---|----|
| −1  | −1 | −1| −1 |
| 0   | −1 | 0 | 0  |
| +1  | −1 | 0 | +1 |

**Table 4 — T81.B3 OR (normative)**

| OR  | −1 | 0  | +1 |
|-----|----|----|----|
| −1  | −1 | 0  | +1 |
| 0   | 0  | 0  | +1 |
| +1  | +1 | +1 | +1 |

Comparators (LT, EQ, GT) shall return T81.B3 with UNKNOWN reserved for comparisons involving invalid operands (e.g., reserved encodings), ensuring there is exactly one “unknown” mechanism rather than multiple NaN payload-like cases.

### Runtime Execution Model

#### Deterministic sequencing

A conforming IEEE-T81 runtime:

- shall execute instructions in a single total order (no reordering),
- shall define evaluation order for expression trees as left-to-right, depth-first,
- shall not introduce nondeterministic reduction orderings.

This requirement directly targets observed sources of nondeterminism where parallelism or reassociation changes floating-point summation order.

#### Memory model (single-threaded)

IEEE-T81 defines an abstract memory model for deterministic execution:

- Memory is a sequence of octets.
- Canonical tryte serialization (Tryte-Octet) shall be used for storing balanced-ternary values in canonical form.
- Endianness shall not be an implementation choice for canonical formats: all canonical formats are defined as octet sequences in increasing address order.

#### Deterministic external inputs

All environmental nondeterminism (time, random numbers, I/O scheduling, network arrival order) shall be mediated through a T81 Environment Interface (T81-ENV):

- Record mode: runtime emits a canonical event log.
- Replay mode: runtime consumes the event log deterministically.

In D3 conformance mode, any non-replay-specified environmental call shall raise IOE and trap, unless the program declares itself “non-replayable” (informative-only profile).

#### Deterministic math library policy (informative but recommended)

Because typical math libraries may yield different results across CPU and GPU and are not always correctly rounded, a T81 runtime should avoid delegating transcendental operations to platform libm unless the operation is implemented as part of the deterministic spec and verified against the executable reference. This recommendation reflects observations in heterogeneous computing guidance that CPU and GPU math library results often differ and are not guaranteed correctly rounded.

### Canonical Serialization

#### Goals

Canonical serialization ensures that semantically equivalent data structures produce identical byte streams, enabling stable hashing and cross-platform verification. Canonicalization principles are used in standardized forms such as JSON canonicalization and deterministic CBOR rules.

#### T81 Canonical Serialization (T81-CS) format

T81-CS is a binary, self-delimiting format defined as a restricted TLV (Type–Length–Value) with canonical ordering rules.

**Core rules (normative)**

- Fixed endianness: Multioctet integers in T81-CS shall be encoded in big-endian octet order.
- Shortest form: Length fields and integer magnitudes shall use the shortest possible encoding that can represent the value (aligned with “preferred serialization” in deterministic CBOR).
- Deterministic ordering: Map/dictionary keys shall be ordered deterministically by bytewise lexical order of their canonical key encoding (conceptually aligned with deterministic property sorting in JCS and map-key sorting in CBOR).
- No padding: No alignment padding is permitted.
- No multiple encodings: Encoders shall not emit alternate encodings for the same semantic value.

**Primitive types (normative)**

- T81-CS Null: tag 0x00, length 0
- T81-CS Bool3: tag 0x01, length 1 octet (Tryte-Octet restricted to {39,40,41} mapping to {−1,0,+1})
- T81-CS TryteString: tag 0x02, length L, payload L octets each in [0..80]
- T81-CS Int(n): tag 0x03, payload: (n_trits as trytes) + declared trit length
- T81-CS Fixed(n,f): tag 0x04, payload: Int(n) + scale f
- T81-CS Bytes: tag 0x05, payload: raw octets
- T81-CS Text: tag 0x06, payload: UTF-8 octets (normalization policy is application-defined; this standard does not mandate Unicode normalization)
- T81-CS Array: tag 0x07, payload: concatenation of canonical child encodings
- T81-CS Map: tag 0x08, payload: sorted (key,value) pairs; keys must be canonical encodings of primitive scalars

#### Canonical header for deterministic runs (normative)

A deterministic execution record shall begin with a canonical header map containing at minimum:

- profile (T81-P0 / T81-P1 / T81-P2)
- determinism_level (D0–D4 claimed)
- program_id (application-defined bytes)
- input_hash (hash of canonical input bundle)
- trace_hash_alg (e.g., SHA-256)

Hash algorithms referenced in conformance shall be standardized secure hash functions; this draft specifies SHA-256 as the baseline, as defined in the Secure Hash Standard.

## Conformance

### Conformance and Verification

#### Conformance classes

A system may claim conformance to IEEE-T81 at one or more determinism levels, but shall satisfy all requirements for the claimed level(s):

- T81-C0: Representation + instruction semantics; D0 determinism within a process.
- T81-C1: Adds canonical serialization; enables D1 determinism when runtime inputs are stable.
- T81-C2: Adds deterministic runtime profile recording; targets D2 determinism across compilers.
- T81-C3: Adds trace hashing and executable reference equivalence; targets D3 determinism across CPU architectures.

#### Mandatory artifacts

A conforming implementation shall provide:

- A reference interpreter mode (may be slower) whose behavior is bitwise identical to the normative executable semantics.
- A conformance test runner capable of:
  - executing arithmetic and serialization vectors,
  - producing canonical trace records,
  - producing trace digests.

#### Arithmetic test vectors (normative requirements)

The conformance suite shall include:

- Addition/subtraction vectors covering:
  - full carry range cases (sums hitting −3..+3 in a trit),
  - boundary values (min/max),
  - overflow cases for each profile.
- Multiplication vectors including boundary products causing overflow.
- Division vectors including:
  - exact divisions,
  - inexact divisions requiring rounding,
  - tie cases for RN-T0 and RN-A0,
  - division-by-zero.

#### Serialization test vectors (normative requirements)

The suite shall include canonical encodings for:

- maps with keys requiring ordering checks,
- arrays containing mixed types,
- tryte strings and fixed-point values,
- disallowed alternate encodings (which decoders shall reject in strict mode).

These requirements reflect the general principle that deterministic encodings must remove representational choices (e.g., sorting requirements and shortest-form requirements as described in deterministic CBOR, and deterministic property sorting as in JCS).

#### Trace hashing (normative requirements)

For a deterministic run, the runtime shall produce:

- a canonical trace stream encoded in T81-CS
- a digest computed over the trace bytes using SHA-256

The SHA-256 algorithm is specified in FIPS 180-4.

#### Cross-platform verification rule (normative)

For any conformance claim at D3, the implementation shall demonstrate that for the same canonical program bundle and canonical input bundle:

- the output bundle bytes match exactly, and
- the trace digest matches exactly

across at least two distinct CPU architectures and two distinct OS environments, or across one native architecture and one ISA-accurate emulator.

**Informative note on motivation**  
This “hash-matched output” rule is intended to operationalize the same kind of “bit-for-bit identical artifact” notion used in reproducible builds definitions, but applied to execution outputs and traces rather than only build artifacts.

## Annexes (Informative)

### Annex A — Performance considerations

Balanced-ternary determinism is not inherently free: canonical arithmetic and strict evaluation order can prevent certain aggressive optimizations (e.g., reassociation). The goal is to remove degrees of freedom that commonly change results under compiler optimization or heterogeneous reduction orderings.

Tryte-based packing is intended to reduce overhead in storage and memory bandwidth by grouping four trits into a base-81 unit (81 = 3⁴). While base-81 groupings are dense in ternary terms, binary alignment still requires explicit packing rules because 81 does not align to powers of two; deterministic packing is therefore a core part of the specification, not an implementation detail.

### Annex B — Emulation on binary hardware

An IEEE-T81 implementation may be realized via:

- a VM/interpreter with explicit trit arrays or tryte octets,
- a JIT compiler lowering ternary operations into deterministic binary sequences,
- an ISA-level emulator providing instruction-accurate deterministic state transitions.

To preserve D3 determinism, any use of host floating-point (IEEE 754) must be avoided in normative computation unless formally proven equivalent to the T81 semantics. This caution is motivated by documented differences arising from FMA availability, threading differences, and non-guaranteed correct rounding of many math-library functions in heterogeneous environments.

### Annex C — Example applications

Deterministic ternary substrates are well suited to:

- Scientific pipelines requiring byte-identical reruns across machines and over time.
- ML inference requiring bit-level reproducibility and stable regression testing; deep learning reductions are known to be sensitive to summation order due to floating-point non-associativity.
- Audit and compliance systems where execution traces can be hashed and verified using standardized hash functions.

### Annex D — Limitations and future extensions

- Deterministic concurrency is out of scope. Future revisions may define deterministic parallel reductions and scheduler constraints.
- Floating-point analogs. This draft focuses on integer and fixed-point determinism. A future T81 floating-point annex may define ternary scientific notation with canonical rounding modes, but must do so without reintroducing multi-encoding ambiguities (e.g., NaN payload variability) that IEEE 754 explicitly permits in some conversions.
- Hardware implementations. Native ternary hardware is not specified; historical ternary machines such as the Setun demonstrate feasibility of balanced-ternary computing but are not normative reference implementations for this standard.

## Bibliography

- IEEE. IEEE Standard for Floating-Point Arithmetic (IEEE 754-2019).
- IEEE. IEEE 1666-2023: SystemC® Language Reference Manual.
- NIST. FIPS 180-4: Secure Hash Standard (SHS).
- IETF. RFC 8785: JSON Canonicalization Scheme (JCS).
- IETF. RFC 8949: Concise Binary Object Representation (CBOR).
- Reproducible Builds Project. “Definitions: When is a build reproducible?”.
- NVIDIA. Floating Point and IEEE 754 (CUDA documentation; heterogeneous FP considerations).
- GNU Project / GCC. “Optimize Options: -ffp-contract …”.
- RFC Editor. RFC series (canonical encoding principles; referenced via RFC 8785 and RFC 8949).
- Moscow State University. Setun historical context (balanced ternary).
- Douglas W. Jones. “Ternary number systems / balanced ternary notes.”
- User-supplied design and analysis notes supporting this fictional draft: “Deterministic Computing Beyond Binary: A Balanced-Ternary Substrate for Cross-Platform Reproducible Systems.”
- User-supplied base-81/ternary analysis notes supporting tryte grouping rationale: “TYRNARY – T81ANALYSIS: Base-81 (T81) and Trinary Computing.”

This version should render beautifully in most Markdown viewers (GitHub, VS Code, Obsidian, etc.) while keeping the full technical precision and IEEE-like formality intact. Let me know if you want adjustments like adding more LaTeX math rendering hints or splitting into sections/files!
