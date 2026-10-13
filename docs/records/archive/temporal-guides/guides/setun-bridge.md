---
layout: page
title: "Guide: Setun Bridge"
---

# Guide: Setun Bridge (v2)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Guide: Setun Bridge (v2)](#guide-setun-bridge-v2)
  - [Supported Mnemonics](#supported-mnemonics)
  - [Labels](#labels)
  - [Diagnostics](#diagnostics)
  - [Notes](#notes)

<!-- T81-TOC:END -->


The Setun bridge translates a compact Setun-style assembly dialect into TISC `Program` bytecode-ready structures.

API surface:

- `include/t81/setun/bridge.hpp`
- `t81::setun::translate_line(...)`
- `t81::setun::translate_program(...)`
- `t81::setun::translate_program_diagnostic(...)`

## Supported Mnemonics

- `NOP`
- `HALT`
- `LOADI Rdst, imm`
- `MOV Rdst, Rsrc`
- `ADD Rdst, Rsrc`
- `SUB Rdst, Rsrc`
- `LOAD Rdst, addr`
- `STORE addr, Rsrc`
- `JMP target`
- `JZ Rcond, target`
- `JNZ Rcond, target`
- `JN target`
- `JP target`

`target` accepts numeric PC values or symbolic labels in program translation mode.

## Labels

Labels use `name:` syntax.

Example:

```asm
start:
LOADI R1, 3
JNZ R1, start
HALT
```

Rules:

- Label syntax: `[A-Za-z_][A-Za-z0-9_]*`
- Duplicate labels are rejected deterministically.
- Undefined labels are rejected deterministically.

## Diagnostics

Use `translate_program_diagnostic` for line/column mapped errors:

- `error` (`BridgeError`)
- `line` (1-based)
- `column` (1-based)
- `message`
- `source_line`

This is the recommended API for CLI tooling and editor integrations.

## Notes

- `translate_line` is intentionally single-line and does not resolve symbolic labels.
- Comments are supported via `;` or `#`.
