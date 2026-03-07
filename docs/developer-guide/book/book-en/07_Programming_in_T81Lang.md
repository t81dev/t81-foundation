# Chapter 7: Programming in T81Lang

T81Lang is designed to keep programs readable, auditable, and predictable under governed execution. This chapter teaches the language as a practical craft: write code that humans can reason about and systems can verify.

For onboarding, the central shift is this: correctness is not only "does it run," but also "can we explain why it behaved this way under explicit constraints?"

## 7.1 Design Philosophy

T81Lang favors explicit semantics over clever shorthand. That choice improves auditability and reduces hidden behavior that would otherwise create cross-environment surprises.

When mentoring new contributors, emphasize this rule: if a construct hides control flow or state transitions too aggressively, it probably conflicts with deterministic maintainability.

## 7.2 Syntax Basics

The syntax is intentionally approachable so learners can focus on meaning rather than novelty. Clarity in code structure is treated as an operational asset.

Good onboarding pattern:

1. start with tiny examples,
2. predict behavior before running,
3. confirm results,
4. document why prediction matched execution.

This trains precise mental models early.

## 7.3 Data Types

Type decisions in T81Lang affect more than storage. They influence serialization identity, runtime interpretation, and downstream policy interaction.

Teach types as contracts:

1. what values are represented,
2. what operations are valid,
3. what evidence expectations follow from using them.

Teams that treat types this way produce fewer ambiguous bugs.

## 7.4 Control Flow

Control flow constructs are readable by design, but runtime policy still governs what is permitted. A branch can be syntactically valid and still constrained during execution.

This is an important onboarding lesson: language-level possibility is not the same as policy-authorized behavior.

## 7.5 Functions

Functions are the main unit for creating testable intent boundaries. Clear function contracts make trace interpretation easier and reduce accidental side effects.

Teaching practice that works well:

1. write function with explicit purpose,
2. define expected output for fixed input,
3. run under controlled context,
4. confirm behavior and trace alignment.

## 7.6 Structures and Methods

Structures model domain concepts explicitly, which helps maintain readability as projects grow. Methods should preserve clear ownership and avoid hidden coupling.

Onboarding heuristic: if a structure cannot be described in one sentence with clear responsibilities, it probably needs to be split.

## 7.7 Axion Integration

Axion integration should be taught as part of normal programming, not as a late-stage add-on. Policy-aware development prevents teams from confusing intentional denials with random runtime instability.

Preferred authoring loop:

1. write code,
2. compile/check,
3. run with explicit policy,
4. review outcomes and trace context.

This loop helps new users develop correct expectations quickly.

## 7.8 Examples

Examples should form a progression from simple to realistic. A strong onboarding sequence includes:

1. deterministic arithmetic/data transformations,
2. structured control flow with clear branch expectations,
3. policy-constrained scenarios,
4. trace-backed explanation of observed results.

The goal is to teach habits that scale, not tricks that only work in toy cases.

### Practice Path

Build a short example portfolio with three programs:

1. one pure computation example,
2. one policy-sensitive behavior example,
3. one intentionally failing example where you explain the failure via trace.

This portfolio becomes a useful internal onboarding artifact for teams.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Write clear, predictable T81Lang functions | You can predict output for fixed inputs before running |
| Integrator | Build maintainable modules with explicit contracts | You can describe side effects and policy touchpoints per function |
| Auditor | Review code for explainability and determinism | You can identify hidden behavior risks in control flow and types |

### Worked Example

A concise but opaque helper function passes tests, yet fails audit review because side effects are implicit. Refactoring into explicit function boundaries improves trace interpretation and reliability.

### Hands-On Lab

1. Write a small program with two functions and one structured type.
2. Add one policy-sensitive operation.
3. Document expected behavior and validate via run + trace.

### Expected Outcomes

- You can code in a style that survives audit and maintenance.
- You can connect language design choices to runtime trust outcomes.

### Chapter Summary

You should now understand T81Lang as a language for explainable, policy-aware deterministic programming.

### Read Next

Proceed to Chapter 8 to learn how verification and audit convert engineering behavior into defendable assurance claims.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 6: CLI and API Usage](./06_Usage.md)
- [Next: Chapter 8: Verification and Audit](./08_Verification_and_Audit.md)

<!-- chapter-nav-end -->
