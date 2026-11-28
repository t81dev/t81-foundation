# T81 Engineering & Research Status Overlay — Post-`v2.1.0` Modularization & CI Hardening (2026-11-20)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Engineering & Research Status Overlay — Post-`v2.1.0` Modularization & CI Hardening (2026-11-20)](#t81-engineering-&-research-status-overlay-—-post-`v210`-modularization-&-ci-hardening-2026-11-20)
  - [1. What Landed Since `f25387cb7` (`43730f0dd` → `217cc0698`)](#1-what-landed-since-`f25387cb7`-`43730f0dd`-→-`217cc0698`)
  - [2. Updated Modular File Map](#2-updated-modular-file-map)
    - [CLI (`tools/cli/core/`, linked into `t81` / `t81_tool_cli`)](#cli-`toolsclicore`-linked-into-`t81`--`t81_tool_cli`)
    - [Virtual Machine (`vm/` and `vm/internal/`, built into `t81_vm`)](#virtual-machine-`vm`-and-`vminternal`-built-into-`t81_vm`)
  - [3. Preserved Research Holds & Next Actions](#3-preserved-research-holds-&-next-actions)

<!-- T81-TOC:END -->


**Status:** Non-normative dated operational snapshot (supersedes `2026-11-19` `v2.1.0` overlay for `main` layout; preserves all historical research holds and negative findings).  
**Pinned Tag:** `v2.1.0` (`9ddeeccda`) · **Pinned `main`:** `217cc0698` (plus this handoff commit)

## 1. What Landed Since `f25387cb7` (`43730f0dd` → `217cc0698`)

- **Python SDK, RFC-00D1 Negative Tests, & Curated Docs (`43730f0dd`):**
  - Exposed `v2.1.0` Dual-Rail / Axion / SHA-3 primitives in `lang/python/t81_python.cpp` and `ffi/python/t81/__init__.py`.
  - Added 3 RFC-00D1 export negative contract tests (`7/7` passing in `canonfs_cli_contract_tests`).
  - Curated `examples/` and `fs/README.md` entry points.
- **Cross-Platform CI Hardening (`820fc3919`, `ee10baf86`):**
  - Fixed Windows `clang-format` header ordering (`windows.h` before `shellapi.h`) and MSVC `/WX` `C4702` unreachable-code warnings (`cmake/Compiler.cmake`).
  - Updated Intel oneAPI APT GPG key URL in `.github/workflows/ci.yml` to `https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB`.
  - Verified green across `windows-x86_64` (MSVC + clang-cl), `linux-x86_64` (clang + gcc), `linux-arm64`, `macos-arm64`, `macos-x86_64`, `Format Check`, `runtime-contract`, `CodeQL`, `warn-strict`, `clang-tidy`, and `Cross-Platform Determinism Verification`.
- **Monolith Split (`ee1c6dae6`, `aa3ed2f82`, `bb66666fe`, `2f0d8f79c`, `90977dae7`, `91633f71b`, `217cc0698`):**
  - Zero behavioral changes, zero edits to locked `include/t81/**/*.hpp` headers, **-11,657 lines** removed from the two core monoliths:
    - `tools/cli/core/main.cpp`: **11,135 → 4,107 lines** (`-7,028`, `-63.1%`)
    - `vm/vm.cpp`: **7,762 → 3,133 lines** (`-4,629`, `-59.6%`)

## 2. Updated Modular File Map

### CLI (`tools/cli/core/`, linked into `t81` / `t81_tool_cli`)
- `tools/cli/core/main.cpp` (4,107 lines) — entrypoint, arg parsing, shared utilities (`json_escape`, `discover_*`, `sha3_512_*`), remaining misc commands (`doctor`, `test`, `fmt`, `artifact`, `benchmark`, `llama`).
- `tools/cli/core/cli_args.hpp` — shared `struct Args` definition.
- `tools/cli/core/help_text.{hpp,cpp}` — all `print_help_*` text and `print_help_topic` dispatch.
- `tools/cli/core/cmd_axion_canonfs.{hpp,cpp}` — `t81 axion` and `t81 canonfs` subcommands.
- `tools/cli/core/cmd_determinism.{hpp,cpp}` — `t81 determinism` capture, baseline, and diff commands.
- `tools/cli/core/cmd_policy.{hpp,cpp}` — `t81 policy` compile, run, validate, and list commands.
- `tools/cli/core/cmd_vm_tisc_ir.{hpp,cpp}` — `t81 tensor`, `vm`, `profile`, `tier`, `tisc`, and `ir` commands.
- `tools/cli/core/cmd_weights_model.{hpp,cpp}` — `t81 weights` and `t81 model` import, diff, info, verify, quantize, and export commands.
- `tools/cli/core/mcp_server.{hpp,cpp}` — 15-tool native C++ MCP server (`ScopedJsonParser`).

### Virtual Machine (`vm/` and `vm/internal/`, built into `t81_vm`)
- `vm/vm.cpp` (3,133 lines) — `Interpreter` lifecycle, core integer/control/memory/stack/heap opcodes, tier-ceiling checks, and Axion supervision dispatch.
- `vm/internal/vm_support.hpp` + `vm/vm_support.cpp` — tier promotion (`try_promote_tier`), `hash_matches_policy_entry`, `is_exact_ternary_tensor`, `fraction_from_double`, `make_deny_with_reason_engine`, and `t81::axion::DeterminismDetector`.
- Zero-overhead opcode includes inside `Interpreter::step()`'s `switch (insn.opcode)`:
  - `vm/internal/vm_llm_math_opcodes.inc` — `TLoadHash`..`TRoPE`, metadata reflection, LLM math ops.
  - `vm/internal/vm_string_vec_opcodes.inc` — `StrLen`..`StrJoin` string & vector intrinsics.
  - `vm/internal/vm_numeric_opcodes.inc` — `I2F`..`FracDiv` float, transcendental, and exact fraction ops.
  - `vm/internal/vm_tensor_adt_opcodes.inc` — `ChkShape`..`Assert` vector, runtime, ADT (`Option`/`Result`/`Enum`/`Complex`), SWAR, and core tensor ops.
  - `vm/internal/vm_cognitive_opcodes.inc` — `SymLoad`..`DistSeal` Cognitive Tier 1–5 opcodes.
  - `vm/internal/vm_ai_opcodes.inc` — `QMATMUL`..`QUANT_MIXED` RFC-0026 / RFC-0034 AI & quantization opcodes.

## 3. Preserved Research Holds & Next Actions
- All prior negative findings and holds remain intact (`F01` `M1–M6 STOP`, `SCG-08 Gate 2` `NO-GO`, `H01/10A` `BLOCKED`, `AXION-AI-001` `HD01–HD06` open, `INT4×Ternary` 4-plane compute crossover, KV-cache tie collisions, dense VSA, M2/M5 Harley-Seal CSA-3).
- **Next Actions:**
  1. Human release ops: PyPI Trusted Publisher (OIDC) + GitHub Release notes for `v2.1.0`.
  2. Real HuggingFace BitNet b1.58 checkpoint validation via `load_safetensors_bitnet.py --emit-sbdr2d`.
  3. Issue #555 M5 `bench_gemm_dual_rail.cpp` cross-check when shared.
