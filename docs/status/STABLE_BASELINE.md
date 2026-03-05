# Stable Baseline

## Purpose

This document defines the last known-good state of the T81 repository before experimental AI integration work. It establishes a deterministic, reproducible reference point that all future experiments must branch from and must not modify directly.

The baseline exists to protect the integrity of the core system while allowing research and experimentation to continue in isolated branches or directories.

---

## Baseline Identification

| Field            | Value                  |
| ---------------- | ---------------------- |
| Version Tag      | v1.2.1-stable-baseline |
| Branch           | main                   |
| Commit           | 293223a8bdf967dbb9d0a3220b0bc0e89b116cd7 |
| Date Established | 2026-03-04            |
| Established By   | t81dev                 |

---

## Baseline Guarantees

The stable baseline guarantees the following properties:

### 1. Deterministic Execution

All core components produce identical results across repeated runs under identical inputs.

Determinism surfaces include:

* T81VM instruction execution
* Canonical serialization
* Arithmetic primitives
* Core container behaviors

### 2. Successful Build

The repository compiles successfully using supported toolchains.

Supported environments include:

* macOS (clang)
* Linux (gcc/clang)

### 3. Passing Test Suite

All tests in the repository pass without modification.

Test categories include:

* Language conformance
* VM execution
* Core datatype behavior
* Determinism checks

### 4. Clean Repository State

The baseline contains:

* No experimental features merged into core
* No partially implemented subsystems
* No disabled failing tests

---

## Baseline Scope

The stable baseline applies to the following directories:

```
/src
/include/t81
/spec
/tests
/tools
```

These directories represent the **core deterministic stack**.

Changes to these locations require:

* explicit review
* passing CI
* determinism validation

---

## Experimental Work Policy

All experimental features must be isolated under:

```
/experiments
/research
```

Examples include:

* LLM integration
* ternary model quantization
* AI-native opcodes
* hardware acceleration

Experimental code must **not modify the core system** until it passes promotion gates.

---

## Promotion Gates

Before experimental work can be merged into the core system, the following criteria must be met:

1. **Build Stability**

   * Compiles on all CI platforms

2. **Determinism Validation**

   * Identical output hashes across repeated runs

3. **Test Coverage**

   * Existing tests remain unchanged and passing

4. **Benchmark Evidence**

   * Performance or capability improvement is demonstrated

5. **Documentation**

   * Feature documentation and architecture notes are complete

6. **Manual Review**

   * Code review completed

---

## Reproducing the Baseline

Clone the repository and check out the stable tag.

```bash
git clone https://github.com/t81dev/t81-foundation
cd t81-foundation

git checkout v1.2.1-stable-baseline
```

Build the project.

```bash
mkdir build
cd build
cmake ..
make
```

Run the test suite.

```bash
ctest
```

Successful execution confirms the repository matches the baseline state.

---

## Purpose in Project Governance

The stable baseline acts as the **anchor point** for the project.

It ensures:

* reproducibility
* deterministic behavior
* architectural stability

Experimental work may evolve rapidly, but the baseline protects the long-term integrity of the T81 deterministic computing stack.

---

## Future Baseline Updates

A new stable baseline may be established when:

* a major release is completed
* core architecture changes stabilize
* determinism surfaces are expanded

Each baseline should be tagged and documented similarly to this file.

