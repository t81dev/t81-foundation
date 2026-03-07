# Chapter 1: Introduction

If this is your first contact with T81, you should read this chapter as an orientation map, not a policy document. The purpose here is to answer three practical onboarding questions:

1. What is T81 trying to optimize for?
2. How is T81 different from a normal language/runtime stack?
3. How should I read the rest of this book without confusing narrative with normative guarantees?

This chapter is non-normative by design. It teaches context and intent. Normative contracts remain in `/spec` and governance documents.

## 1.1 Scope and Definition

T81 is a deterministic computing platform with explicit governance boundaries. That sentence has two important parts.

First, "deterministic computing platform" means T81 is not only a language or only a VM. It is a coordinated stack of tooling, execution semantics, policy enforcement, and evidence workflows.

Second, "explicit governance boundaries" means claims are intentionally scoped. The project avoids blanket promises like "everything is deterministic." Instead, guarantees are attached to specific surfaces with explicit verification status.

For onboarding, this is the mindset to keep: T81 values explainability and controlled behavior as much as feature breadth.

**New-user checkpoint**:

* If you expect a "just run it and trust it" experience, T81 will feel strict.
* If you want auditable behavior and controlled guarantees, that strictness is the point.

## 1.2 System Architecture

T81 is easiest to remember as a five-layer chain:

1. Source and tooling layer (authoring and compilation)
2. VM/ISA layer (machine behavior)
3. Policy layer (what execution is permitted)
4. Canonical artifact layer (stable identity)
5. Governance layer (what guarantee level is claimable)

Each layer answers a different question:

* Tooling: "What did we mean to run?"
* VM/ISA: "What did the machine do?"
* Policy: "What was allowed?"
* Canonicalization: "What exact artifact is this?"
* Governance: "What can we responsibly promise?"

When these layers stay separate, onboarding becomes clearer and operations become more reliable.

## 1.3 Verifiable Compute Mission

T81 is not aiming to be the default runtime for every problem. It is aiming to be a strong option when output legitimacy matters as much as output availability.

Example onboarding scenario:

* A team runs the same pipeline on two environments.
* Results diverge.
* In a typical stack, this can become a long forensic exercise.
* In a T81-oriented workflow, the team is expected to compare policy context, artifact identity, and trace/gate outputs quickly.

The mission is not "zero bugs." The mission is "bounded, explainable behavior under explicit constraints."

## 1.4 Terminology

You should be fluent in a few terms before moving forward:

* **DCP**: Deterministic Core Profile (release-certified deterministic subset)
* **Determinism Surface**: boundary where deterministic behavior is validated
* **Governed non-DCP**: policy-controlled surface outside DCP deterministic certification
* **CanonFS**: content-addressed artifact identity workflow
* **Promotion**: process to move a surface toward stronger assurance classification

If you remember only one onboarding rule, remember this: do not read these terms as marketing words; read them as operational classifications.

## 1.5 Verification Checklist

Use this checklist when reading every chapter:

1. Is this narrative explanation or normative requirement?
2. What exact surface is being discussed?
3. What assurance class is that surface in right now?
4. What evidence path supports the claim?
5. What is explicitly outside scope?

### Try It

Run this quick orientation exercise:

1. Open `README.md` (root) and identify DCP/gov boundary statements.
2. Open `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` and find one Verified and one Partial surface.
3. Confirm you can explain the difference in one sentence each.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Understand scope, boundaries, and guarantees | You can explain DCP vs non-DCP in plain language |
| Integrator | Identify where implementation claims must map to evidence | You can point to the exact governance docs for one claim |
| Auditor | Separate narrative language from normative requirements | You can classify one statement as informative vs binding |

### Worked Example

A teammate says, "T81 is deterministic everywhere." Use this chapter to correct the claim: deterministic guarantees are scoped to verified surfaces and DCP boundaries, while other areas remain governed but not equivalently certified.

### Hands-On Lab

1. Open root `README.md` and highlight three scope-limiting statements.
2. Open `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` and select one Verified and one Partial surface.
3. Write two one-sentence claim statements, each with explicit scope.

### Expected Outcomes

- You can state what is guaranteed, where, and under what conditions.
- You can avoid over-claiming determinism in onboarding conversations.

### Chapter Summary

You should now understand that T81 is a scoped-assurance platform, not an all-or-nothing determinism claim.

### Read Next

Proceed to Chapter 2 to learn how principles translate into engineering behavior during real tradeoffs.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Next: Chapter 2: Core Principles and Invariants](./02_Principles.md)

<!-- chapter-nav-end -->
