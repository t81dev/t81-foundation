# Chapter 2: Core Principles and Invariants

Principles become valuable only when they change choices in difficult moments. This chapter explains how T81 principles are applied when speed, convenience, and assurance goals conflict.

## 2.1 The Determinism Invariant

In T81, determinism is treated as a scoped engineering contract. The project intentionally avoids global claims that cannot be verified.

For onboarding, this means you should never ask "is T81 deterministic?" as a yes/no question. You should ask: "which surface, under what conditions, with what evidence?"

That framing may feel strict at first, but it prevents ambiguous communication and reduces incident confusion.

### 2.1.1 Determinism Surfaces and Attack Vectors

A determinism surface is any boundary where repeatability can be tested with confidence. Typical attack/drift vectors include:

* hidden host-dependent behavior,
* ordering instability in transforms,
* ambiguous serialization rules,
* policy assumptions that are not enforced,
* optimization paths without equivalence evidence.

T81's practical defense model is layered:

1. canonical representations,
2. deterministic tests and repro gates,
3. policy enforcement at execution boundaries,
4. explicit governance classification.

No single layer is enough by itself.

## 2.2 Ternary Logic (Base-3)

Ternary in T81 is architectural identity, not a novelty feature. It influences type semantics, encoding discussions, and machine-level thinking.

For new users, the key takeaway is consistency: the system tries to keep conceptual language aligned from high-level programming to lower-level representation.

This reduces cognitive fragmentation during onboarding.

## 2.3 Auditability and the Axion Trace

Determinism without observability is fragile. If behavior diverges and no one can explain where or why, assurance collapses operationally.

Axion traceability is therefore not only a debug aid. It is part of the evidence model for changes, incidents, and release confidence.

## 2.4 The Nine Principles (Ethics Enforcement)

The principle framing keeps correctness tied to accountability. It encourages maintainers to ask not only "does it work" but also "is it controlled, explainable, and scoped honestly?"

In day-to-day engineering, this becomes concrete behavior:

* favor explicit boundary checks over implicit trust,
* fail clearly when policy is violated,
* document assurance class before public claims,
* treat evidence quality as part of product quality.

## 2.5 Verification Checklist

Use this short review template:

1. Claim identified?
2. Surface identified?
3. Assurance class identified?
4. Evidence path identified?
5. Rollback/incident path identified?

If any answer is "no," the work is not finished.

## 2.6 Formal Audit Matrix

Think of the audit matrix as a map from language to proof.

* Narrative says what we want.
* Matrix says where we prove it.

This keeps communication healthy: aspirations remain useful, but guarantees remain disciplined.

### Try It

Pick one claim in this chapter and map it to:

* one source of truth file,
* one verification artifact,
* one owner/team role.

If you cannot do that mapping, refine the claim.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Internalize determinism as an invariant, not a preference | You can name at least three determinism attack surfaces |
| Integrator | Apply principles during feature tradeoffs | You can propose a safe fallback when an optimization risks equivalence |
| Auditor | Map each principle to observable evidence | You can request trace/gate artifacts that support principle claims |

### Worked Example

A new optimization improves throughput but changes one edge-case output. Principle-first reasoning says the optimization cannot be treated as neutral until equivalence is shown or scope is downgraded.

### Hands-On Lab

1. Pick one subsystem and list its determinism surfaces.
2. For each surface, write one likely failure mode and one mitigation.
3. Draft a short audit matrix row with status, evidence, and risk class.

### Expected Outcomes

- You can translate principles into day-to-day engineering decisions.
- You can explain why "faster" and "equivalent" are separate claims.

### Chapter Summary

You should now understand the core T81 habit: scope first, claims second.

### Read Next

Proceed to Chapter 3 for a guided walkthrough of how these principles show up in architecture.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 1: Introduction](./01_Introduction.md)
- [Next: Chapter 3: T81VM Architecture](./03_Architecture.md)

<!-- chapter-nav-end -->
