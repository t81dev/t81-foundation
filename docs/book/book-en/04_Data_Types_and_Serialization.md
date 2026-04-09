# Chapter 4: Data Types and Serialization

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Chapter 4: Data Types and Serialization](#chapter-4-data-types-and-serialization)
  - [4.1 Primitive Types](#41-primitive-types)
  - [4.2 T81Float and dmath](#42-t81float-and-dmath)
  - [4.3 Tensors and Canonical Layouts](#43-tensors-and-canonical-layouts)
  - [4.4 Canonical Serialization Rules](#44-canonical-serialization-rules)
    - [Try It](#try-it)
    - [Role-Based Learning Path](#role-based-learning-path)
    - [Worked Example](#worked-example)
    - [Hands-On Lab](#hands-on-lab)
    - [Expected Outcomes](#expected-outcomes)
    - [Chapter Summary](#chapter-summary)
    - [Read Next](#read-next)

<!-- T81-TOC:END -->


In deterministic systems, representation details are system behavior. This chapter explains why T81 treats data shape, numeric meaning, and serialization rules as assurance infrastructure.

## 4.1 Primitive Types

Primitive types are not just storage formats. They are contracts that influence how values are interpreted across compile, run, serialize, and compare phases.

For onboarding, this means type choice is not merely style. Type choice can affect reproducibility, trace interpretation, and artifact identity.

## 4.2 T81Float and dmath

Floating-point drift is one of the most common sources of cross-environment inconsistency.

T81 addresses this by tying numeric behavior claims to explicit verification scope and boundary classification. New users should avoid assumptions based on a single host run and instead follow evidence paths.

## 4.3 Tensors and Canonical Layouts

Model and tensor paths multiply drift risk because they combine large data, conversion pipelines, and external artifacts.

T81 narrative guidance for onboarding:

* keep layout expectations explicit,
* bind artifacts to canonical identity workflows,
* use policy controls for sensitive loading/execution paths,
* classify guarantee strength before public claims.

## 4.4 Canonical Serialization Rules

Canonical serialization is what makes "same meaning" verifiable as "same bytes/hash identity" where required.

Without canonicalization, evidence quality drops quickly. With canonicalization, cross-run comparison becomes practical.

### Try It

Choose one sample artifact and answer:

1. What gives this artifact identity in your workflow?
2. How would you detect identity drift?
3. Which surface classification governs guarantees for this path?

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Understand why canonical form matters | You can explain why two equivalent values need one serialization identity |
| Integrator | Prevent representation drift across components | You can design data contracts that preserve canonical behavior |
| Auditor | Validate artifact identity assumptions | You can test that canonicalization is stable across repeat runs |

### Worked Example

Two teams exchange numerically equivalent payloads that hash differently because layout rules were inconsistent. Canonical serialization eliminates this ambiguity and restores reproducible comparison.

### Hands-On Lab

1. Serialize the same logical object from two code paths.
2. Compare byte outputs and hashes.
3. Normalize to canonical rules and repeat.

### Expected Outcomes

- You can detect representation drift quickly.
- You can justify canonicalization as an assurance control, not a formatting choice.

### Chapter Summary

You should now understand that representation discipline is foundational to reproducibility, not a formatting preference.

### Read Next

Proceed to Chapter 5 for practical build and verification workflow guidance.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 3: T81VM Architecture](./03_Architecture.md)
- [Next: Chapter 5: Installation and Build Verification](./05_Installation.md)

<!-- chapter-nav-end -->
