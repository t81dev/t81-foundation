
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

# T81Lang Specification
Version 0.1 — Draft

A high-level, deterministic, strongly typed programming language.

---

## 1. Language Properties
- Explicit mutability  
- Pure by default  
- No undefined behavior  
- Deterministic concurrency  

## 2. Grammar
UTF-8, strict tokenization, mandatory type annotations.

## 3. Type System
- Affine ownership  
- No implicit conversions  
- Canonical representation rules  

## 4. Error Model
`Result[T, E]` and `Option[T]` instead of exceptions.

## 5. Compilation Pipeline
Source → AST → Semantic Analysis → IR → Axion → TISC  

