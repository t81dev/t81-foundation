# T81 Specification

> **Source of Truth:** This directory contains the **normative specifications** for the T81 ecosystem.

**Last Updated:** February 10, 2026

## 1. Normative Language

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHALL NOT**, **SHOULD**, **SHOULD NOT**, **RECOMMENDED**, **MAY**, and **OPTIONAL** in these documents are to be interpreted as described in [RFC 2119](https://tools.ietf.org/html/rfc2119).

## 2. Specification Index

### Core Normative Specs (`spec/*.md`)

| Document | Description |
| :--- | :--- |
| [t81-overview.md](t81-overview.md) | High-level architectural overview |
| [t81-data-types.md](t81-data-types.md) | Canonical data types and ternary representation |
| [tisc-spec.md](tisc-spec.md) | Ternary Instruction Set Architecture (ISA) |
| [t81vm-spec.md](t81vm-spec.md) | T81VM execution model |
| [t81lang-spec.md](t81lang-spec.md) | T81Lang syntax and semantics |
| [axion-kernel.md](axion-kernel.md) | Axion safety kernel and policy enforcement |
| [cognitive-tiers.md](cognitive-tiers.md) | Cognitive tier model and constraints |
| [determinism-profile.md](determinism-profile.md) | Strict Determinism Profile (Tier A) |

### Supplemental Specs (`spec/supplemental/`)

Referenced by core specs and implementation but outside the frozen DCP surface.

| Document | Description |
| :--- | :--- |
| [supplemental/canonfs-spec.md](supplemental/canonfs-spec.md) | CanonFS deterministic filesystem |
| [supplemental/hanoi-kernel-spec.md](supplemental/hanoi-kernel-spec.md) | Hanoi kernel archived reference (non-normative) |
| [supplemental/axion-policy-grammar.md](supplemental/axion-policy-grammar.md) | APL grammar and verification targets |
| [supplemental/constitution.md](supplemental/constitution.md) | Foundational governance principles and invariants |
| [supplemental/cpp-mapping.md](supplemental/cpp-mapping.md) | Legacy-to-current C++ API mapping notes |

## 3. RFC Process (Change Management)

Significant changes to the specification must follow the RFC (Request for Comments) process.
See `rfcs/README.md` for details.

## 4. Historical Context

- **[companion/](companion/):** Contains historical or non-normative companion documents (e.g., `t81-spec.md`).

## 5. Versioning

Specifications are versioned independently of the runtime implementation, but typically align on MAJOR/MINOR numbers. See [VERSIONING.md](../../VERSIONING.md) for the compatibility contract.
