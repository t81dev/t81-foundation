# Chapter 5: Installation and Build Verification

This chapter teaches installation as reproducible setup. In T81, build behavior is part of what you can later claim, so setup discipline matters.

## 5.1 Prerequisites

Prerequisites are not housekeeping. They are part of evidence quality.

If toolchain assumptions are undocumented, reproducibility troubleshooting becomes expensive and slow.

Onboarding advice: start from documented versions and avoid invisible local customizations until baseline checks pass.

## 5.2 Building from Source

A clean build flow should be your default habit:

1. configure from a clean state,
2. build with explicit parameters,
3. keep commands scriptable and replayable.

This makes comparison and support much easier across team members.

## 5.3 Verifying the Build

For T81, compile success alone is insufficient for assurance claims. Verification should include deterministic checks and governance-aligned checks based on the targeted surface class.

Teaching rule: "it builds" is a starting milestone, not the finish line.

## 5.4 Troubleshooting

Use structured triage when behavior diverges:

1. clean/rebuild,
2. rerun relevant gates,
3. compare policy/config parity,
4. classify impact by assurance class.

This sequence avoids unproductive guesswork.

### First 60-Minute Onboarding Exercise

1. Build from clean source.
2. Run baseline checks.
3. Execute one controlled program with explicit policy.
4. Record what artifacts/evidence you used to confirm expected behavior.

If you can do this once, you have a reliable foundation for the rest of the book.

### Role-Based Learning Path

| Role | Focus In This Chapter | You Are Ready When |
| --- | --- | --- |
| New User | Build from clean state with confidence | You can reproduce the same build twice from scratch |
| Integrator | Keep setup scriptable and comparable across environments | You can hand another engineer one command path that works unchanged |
| Auditor | Confirm setup evidence quality | You can identify missing provenance details in a build report |

### Worked Example

An onboarding user compiles successfully but cannot reproduce gate results later. The root issue is undocumented toolchain variance, not runtime semantics.

### Hands-On Lab

1. Perform a clean build and capture tool versions.
2. Run baseline tests and determinism gate.
3. Rebuild from clean state and compare outputs.

### Expected Outcomes

- You can distinguish build success from verification success.
- You can produce a minimal reproducible setup record.

### Chapter Summary

You should now have an operational mindset: setup and verification are part of trust, not separate from it.

### Read Next

Proceed to Chapter 6 for practical CLI/API usage patterns.

<!-- chapter-nav-start -->

---

**Navigation**

- [Book Index](./README.md)
- [Previous: Chapter 4: Data Types and Serialization](./04_Data_Types_and_Serialization.md)
- [Next: Chapter 6: CLI and API Usage](./06_Usage.md)

<!-- chapter-nav-end -->
