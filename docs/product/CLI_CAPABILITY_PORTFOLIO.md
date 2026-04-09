# CLI Capability Portfolio (t81)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI Capability Portfolio (t81)](#cli-capability-portfolio-t81)
  - [1. Command Tiers](#1-command-tiers)
    - [Core (public default surface)](#core-public-default-surface)
    - [Advanced (supported expert surface)](#advanced-supported-expert-surface)
    - [Labs (non-default, non-DCP, ops/experimental)](#labs-non-default-non-dcp-opsexperimental)
  - [2. Add / Keep / Remove Decisions](#2-add--keep--remove-decisions)
    - [Add (approved and implemented)](#add-approved-and-implemented)
    - [Keep (approved)](#keep-approved)
    - [Remove or downgrade unless hardened](#remove-or-downgrade-unless-hardened)
  - [3. Promotion / Demotion Rules](#3-promotion--demotion-rules)
  - [4. Global UX Standards (All Tiers)](#4-global-ux-standards-all-tiers)
  - [5. Capability Gaps to Address Before New Expansion](#5-capability-gaps-to-address-before-new-expansion)
  - [6. Sequencing (Design-First)](#6-sequencing-design-first)
    - [Phase A: Design lock (now)](#phase-a-design-lock-now)
    - [Phase B: Core completion](#phase-b-core-completion)
    - [Phase C: Standardization](#phase-c-standardization)
    - [Phase D: Promotion decisions](#phase-d-promotion-decisions)
  - [7. Non-Goals](#7-non-goals)

<!-- T81-TOC:END -->


Status: Draft
Last Updated: 2026-02-26
Owner: Product/Tooling

This document is the pre-implementation capability decision record for `t81`.
No net-new command work should begin unless it is approved here.

## 1. Command Tiers

### Core (public default surface)

- `check`
- `lint`
- `compile`
- `run`
- `disasm`
- `debug`
- `repl`
- `test`
- `doctor`
- `fmt`
- `init`
- `help`
- `version`

### Advanced (supported expert surface)

- `weights`
- `policy`
- `trace`

### Labs (non-default, non-DCP, ops/experimental)

- `benchmark`
- `pkg`
- `repro-hash`
- `canonize-tensor`
- `canonize-file`
- `llama-run`

## 2. Add / Keep / Remove Decisions

### Add (approved and implemented)

1. `test` (Core) - implemented
- Purpose: run project/test workflows from the CLI.
- Minimum bar: deterministic exit codes, support `--json`, docs parity tests.

2. `doctor` (Core) - implemented
- Purpose: environment/toolchain diagnostics and readiness checks.
- Minimum bar: machine-readable report mode (`--json`) and actionable fixes.

3. `fmt` (Core) - implemented
- Purpose: formatting entrypoint for user/project files.
- Minimum bar: dry-run mode, explicit formatter version reporting.

### Keep (approved)

- All current Core and Advanced tier commands above, contingent on contract tests and docs parity.

### Remove or downgrade unless hardened

1. `pkg check` (historically weak/placeholder semantics risk)
- Decision: now implemented with real manifest validation and `--json` mode.

2. Any command/help path that advertises unsupported flags
- Decision: immediate removal from docs/help until implemented.

## 3. Promotion / Demotion Rules

A command can be promoted from Labs -> Advanced -> Core only if all pass:

1. Clear persona + workflow fit.
2. Stable help contract and deterministic exit behavior.
3. Contract tests and docs parity/smoke gates.
4. No placeholder behavior.
5. Explicit support owner.

A command is demoted immediately if:

1. Repeated docs/behavior drift.
2. Placeholder behavior ships to users.
3. CI contract gates are bypassed.

## 4. Global UX Standards (All Tiers)

1. Help forms:
- `t81 --help`
- `t81 help`
- `t81 help <command>`
- `t81 <command> --help`

2. Output channels:
- command result -> `stdout`
- diagnostics/errors -> `stderr`

3. Exit policy:
- `0` success
- `1` user/usage error
- `2+` command-defined failures documented in CLI manual

4. Machine mode:
- `--json` required for commands intended for automation.

## 5. Capability Gaps to Address Before New Expansion

1. Unified `--json` policy coverage across Core and Advanced commands.
2. `pkg` hardening (no placeholder check semantics). Status: complete in current CLI.
3. Global `--no-color` support policy.
4. Command-specific non-zero exit code documentation for all runtime-affecting commands.

## 6. Sequencing (Design-First)

### Phase A: Design lock (now)

1. Finalize this portfolio.
2. Finalize `CLI_V1_UX_DESIGN.md` backlog ordering.

### Phase B: Core completion

1. Implement `doctor`.
2. Implement `test`.
3. Implement `fmt`.

### Phase C: Standardization

1. Expand `--json` consistently.
2. Expand contract tests to new commands.

### Phase D: Promotion decisions

1. Re-evaluate Labs commands for possible promotion based on usage and quality.

## 7. Non-Goals

1. Adding commands solely for feature count.
2. Exposing experimental internals on default help surface.
3. Shipping command aliases that increase ambiguity without clear UX gain.
