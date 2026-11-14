# Runtime Semantics Boundary (Foundation vs VM)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Runtime Semantics Boundary (Foundation vs VM)](#runtime-semantics-boundary-foundation-vs-vm)
  - [Active Runtime Baseline](#active-runtime-baseline)
  - [Ownership Split](#ownership-split)
  - [Source-of-Truth Rule](#source-of-truth-rule)
  - [Required Cross-Repo Fan-Out](#required-cross-repo-fan-out)
  - [High-Risk Mismatch Zones](#high-risk-mismatch-zones)
  - [Verification Rule](#verification-rule)

<!-- T81-TOC:END -->


This note locks the ownership boundary between normative semantics (`t81-foundation`) and executable runtime behavior (`t81-vm`).

## Active Runtime Baseline

- Active tagged runtime baseline: `runtime-contract-v0.5`
- VM contract version: `2026-02-08-v5`
- VM contract commit pin (`t81-vm/main`): `4158a42156a085a2b722205be951576fc01969b9`
- Contract marker in this repo: `contracts/runtime-contract.json`

## Ownership Split

`t81-foundation` owns:

- Normative semantics in `spec/` (language, VM model, ISA meaning, canonical data behavior).
- RFC process for semantics changes (`spec/rfcs/`).
- Architectural intent and invariants that execution must preserve.

`t81-vm` owns:

- Executable implementation of VM behavior.
- Runtime compatibility artifact and host-facing ABI contract:
  - `t81-vm/docs/contracts/vm-compatibility.json`
  - `t81-vm/include/t81/vm/c_api.h`
- Parity/conformance automation and evidence artifacts.

## Source-of-Truth Rule

1. If code behavior and `spec/` differ, treat `spec/` as semantically authoritative.
2. If runtime compatibility details differ across repos, treat `t81-vm/docs/contracts/vm-compatibility.json` as executable compatibility authority.
3. Any proposed change that affects both semantics and runtime must ship as a linked cross-repo change set.

## Required Cross-Repo Fan-Out

Contract-impacting runtime changes must synchronize:

1. `t81-vm` contract artifact/tag and ABI headers.
2. `t81-lang` compatibility gate and docs.
3. `t81-python` bridge compatibility docs/tests.
4. `t81-roadmap` migration status/checkpoint artifacts.

## High-Risk Mismatch Zones

- Opcode semantics and trap classes.
- Numeric conversion/quantization behavior claims.
- Host ABI signatures and struct layout expectations.
- Determinism guarantees used by downstream tooling.

## Verification Rule

- Run `python3 scripts/check-runtime-contract-sync.py` whenever runtime-contract versions are promoted.
- CI enforces marker drift approval (`T81_ALLOW_RUNTIME_CONTRACT_CHANGE=1`) for intentional contract marker updates.
