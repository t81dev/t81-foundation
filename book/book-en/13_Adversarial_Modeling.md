# Chapter 13: Adversarial Modeling and Determinism Attacks

Determinism can fail through design mistakes, implementation bugs, or deliberate attack. This chapter teaches how to reason about hostile conditions so assurance claims remain credible under pressure.

For onboarding, the core lesson is: strong systems are built assuming adversaries exist, not hoping they do not.

## 13.1 Threat Model

The threat model defines attacker capabilities, target surfaces, trust assumptions, and unacceptable outcomes. It creates a shared frame for risk discussions.

Without a clear threat model, teams either underreact to real risk or overreact to low-impact issues.

## 13.2 Compiler-Level Attacks

Compiler pipelines can introduce nondeterminism or semantic drift through optimization behavior, undefined assumptions, or toolchain variance.

Mitigation onboarding points:

1. pin toolchains where required,
2. validate critical paths with deterministic checks,
3. treat unexplained output variance as an investigation trigger.

## 13.3 VM and GC Attack Vectors

Runtime internals like VM state handling and memory management can become attack surfaces when assumptions are weak or poorly tested.

A mature security posture includes targeted stress tests and deterministic regression checks for these paths.

## 13.4 CanonFS and Hash Attacks

Canonical artifact systems rely on robust hashing and canonicalization rules. Weaknesses here can undermine identity guarantees and replay confidence.

Teach teams to separate "hash exists" from "hash meaningfully guarantees identity under defined assumptions."

## 13.5 Distributed Tier Time-Travel Attack

Distributed systems add timing and ordering risks that can be exploited to create inconsistent interpretations of state progression.

A useful onboarding pattern is scenario-driven review: walk through possible race/order manipulations and map how controls detect or prevent them.

## 13.6 Determinism Breach Postmortem Template

A strong postmortem template standardizes how breaches are analyzed and communicated. It should capture:

1. affected surface and assurance class,
2. trigger and root cause,
3. evidence artifacts,
4. containment/remediation,
5. preventive follow-up actions.

Standardized structure improves organizational learning and avoids repeating the same class of failure.

### Tabletop Exercise

Run a tabletop incident for one hypothetical determinism breach:

1. define attack path,
2. classify severity,
3. specify immediate containment,
4. draft postmortem summary.

This gives teams practical readiness before real incidents occur.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Recognize attack-informed design needs | You can explain why deterministic systems still need threat modeling |
| Integrator | Build mitigations into delivery workflows | You can attach one mitigation test to each threat class |
| Auditor | Assess breach readiness and response quality | You can review postmortem completeness against template fields |

### Worked Example

A compiler upgrade introduces subtle output drift under rare conditions. Threat modeling identifies the class early, and gate coverage prevents silent regression rollout.

### Hands-On Lab

1. Run a tabletop for one determinism breach scenario.
2. Fill the postmortem template with hypothetical evidence.
3. Identify one prevention control to add to CI.

### Expected Outcomes

- You can connect threat analysis to concrete engineering controls.
- You can improve incident quality before a real breach occurs.

### Chapter Summary

You should now understand adversarial modeling as a required discipline for maintaining trustworthy deterministic claims.

### Read Next

Proceed to Chapter 14 for long-term continuity strategies and resilience planning.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 12: Formal Semantics of TISC and T81VM](./12_Formal_Semantics.md)
- [Next: Chapter 14: Continuity and Resilience](./14_Continuity_Resilience.md)

<!-- chapter-nav-end -->
