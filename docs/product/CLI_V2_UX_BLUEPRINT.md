# CLI v2 UX Blueprint (t81)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI v2 UX Blueprint (t81)](#cli-v2-ux-blueprint-t81)
  - [1. North-Star Design](#1-north-star-design)
  - [2. Command Architecture](#2-command-architecture)
    - [2.1 Core User Workflow (default help)](#21-core-user-workflow-default-help)
    - [2.2 Expert Workflow (advanced help)](#22-expert-workflow-advanced-help)
    - [2.3 Internal/Labs Workflow (labs help)](#23-internallabs-workflow-labs-help)
  - [3. What We Should Provide](#3-what-we-should-provide)
  - [4. What We Should Not Provide](#4-what-we-should-not-provide)
  - [5. Gaps We Missed](#5-gaps-we-missed)
  - [6. Capability Decisions (Keep / Move / Remove)](#6-capability-decisions-keep--move--remove)
    - [Keep (as product capabilities)](#keep-as-product-capabilities)
    - [Move to Internal/Labs](#move-to-internallabs)
    - [Remove from Public Contract Now](#remove-from-public-contract-now)
  - [7. Migration Strategy (No Flag Day)](#7-migration-strategy-no-flag-day)
  - [8. Acceptance Bar for “World-Class UX”](#8-acceptance-bar-for-“world-class-ux”)

<!-- T81-TOC:END -->


Status: Proposed
Last Updated: 2026-02-26
Owner: Product/Tooling

This is the design-first target CLI UX for `t81`.
It defines what we should ship, what we should not ship, and how to migrate.

## 1. North-Star Design

Design rule: command names should mirror user intent, not implementation internals.

Canonical grammar:

```text
t81 [global-options] <domain> <action> [args] [options]
```

Global options:
- `-h`, `--help`
- `-V`, `--version`
- `-q`, `--quiet`
- `-v`, `--verbose`
- `--json` (machine mode where supported)

## 2. Command Architecture

### 2.1 Core User Workflow (default help)

- `t81 project init <name>`
- `t81 code check <file.t81>`
- `t81 code fmt <file...>`
- `t81 code build <file.t81|file.t81w> [-o out.tisc]`
- `t81 code run <file.t81|file.tisc>`
- `t81 code test [--filter <regex>]`
- `t81 env doctor`

### 2.2 Expert Workflow (advanced help)

- `t81 code disasm <file.tisc>`
- `t81 code debug <file.t81|file.tisc>`
- `t81 code repl`
- `t81 weights import|info|quantize ...`
- `t81 policy compile|run ...`
- `t81 trace show|diff|replay|export ...`

### 2.3 Internal/Labs Workflow (labs help)

- `t81 internal benchmark ...`
- `t81 internal repro-hash ...`
- `t81 internal canonize-file ...`
- `t81 internal canonize-tensor ...`
- `t81 internal llama-run ...`
- `t81 internal pkg init|check ...`

## 3. What We Should Provide

1. One obvious path for first-run users (`project init -> code check/build/run`).
2. Stable machine mode (`--json`) for automation-facing commands:
- `env doctor`
- `code test`
- `weights info`
- `policy run`
- `trace export`
- `internal pkg check`
3. Deterministic exit classes:
- `0`: success
- `1`: usage/user input error
- `2`: environment/runtime/tool execution failure
- `10+`: domain-specific runtime trap classes (already documented)
4. Consistent help depth:
- `t81 --help`
- `t81 help advanced`
- `t81 help labs`
- `t81 help <domain>`
- `t81 help <domain> <action>`

## 4. What We Should Not Provide

1. Placeholder commands that always “pass” without real validation.
2. Mixed naming styles (`verb`, `noun`, and internal jargon) at top level.
3. Experimental commands on default help output.
4. Hidden behavior changes behind `--verbose` or `--quiet`.
5. Undocumented non-zero exit behavior.
6. Duplicate commands that create ambiguity without real UX benefit.

## 5. Gaps We Missed

1. Domain-first command model (`<domain> <action>`) is not implemented yet.
2. Unified `--json` policy is still partial.
3. A first-class `project` lifecycle beyond init is missing (`project info`, `project validate`).
4. Packaging lifecycle is incomplete (`pkg` has init/check only; no lock/resolve/publish story).
5. Formatter is currently minimal normalization, not language-aware formatting.
6. Testing UX is still tool-wrapper grade, not product-grade reporting.

## 6. Capability Decisions (Keep / Move / Remove)

### Keep (as product capabilities)

- `check`, `compile`, `run`, `disasm`, `debug`, `repl`
- `test`, `doctor`, `fmt`
- `weights`, `policy`, `trace`
- `init`

### Move to Internal/Labs

- `pkg` (until full package lifecycle is implemented)
- `benchmark`
- `repro-hash`
- `canonize-file`
- `canonize-tensor`
- `llama-run`

### Remove from Public Contract Now

- Any command or flag with placeholder semantics.
- Any alias not documented and contract-tested.

## 7. Migration Strategy (No Flag Day)

Phase 1: Introduce domain commands as primary docs path.
- Add `t81 code ...`, `t81 project ...`, `t81 env ...`, `t81 internal ...`.
- Keep existing top-level commands as compatibility aliases.

Phase 2: Emit deprecation hints for old top-level commands.
- Example: `t81 compile` prints hint to use `t81 code build`.

Phase 3: Freeze old forms and keep as long-term aliases only where needed.
- No new features on legacy aliases.
- New features land only on domain-first paths.

## 8. Acceptance Bar for “World-Class UX”

1. New user can ship `init -> build -> run -> test` without reading internals docs.
2. Every automation-facing command has stable JSON schema.
3. Help output is short by default, complete at command depth.
4. Internal/labs commands are discoverable but clearly non-default.
5. Docs parity + smoke + contract tests block merge on drift.
