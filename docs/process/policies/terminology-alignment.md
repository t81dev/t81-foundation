# Terminology Alignment Notes

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Terminology Alignment Notes](#terminology-alignment-notes)
  - [Canonical Terms](#canonical-terms)
  - [Alignment Rules](#alignment-rules)

<!-- T81-TOC:END -->


Snapshot date: 2026-02-08.

This document aligns key terms used across `t81-foundation`, `duotronic-whitepaper`, and `t81-docs`.

## Canonical Terms

| Term | Canonical Reference | Notes |
| --- | --- | --- |
| `T81` | `spec/index.md` and root `README.md` | Ecosystem-level term for the balanced-ternary stack. |
| `T81VM` / `HanoiVM` | `spec/t81vm-spec.md` | Runtime model and deterministic execution semantics. |
| `TISC` | `spec/tisc-spec.md` | Instruction set contract and opcode semantics. |
| `Axion` | `spec/axion-kernel.md` | Policy/safety kernel semantics. |
| `Runtime Contract` | `t81-vm/docs/contracts/vm-compatibility.json` | Executable compatibility baseline for downstream repos. |
| `Normative` | `spec/` | Binding semantics language (`MUST`, `SHOULD`, etc.). |
| `Interpretive` | `docs/`, `duotronic-computing`, `t81-docs` | Explanatory guidance that must defer to normative docs. |

## Alignment Rules

1. Avoid introducing synonym terms for existing canonical concepts without a documented rationale.
2. When a term appears in both normative and interpretive docs, link to the normative source.
3. If terminology conflicts are found in downstream repos, open a cross-repo issue and track resolution in `t81-roadmap`.
