# CLI v1 UX Design (t81)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI v1 UX Design (t81)](#cli-v1-ux-design-t81)
  - [1. Design Goal](#1-design-goal)
  - [2. Primary Personas](#2-primary-personas)
  - [3. UX Principles (v1)](#3-ux-principles-v1)
  - [4. Experience Model](#4-experience-model)
    - [4.1 Command Families](#41-command-families)
    - [4.2 Help Model](#42-help-model)
    - [4.3 Output Model](#43-output-model)
    - [4.4 Exit Model](#44-exit-model)
  - [5. Workflow Design](#5-workflow-design)
    - [5.1 First 5 Minutes (critical path)](#51-first-5-minutes-critical-path)
    - [5.2 Debugging Path](#52-debugging-path)
    - [5.3 CI/Automation Path](#53-ciautomation-path)
  - [6. Current vs Desired (Command-Level)](#6-current-vs-desired-command-level)
    - [6.1 Core Commands](#61-core-commands)
    - [6.2 Command Families](#62-command-families)
    - [6.3 Global Behavior](#63-global-behavior)
  - [7. What We Failed Before (Retrospective)](#7-what-we-failed-before-retrospective)
  - [8. Backlog (Prioritized)](#8-backlog-prioritized)
    - [P0 (Must ship for v1 readiness)](#p0-must-ship-for-v1-readiness)
    - [P1 (Should ship in same release train)](#p1-should-ship-in-same-release-train)
    - [P2 (Post-v1 enhancements)](#p2-post-v1-enhancements)
  - [9. Delivery Plan](#9-delivery-plan)
  - [10. Acceptance Criteria for "World-Class v1"](#10-acceptance-criteria-for-"world-class-v1")

<!-- T81-TOC:END -->


Status: Draft
Last Updated: 2026-02-26
Owner: Product/Tooling

This document defines the intended user experience for `t81` CLI v1.
It complements `CLI_UX_PRODUCT_SPEC.md` (contract/quality bar) with explicit
experience design decisions and implementation priorities.

For the from-scratch domain-first redesign target, see:

- `docs/product/CLI_V2_UX_BLUEPRINT.md`

Capability scope and tiering decisions are frozen in:

- `docs/product/CLI_CAPABILITY_PORTFOLIO.md`

## 1. Design Goal

Make `t81` feel like a professional developer tool, not a research shell.
Users should be able to:

1. Find the right command quickly.
2. Recover from errors without guessing.
3. Script workflows without parsing unstable text.

## 2. Primary Personas

1. First-run developer:
Needs successful `check -> compile -> run` in under 3 minutes.
2. Daily language/runtime contributor:
Needs fast feedback, deterministic diagnostics, and consistent flags.
3. CI/release automation:
Needs stable exit codes, channel semantics, and parse-safe output.

## 3. UX Principles (v1)

1. Progressive disclosure:
Top-level help stays short; command help is complete and actionable.
2. Error-to-action:
Every user error should point to a likely correction.
3. Consistency over cleverness:
Similar command families (`weights/policy/trace/pkg`) behave similarly.
4. Automation-safe:
No ambiguous success text; no critical info only on stderr for success paths.
5. Explicit stability:
Documented command contracts are treated as release API.

## 4. Experience Model

### 4.1 Command Families

- Core compile/execute: `check`, `lint`, `compile`, `run`, `disasm`, `debug`, `repl`
- Governance/repro: `policy`, `trace`, `repro-hash`
- Artifacts/data: `weights`, `canonize-*`
- Project ops: `init`, `pkg`, `benchmark`
- Meta: `help`, `version`

### 4.2 Help Model

Supported forms:

- `t81 --help`
- `t81 help`
- `t81 help <command>`
- `t81 <command> --help`

Design requirements:

- Unknown help topic is an error (non-zero).
- Command help must include `Usage`, key args, and one short intent sentence.

### 4.3 Output Model

- `stdout`: command results and machine-consumable payloads.
- `stderr`: errors/warnings/diagnostics.
- `--quiet`: suppresses non-error informational text only.
- `--verbose`: additive diagnostics only; no semantic behavior changes.

### 4.4 Exit Model

- `0`: success
- `1`: usage/user error
- `2+`: command-defined execution failure classes (must be documented)

## 5. Workflow Design

### 5.1 First 5 Minutes (critical path)

1. `t81 --help`
2. `t81 check <file.t81>`
3. `t81 compile <file.t81> -o <file.tisc>`
4. `t81 run <file.tisc>`

Design bar:

- No hidden prerequisites in command output.
- Errors must include file/line/column where applicable.

### 5.2 Debugging Path

1. `t81 check` for diagnostics
2. `t81 disasm` to inspect emitted program
3. `t81 debug` for runtime inspection
4. `t81 trace ...` for post-run analysis

### 5.3 CI/Automation Path

1. Deterministic exit behavior
2. Stable help/usage text contracts
3. Docs-parity + smoke checks as CI gates

## 6. Current vs Desired (Command-Level)

### 6.1 Core Commands

| Command | Current | v1 Target | Priority |
| :--- | :--- | :--- | :--- |
| `compile` | Functional, now has help | Add examples in help output | P1 |
| `run` | Functional; output channel fixed | Add explicit runtime failure code map in help/docs | P1 |
| `check/lint` | Functional; good diagnostics | Keep alias behavior identical and documented | P1 |
| `disasm` | Functional | Add concise summary footer option (future) | P2 |
| `debug` | Functional | Improve discoverability of debugger commands | P2 |
| `repl` | Functional, command-rich | Add `repl --help` command list banner | P2 |
| `test` | Implemented (`ctest` wrapper) | Expand structured summary detail over time | P1 |
| `doctor` | Implemented | Expand checks (toolchain + policy-specific readiness) | P1 |
| `fmt` | Implemented (deterministic whitespace normalization) | Add richer language-aware formatting later | P2 |

### 6.2 Command Families

| Family | Current | v1 Target | Priority |
| :--- | :--- | :--- | :--- |
| `weights` | Good subcommand model | Add consistent missing-arg hints with examples | P1 |
| `policy` | Good subcommand model | Add validation summary schema for CI use | P2 |
| `trace` | Good utilities | Add `trace --help` contextual examples | P1 |
| `pkg/init` | Functional, validation asymmetry history | Align name-validation posture and document it | P0 |
| `canonize-*` | Functional | Add explicit reproducibility notes in help | P2 |

### 6.3 Global Behavior

| Area | Current | v1 Target | Priority |
| :--- | :--- | :--- | :--- |
| Global flags | Now accepted before/after command | Keep frozen and test-guarded | P0 |
| Help semantics | Now mostly normalized | Ensure all commands have dedicated help pages | P0 |
| Error style | Standard `error:` prefix | Add consistent remediation hint suffix | P1 |
| Docs parity | Enforced in CI | Keep mandatory on all release branches | P0 |

## 7. What We Failed Before (Retrospective)

1. Added features without a stable interaction contract.
2. Allowed docs to describe non-shipping flags.
3. Let help UX drift behind command growth.
4. Under-tested user-facing behavior (tests focused on internals).
5. Optimized for throughput over usability consistency.

## 8. Backlog (Prioritized)

### P0 (Must ship for v1 readiness)

1. Command family consistency pass for `init/pkg` validation + docs sync.
2. Help coverage completion for every top-level command and key subcommands.
3. Keep CI gates mandatory: contract tests, docs parity, docs smoke.

### P1 (Should ship in same release train)

1. Add actionable examples to command help output.
2. Normalize all error messages to include correction hints.
3. Add docs section mapping non-zero exit codes for runtime commands.

### P2 (Post-v1 enhancements)

1. Structured output mode (`--json`) for selected commands.
2. `--no-color` and terminal capability handling for trace rendering.
3. Shell completion generation and install docs.

## 9. Delivery Plan

1. Design lock (this document + product spec): complete.
2. Behavior lock (tests/CI gates): in progress.
3. Consistency release (P0 + selected P1): next milestone.
4. Ergonomics expansion (P2): following milestone.

## 10. Acceptance Criteria for "World-Class v1"

1. A new contributor can complete first-run path without docs confusion.
2. No unsupported flags appear in CLI docs.
3. Help quality and exit behavior are deterministic across commands.
4. CI catches docs/UX drift before merge.
5. User-facing errors consistently point to corrective action.
