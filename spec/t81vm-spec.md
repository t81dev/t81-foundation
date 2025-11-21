
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

