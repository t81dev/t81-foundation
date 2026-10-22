# Deterministic Corpus Manifest

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Deterministic Corpus Manifest](#deterministic-corpus-manifest)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Corpus Files](#corpus-files)
  - [Verification Command](#verification-command)
  - [DCP Linkage](#dcp-linkage)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Version: 1.0.0
Owner: Status/Governance
Last Updated: 2026-02-25

## Purpose

Track canonical determinism corpus inputs and expected hash references used for
long-term reproducibility assurance.

## Scope

This manifest covers deterministic fixture corpus membership and hash references
for DCP-adjacent determinism checks.

## Corpus Files

Program corpus:

- `tests/fixtures/t81lang_determinism/01_bigint_add.t81`
- `tests/fixtures/t81lang_determinism/02_fraction_sub.t81`
- `tests/fixtures/t81lang_determinism/03_float_literal.t81`
- `tests/fixtures/t81lang_determinism/04_mixed_widen_float.t81`
- `tests/fixtures/t81lang_determinism/05_bool_and_string.t81`
- `tests/fixtures/t81lang_determinism/06_mixed_widen_fraction.t81`
- `tests/fixtures/t81lang_determinism/07_if_else_print.t81`
- `tests/fixtures/t81lang_determinism/08_bounded_loop_print.t81`
- `tests/fixtures/t81lang_determinism/09_nested_if_print.t81`
- `tests/fixtures/t81lang_determinism/10_relation_bool_print.t81`
- `tests/fixtures/t81lang_determinism/11_match_option_some_print.t81`
- `tests/fixtures/t81lang_determinism/12_match_option_guard_print.t81`
- `tests/fixtures/t81lang_determinism/13_match_enum_payload_guard_print.t81`
- `tests/fixtures/t81lang_determinism/14_result_match_guard_print.t81`
- `tests/fixtures/t81lang_determinism/15_bitwise_shift_chain_print.t81`
- `tests/fixtures/t81lang_determinism/16_symbol_equality_branch_print.t81`

Expected hash references:

- `tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt`
- `tests/fixtures/t81lang_determinism/t81lang_ast_ir_repro_hash.txt`

## Verification Command

Run from repository root:

```bash
python3 scripts/ci/t81lang_repro_gate.py
```

Compare generated hashes to committed references listed above.

## DCP Linkage

This corpus supports DCP determinism evidence and must remain consistent with:

- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`

## Cross-References

- `tests/corpus/README.md`
- `docs/governance/INCIDENT_RESPONSE.md`
- `docs/product/RELEASE_DISCIPLINE.md`

## Versioning Statement

Corpus membership or expected hash reference changes require explicit review and
must be reflected in release discipline evidence.
