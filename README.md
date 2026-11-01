# T81 Foundation

### Exploring computation beyond the binary assumption.

**Ternary computation · Alternative representations · Deterministic systems · Experimental computer architecture**

[Research Charter](docs/research/RESEARCH_CHARTER.md) · [Research Roadmap](docs/ROADMAP.md) · [Architecture](docs/ARCHITECTURE.md) · [AI Research Context](docs/ai-context/)

---

## What is T81?

**T81 Foundation is an open-source experimental computing research program investigating whether alternative computational representations—particularly balanced ternary—can provide measurable advantages over conventional binary systems.**

The project studies computation at several levels: mathematical representation, algorithms, software execution, storage, hardware architecture, and verifiable system behavior.

Its central question is:

> Under what conditions can ternary computation outperform, simplify, or otherwise improve upon an equivalent binary approach?

The answer is not assumed.

Some experiments have demonstrated useful mathematical and representational properties. Others have shown that conventional binary implementations remain faster or more efficient. Several important questions remain unresolved.

T81 exists to investigate those boundaries systematically.

**This is a research platform, not a claim that a superior ternary computer has already been built.**

---

## Why ternary?

Modern computing overwhelmingly uses binary states: 0 and 1.

Ternary computing uses three states. In balanced ternary, these are represented as:

\[
T = \{-1,0,+1\}
\]

This creates interesting possibilities for signed arithmetic, numerical representation, state encoding, decision systems, and computational architecture.

For example:

| Property | Mathematical relationship |
|---|---|
| One trit | Three possible states |
| Four trits | \(3^4 = 81\) states |
| Six trits | \(3^6 = 729\) states |
| Information per trit | \(\log_2 3 \approx 1.585\) bits |
| Base-81 digit | Four grouped trits |

These relationships are mathematical facts.

They do **not** establish that ternary processors are faster, that ternary memory is physically denser, or that ternary algorithms consume less energy.

Those are separate engineering questions requiring appropriate comparisons.

T81 investigates the distinction.

---

## What have we learned?

Research is valuable when it identifies both opportunities and limitations.

T81's experimental history has produced several important findings.

### 1. Representation density is not execution speed

Packed ternary data can reduce storage requirements.

However, software must still decode, manipulate, and execute those representations on conventional binary hardware.

In one historical AVX2 matrix-vector experiment:

| Implementation | Reported throughput |
|---|---:|
| Native INT8 AVX2 | 17.15 GFLOP/s |
| Packed 2-bit ternary AVX2 | 12.91 GFLOP/s |

The compact ternary representation did not outperform the conventional INT8 implementation in that workload.

This is a bounded experimental result—not a proof that ternary computation is universally slower.

[Explore the packed ternary research](docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Adaptive representations can have conditional advantages

Research into adaptive representation tiering has identified configurations in which a ternary representation has a lower modeled cost than a matched binary alternative.

Those advantages depend on workload assumptions, metadata, conversion costs, and representation boundaries.

The corrected H04-R02 research preserves these conditional findings while explicitly leaving physical performance unresolved.

[Read the corrected H04-R02 findings](experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. Correctness does not establish optimization

A recent experimental investigation, T81-AIOPT-001, examined multiple representations of four-trit pattern matching.

Its M1 implementation passed extensive local correctness checks, but the experimental workload could be reduced to a simple ordinal comparison.

The result demonstrates why an exhaustive correctness test can still be insufficient for evaluating computational advantage.

The next question is not simply whether another representation works.

It is whether the chosen workload can distinguish meaningful differences between competing algorithms.

[Explore T81-AIOPT-001](docs/research/aiopt/)

### 4. Physical ternary computing remains an open question

Software-emulated ternary computation is not equivalent to computation on native ternary hardware.

Evaluating physical devices requires matched evidence for area, energy, latency, reliability, sensing, encoding, and peripheral costs.

Current physical evaluation remains blocked where adequate device-characterization evidence is unavailable.

T81 does not interpret missing evidence as proof of impossibility.

---

## The T81 research architecture

The repository contains several experimental subsystems developed to investigate computational representations and controlled execution.

```text
                     T81 FOUNDATION
                Experimental Research Program
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTATION       EXECUTION          ASSURANCE
          |                  |                  |
    Balanced ternary       TISC ISA           Axion
    Base-81 / Base-243     T81VM              CanonFS
    Packed data            T81Lang            Evidence
    Adaptive tiering       Core libraries     Provenance
          |                  |                  |
          +------------------+------------------+
                             |
                   CONTROLLED EXPERIMENTS
                             |
               Conventional binary baselines
                             |
                 Verification and comparison
                             |
                    Research conclusions
```

This is a conceptual organization of the research program, not a claim that every subsystem is fully integrated into one verified production runtime.

### Principal components

| Component | Research role |
|---|---|
| **TISC** | Experimental instruction-set semantics |
| **T81VM** | Virtual-machine execution and determinism |
| **T81Lang** | Language frontend and compilation infrastructure |
| **T81 Core** | Numerical types, containers, and representation primitives |
| **Axion** | Policy evaluation and controlled authorization |
| **CanonFS** | Content-addressed artifacts and integrity mechanisms |
| **Research experiments** | Hypothesis testing, correctness validation, and comparative measurements |

These components have different maturity levels. A working implementation is not automatically a validated performance advantage.

---

## Three research tracks

T81 organizes its work into three complementary tracks.

### Track R — Computational research

Investigate whether alternative representations or computational architectures produce useful advantages.

Subjects include:

- Balanced ternary arithmetic and representation.
- Base-81 and higher-order grouping.
- Adaptive representation selection.
- Packed low-bit computation.
- Memory and data-movement costs.
- Native ternary hardware hypotheses.
- Ternary AI inference and model representations.

**Success means establishing a defensible result, including a negative result.**

### Track F — Foundation engineering

Develop the tools needed to conduct reliable experiments and evaluate deterministic computation.

This includes virtual machines, language tooling, artifact identity, authorization, replay, and verification infrastructure.

These capabilities may be useful independently of ternary computation.

Their engineering value must be evaluated separately from claims of radix-specific superiority.

### Track M — Research memory

Preserve the evidence behind every conclusion.

This includes source history, experimental protocols, failed attempts, corrections, reproducibility records, and unresolved questions.

A superseded result remains part of the historical record.

A mathematical model is not silently promoted to a physical measurement.

An AI-generated assessment is not automatically independent verification.

[Read the Master Continuity Charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)

---

## Research methodology

T81 follows an evidence-first approach.

Every comparative experiment should answer five questions:

1. **What is the hypothesis?** Define a falsifiable proposition.
2. **What is the baseline?** Compare against a functionally equivalent, appropriately optimized conventional alternative.
3. **What is measured?** Separate correctness, storage, latency, throughput, energy, and other relevant costs.
4. **What are the limitations?** Identify assumptions, hardware, workload scope, uncertainty, and unresolved dependencies.
5. **What conclusion is justified?** Distinguish supported, unsupported, inconclusive, blocked, and untested outcomes.

The project also distinguishes between:

**Implemented → Tested → Measured → Comparatively established**

These are different evidentiary levels, not interchangeable labels.

An experimental result must not claim more than its evidence supports.

---

## The original vision: Hanoi

T81's history predates the current Foundation repository.

Its earliest known precursor, **Hanoi**, explored recursive computation, ternary virtual-machine structures, symbolic dispatch, and workload-dependent representation tiers using CWEB literate programming.

The project emerged from investigations into discrete mathematics, algorithms, computational models, and the work of Donald Knuth.

Over time, the research expanded into arithmetic libraries, language tooling, virtual machines, low-bit AI computation, deterministic execution, governance, and experimental hardware.

Some historical designs were incomplete. Some assumptions proved incorrect. Others remain unexplored.

The purpose of preserving this lineage is not to validate every earlier proposal.

It is to recover the questions that motivated them and test those questions with better tools.

[Explore the original Hanoi repository](https://github.com/t81dev/Hanoi) · [Research continuity archive](docs/research/continuity/)

---

## Current research priorities

The program's immediate priorities are:

**1. Establish reliable experimental baselines.**  
Verify equivalent semantics and independently reproducible behavior before interpreting performance differences.

**2. Identify discriminating workloads.**  
Select experiments in which alternative representations have a plausible, measurable consequence beyond ordinary encoding changes.

**3. Investigate conditional advantages.**  
Study the circumstances under which representation density, access patterns, conversion costs, or adaptive strategies change the outcome.

**4. Preserve physical hardware questions.**  
Separate limitations of software emulation from the cost and feasibility of native multivalued devices.

**5. Maintain the experimental foundation.**  
Keep the tools, specifications, tests, provenance records, and governance mechanisms reliable enough to support the research.

Active proposals and their acceptance status belong in the research records and pull requests. Their appearance in the repository does not imply formal approval.

[Read the contributor roadmap](docs/ROADMAP.md)

---

## Explore the repository

| Starting point | What you will find |
|---|---|
| [`docs/research/`](docs/research/) | Scientific investigations and experimental conclusions |
| [`experiments/`](experiments/) | Experimental implementations and reproducibility artifacts |
| [`spec/`](spec/) | Instruction-set specifications and RFCs |
| [`include/t81/`](include/t81/) | Core libraries and public interfaces |
| [`vm/`](vm/) | Virtual-machine implementation |
| [`lang/`](lang/) | Language tooling |
| [`kernel/axion/`](kernel/axion/) | Authorization and policy components |
| [`fs/`](fs/) | Artifact identity and storage mechanisms |
| [`docs/ai-context/`](docs/ai-context/) | Continuity material for AI research collaborators |

For build instructions, development dependencies, and available test targets, consult the repository's contributor documentation and CMake configuration. Research results should be reproduced using the environment and commands specified by their individual protocols.

---

## Contributing

T81 welcomes contributions that improve scientific understanding, implementation correctness, reproducibility, or documentation.

Especially valuable contributions include:

- Independent reproduction of existing experiments.
- Stronger conventional binary baselines.
- Mathematical counterexamples and proof corrections.
- Smaller, more discriminating experimental protocols.
- Cross-platform correctness validation.
- Evidence-backed corrections to historical claims.
- Clearly scoped improvements to research infrastructure.

Before proposing a new subsystem, identify which research question it answers and what evidence would establish its usefulness.

Read the [contributor roadmap](docs/ROADMAP.md), repository governance instructions, and relevant experiment protocols before modifying research artifacts.

---

## What T81 does not claim

T81 does not currently claim:

- Universal superiority of ternary computation over binary computation.
- Demonstrated superiority of native ternary silicon.
- A general performance advantage for the integrated T81 architecture.
- That ternary model weights inherently accelerate AI inference.
- That deterministic execution or policy enforcement requires a ternary virtual machine.
- That an AI-generated experiment is independently validated merely because its tests pass.

These boundaries are essential to the research.

They leave room for future discoveries without misrepresenting what has already been established.

---

## The question remains open

Binary computing dominates today's physical and software infrastructure for compelling engineering reasons.

That dominance does not eliminate the value of investigating alternative representations, computational models, or future hardware substrates.

T81's purpose is to determine where those alternatives are useful—and where they are not.

**We are not building a conclusion and searching for evidence to support it.**

**We are building the experimental foundation needed to discover the conclusion.**

---

**T81 Foundation** — Experimental Computing Architecture & Systems Research

[License: Apache 2.0](LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
