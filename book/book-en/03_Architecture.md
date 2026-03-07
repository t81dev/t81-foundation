# Chapter 3: T81VM Architecture

Architecture docs often become abstract diagrams with little onboarding value. This chapter takes the opposite approach: it explains architecture as a practical flow you can reason about while building and debugging.

## 3.1 Overview

T81 architecture is a controlled flow from intent to evidence:

1. source is authored and compiled,
2. machine semantics execute that intent,
3. policy checks constrain execution,
4. traces/artifacts record what happened,
5. governance classifies what can be claimed.

This layered flow is what makes T81 teachable and operable under assurance constraints.

### Architecture Map

```mermaid
flowchart LR
    S[Source + Compile] --> V[TISC + VM Execution]
    V --> P[Policy Enforcement]
    P --> T[Trace + Canonical Artifacts]
    T --> G[Governance Claim Scope]
```

## 3.2 The Runtime Boundary

The runtime boundary is where many systems become ambiguous. T81 tries to keep it explicit:

* explicit inputs (program, policy, artifacts),
* explicit outputs (result, trap, trace context),
* explicit side-effect control.

Onboarding guidance: when diagnosing behavior, first verify boundary inputs before assuming internal bugs.

## 3.3 Memory Model

The memory strategy in T81 is aimed at predictable interpretation and failure clarity.

For new users, the practical lesson is this: memory behavior should not depend on incidental host details in ways that change semantic outcomes.

When things go wrong, failure should be traceable and explicit, not silent.

## 3.4 The Instruction Set (TISC)

TISC is the machine contract between compiler output and runtime execution.

Teaching value:

* it gives a stable language for discussing semantics,
* it anchors tests and tooling around shared meaning,
* it helps teams evaluate changes as explicit semantic choices.

## 3.5 JIT Compilation (Trace-JIT)

JIT is where performance gains can pressure assurance boundaries.

T81's stance for onboarding is simple: optimization exists, but stronger guarantee claims require equivalence evidence and explicit promotion. "Fast" is not automatically "equivalent." 

### Walkthrough Scenario

Imagine a feature runs faster after a runtime optimization. Before celebrating, a T81-aligned process asks:

1. Did behavior remain equivalent on the targeted surface?
2. Are traces/results consistent with expectation?
3. Has the assurance classification changed or stayed the same?

This discipline prevents accidental trust regressions.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Understand intent-to-evidence flow across layers | You can describe all five architecture layers from memory |
| Integrator | Locate boundary ownership for debugging and change impact | You can identify which layer owns a given failure signal |
| Auditor | Verify that architectural claims align with evidence trails | You can trace one behavior from source intent to recorded artifact |

### Worked Example

A run fails under policy on one environment but not another. Architecture-based debugging starts at boundary inputs (policy, artifact identity, runtime context) before suspecting ISA semantics.

### Hands-On Lab

1. Draw the five-layer chain for one sample program.
2. Mark input/output boundaries at each layer.
3. Add one potential failure and one verification point per layer.

### Cross-Chapter Continuity

Continue with `Lab A: Deterministic Baseline Pipeline` in [README](./README.md#cross-chapter-end-to-end-labs) to connect this model to build, run, and verification evidence.

### Expected Outcomes

- You can debug by boundary rather than guesswork.
- You can communicate architecture with operational clarity.

### Chapter Summary

You should now see architecture as a chain of accountable boundaries, not just components.

### Read Next

Proceed to Chapter 4 to understand how type and serialization choices affect reproducibility.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 2: Core Principles and Invariants](./02_Principles.md)
- [Next: Chapter 4: Data Types and Serialization](./04_Data_Types_and_Serialization.md)

<!-- chapter-nav-end -->
