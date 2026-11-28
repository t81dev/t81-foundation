# CLI JSON Schema Contracts (t81)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI JSON Schema Contracts (t81)](#cli-json-schema-contracts-t81)
  - [1. Schema IDs](#1-schema-ids)
  - [2. Compatibility Rules](#2-compatibility-rules)
  - [3. Minimum Contract Tests](#3-minimum-contract-tests)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-03-08
Owner: Product/Tooling

This document freezes machine-output JSON contracts for CLI automation.

## 1. Schema IDs

- `t81.doctor.v1` (`t81 env doctor --json`)
- `t81.env-check.v1` (`t81 env check --json`)
- `t81.env-diag.v1` (`t81 env diag --json`)
- `t81.env-toolchain.v1` (`t81 env toolchain --json`)
- `t81.test.v1` (`t81 code test --json`)
- `t81.fmt.v1` (`t81 code fmt --json`)
- `t81.pkg-check.v1` (`t81 internal pkg check --json`)
- `t81.weights-info.v1` (`t81 weights info --json`)
- `t81.weights-verify.v1` (`t81 weights verify --json`)
- `t81.policy-run.v1` (`t81 policy run --json`)
- `t81.policy-validate.v1` (`t81 policy validate --json`)
- `t81.trace-summary.v1` (`t81 trace summary --json`)
- `t81.trace-filter.v1` (`t81 trace filter --json`)
- `t81.trace-export-entry.v1` (`t81 trace export --format json`)
- `t81.feedback.v1` (`t81 feedback submit ...`)
- `t81.feedback-report.v1` (`t81 feedback report`)
- `t81.error.v1` (generic pre-domain failure envelope for `--json` commands)

Canonical command spellings are part of the contract. Historical aliases or
undocumented shorthand forms are not.

## 2. Compatibility Rules

1. Schema IDs are release API.
2. Existing fields in a given `*.v1` schema cannot be removed or renamed.
3. New optional fields may be added without changing the schema ID.
4. Breaking changes require a new schema ID (`*.v2`) and migration notes.
5. Commands that emit `--json` must include a top-level `schema` field.
6. If a command fails before producing a domain-specific payload, it must emit
   `t81.error.v1` on `stdout` and return a non-zero exit code.

## 3. Minimum Contract Tests

CI must validate that each schema ID above is present in command output where
applicable and that canonical command spellings remain valid.
