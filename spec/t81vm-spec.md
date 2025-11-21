
---
title: T81 Foundation Specification
nav:
  - [Overview](t81-overview.md)
  - [Data Types](t81-data-types.md)
  - [TISC Specification](tisc-spec.md)
  - [T81 Virtual Machine](t81vm-spec.md)
  - [T81Lang](t81lang-spec.md)
  - [Axion Kernel](axion-kernel.md)
  - [Cognitive Tiers](cognitive-tiers.md)
---

[← Back to Spec Index](t81-overview.md)

# T81 Virtual Machine Specification
Version 0.1 — Draft

The T81VM provides deterministic execution for TISC programs.

---

## 1. Execution Modes
- Reference interpreter  
- Deterministic JIT  

## 2. Determinism Constraints
VM MUST produce identical output across all systems.

## 3. Concurrency Model
- Message-passing  
- Race-free  
- Deterministic scheduling  

## 4. Memory Model
- Deterministic GC  
- Explicit object graph  
- Identity-preserving mutations  

## 5. Axion Interface
- Trace generation  
- Safety hooks  
- Entropy modeling  
- Recursion limits  

---

# Cross-References

## Data Types
- **Primitive Types** → [`t81-data-types.md`](t81-data-types.md#2-primitive-types)  
- **Composite Types** → [`t81-data-types.md`](t81-data-types.md#3-composite-types)  
- **Normalization Rules** → [`t81-data-types.md`](t81-data-types.md#5-normalization-rules)  

## TISC (Ternary Instruction Set)
- **Machine Model** → [`tisc-spec.md`](tisc-spec.md#1-machine-model)  
- **Instruction Encoding** → [`tisc-spec.md`](tisc-spec.md#4-instruction-encoding)  
- **Opcode Classes** → [`tisc-spec.md`](tisc-spec.md#5-opcode-classes)  

## T81 Virtual Machine
- **Execution Modes** → [`t81vm-spec.md`](t81vm-spec.md#1-execution-modes)  
- **Deterministic Concurrency** → [`t81vm-spec.md`](t81vm-spec.md#3-concurrency-model)  
- **Axion Interface Hooks** → [`t81vm-spec.md`](t81vm-spec.md#5-axion-interface)  

## T81Lang
- **Language Properties** → [`t81lang-spec.md`](t81lang-spec.md#1-language-properties)  
- **Grammar** → [`t81lang-spec.md`](t81lang-spec.md#2-grammar)  
- **Type System** → [`t81lang-spec.md`](t81lang-spec.md#3-type-system)  

## Axion Kernel
- **Responsibilities** → [`axion-kernel.md`](axion-kernel.md#1-responsibilities)  
- **Subsystems** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)  
- **Recursion Controls** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)  

## Cognitive Tiers
- **Tier Structure** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)  
- **Constraints** → [`cognitive-tiers.md`](cognitive-tiers.md#2-constraints)  

---
