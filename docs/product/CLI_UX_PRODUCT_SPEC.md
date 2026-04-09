# CLI UX Product Spec (t81)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI UX Product Spec (t81)](#cli-ux-product-spec-t81)
  - [1. Product Goal](#1-product-goal)
  - [2. Non-Negotiable UX Principles](#2-non-negotiable-ux-principles)
  - [3. CLI Contract (Normative)](#3-cli-contract-normative)
    - [3.1 Command Grammar](#31-command-grammar)
    - [3.2 Help and Discoverability](#32-help-and-discoverability)
    - [3.3 Exit Code Policy](#33-exit-code-policy)
    - [3.4 Output Channel Policy](#34-output-channel-policy)
    - [3.5 Error Message Style](#35-error-message-style)
  - [4. Scope for Current Hardening Cycle](#4-scope-for-current-hardening-cycle)
  - [5. Quality Gates (Definition of Done)](#5-quality-gates-definition-of-done)
  - [6. Metrics and Targets](#6-metrics-and-targets)
  - [7. Phased Execution Plan](#7-phased-execution-plan)
    - [Phase 0 (Week 1): Contract Lock](#phase-0-week-1-contract-lock)
    - [Phase 1 (Week 2): Test and CI Enforcement](#phase-1-week-2-test-and-ci-enforcement)
    - [Phase 2 (Week 3): Docs and Onboarding Rewrite](#phase-2-week-3-docs-and-onboarding-rewrite)
  - [8. Known Prior Failure Modes (Must Not Repeat)](#8-known-prior-failure-modes-must-not-repeat)
  - [9. Ownership](#9-ownership)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-02-26
Owner: Product/Tooling

This document defines the product contract for the `t81` CLI user experience.
It is the UX source of truth for command behavior, output behavior, and docs parity.

Design companion:

- `docs/product/CLI_V1_UX_DESIGN.md` (personas, workflow design, and prioritized
  UX redesign backlog).

## 1. Product Goal

Deliver a world-class CLI that is:

- Fast to learn on first run.
- Predictable under failure.
- Script-safe in automation.
- Stable across releases.

## 2. Non-Negotiable UX Principles

1. One intent, one behavior: same command shape must always produce same class of output and exit code.
2. Discoverability by default: every command and subcommand must have actionable help.
3. Scriptability first: machine-safe output and deterministic exit codes are mandatory.
4. No docs fiction: docs may only describe behavior that exists in the shipped binary.
5. Backward compatibility: existing documented command contracts are versioned and protected.

## 3. CLI Contract (Normative)

### 3.1 Command Grammar

Canonical grammar:

```text
t81 [global-options] <command> [command-options] [args]
t81 <command> [global-options] [command-options] [args]
```

Global options:
- `-h`, `--help`
- `-V`, `--version`
- `-q`, `--quiet`
- `-v`, `--verbose`

Global options MUST be accepted before or after the command token.

### 3.2 Help and Discoverability

The following MUST work:

- `t81 --help`
- `t81 help`
- `t81 help <command>`
- `t81 <command> --help`

Unknown help topic MUST return non-zero.

### 3.3 Exit Code Policy

- `0`: success.
- `1`: user error (bad args, missing files, unknown command/help topic).
- `2+`: runtime/tool-specific failures only if explicitly documented.

Every help path MUST return `0`, except unknown help topic.

### 3.4 Output Channel Policy

- Primary command result MUST go to `stdout`.
- Diagnostics, warnings, and errors MUST go to `stderr`.
- `--quiet` suppresses non-error informational output.
- `--verbose` enables additional diagnostic output without changing command semantics.

### 3.5 Error Message Style

Errors MUST:

- Start with `error:`.
- Name the failing argument/command when possible.
- Include correction hint where obvious.

Example:
`error: Unknown command 'foo'. Run 't81 --help' to list commands.`

## 4. Scope for Current Hardening Cycle

In scope:

- Help behavior normalization.
- Global option parsing normalization.
- Exit code normalization.
- stdout/stderr normalization.
- Docs-to-binary parity enforcement.

Out of scope (until hardening is complete):

- New top-level commands.
- New major option families.
- Breaking command renames.

## 5. Quality Gates (Definition of Done)

A CLI release candidate is not shippable unless all are true:

1. Help contract tests pass for every top-level command.
2. Unknown command/help topic tests return non-zero.
3. Global options position tests pass (`t81 -q version`, `t81 version -q`, etc.).
4. Output channel tests pass for success and failure paths.
5. Docs parity check passes: every documented flag/command exists and executes in smoke mode.

## 6. Metrics and Targets

Hard targets per release:

- Help coverage: 100% top-level commands, 100% subcommands.
- Docs parity drift: 0 unsupported documented flags.
- CLI regression escape count: 0 P1 regressions on command contract.
- Median time-to-first-success (new user quickstart): <= 3 minutes.

## 7. Phased Execution Plan

### Phase 0 (Week 1): Contract Lock

- Freeze command surface.
- Implement parser changes for true global options.
- Implement command-specific help dispatch.
- Normalize unknown help topic exit code.

### Phase 1 (Week 2): Test and CI Enforcement

- Add golden tests for help and error output.
- Add CLI contract test matrix for exit codes and channels.
- Add docs parity checker to CI.

### Phase 2 (Week 3): Docs and Onboarding Rewrite

- Rewrite CLI user manual to only documented-live features.
- Add "first 5 minutes" quickstart that is continuously tested.
- Remove stale/deprecated examples that no longer execute.

## 8. Known Prior Failure Modes (Must Not Repeat)

1. Help topics listed but not actually reachable.
2. Global flags documented but command-position dependent.
3. Docs claiming flags not supported by parser.
4. Inconsistent validation rules between similar commands.
5. Output written to wrong stream for common workflows.

## 9. Ownership

- Product owner: CLI contract and release bar.
- Tooling owner: parser/dispatcher behavior and tests.
- Docs owner: docs parity and quickstart correctness.
- CI owner: enforcement integration and break-glass policy.

No release may claim CLI quality completion without all owners signing off.
