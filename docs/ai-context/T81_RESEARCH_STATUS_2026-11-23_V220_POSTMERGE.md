# T81 Engineering & Research Status Overlay — `v2.2.0` Release & Post-Merge `main` Reconciliation (2026-11-23)

**Status:** Non-normative dated operational snapshot (supersedes `2026-11-20` overlay for release and `main` layout; preserves all historical research holds, negative findings, and `CURRENT_STATE.json` contract-test compatibility).  
**Pinned Release Tag:** `v2.2.0` (`7aa734ea4`, published `2026-11-22T13:31:48Z`) · **Pinned `origin/main`:** `b2248e457`

## 1. Shipped `v2.2.0` Surface & Packaging Verification

- **PyPI (`t81==2.2.0`, queried `2026-11-23` via `https://pypi.org/pypi/t81/json`):**
  - Latest published version is `2.2.0` with **31 distribution files** (30 prebuilt wheels + 1 sdist).
  - Standard **CPython 3.9–3.14** coverage is complete across all 5 target platforms, including all 5 `cp314` wheels (`macosx_10_15_x86_64`, `macosx_11_0_arm64`, `manylinux_2_27_aarch64`/`manylinux_2_28_aarch64`, `manylinux_2_27_x86_64`/`manylinux_2_28_x86_64`, `win_amd64`). No `v2.1.1` patch cut is needed.
  - `pyproject.toml` (`version = "2.2.0"`) includes Python `3.14` in classifiers and `cp39-*`..`cp314-*` in `[tool.cibuildwheel]`.
  - SDK additions in `v2.2.0` (`docs/releases/v2.2.0.md`): `CanonRef` digest reconstruction + `CanonRef.to_cli_handle()`, `StepLimitExceeded` pause/resume + `vm.halted` + `compile_and_run(..., max_steps=...)`, read-only `vm.axion_events` snapshots alongside `vm.axion_log`, dynamic tensor element-count validation, fixed-size tensor negative indexing, and `int()` conversion for `T81Int`/`BigInt`.
- **GitHub Release (`v2.2.0`, `gh release view v2.2.0`):**
  - **11 assets** published: native archives for Linux `x86_64`/`arm64`, macOS `x86_64`/`arm64`, and Windows `x86_64`, plus `install.sh`, `install.ps1`, `SHA256SUMS`, `sbom.spdx.json`, `canonfs_fixture_trial.py`, and `canonfs_fixture_trial.md`.
  - The native CLI and 15-tool C++ MCP server (`t81 mcp serve`) remain separate native builds outside the PyPI wheel.
- **Runtime & Storage Surfaces Carried Forward (`v2.1.0` → `v2.2.0`):**
  - `T81VM` Dual-Rail `(P, N)` bitplane `TMatMul` (NEON + AVX2) and `SBDR1T2D` CanonFS weight caching (`CanonDualRailRhsCache`).
  - Hardened 15-tool native C++ MCP server (`tools/cli/core/mcp_server.{hpp,cpp}`).
  - Modularized CLI (`tools/cli/core/cmd_*.{hpp,cpp}`) and VM (`vm/vm_support.cpp` + `vm/internal/vm_*_opcodes.inc`).

## 2. Post-`v2.2.0` Merges on `main` (`7aa734ea4` → `2e6e78056`)

- **PR #842 (`78e66dd52`):** `test(canonfs): preserve existing bytes on denied export`.
- **PR #707 (`a958b6e5a`):** `test(isa): restore meaningful opcode matrix conformance under NDEBUG` (CPA-002 evidence retained).
- **PR #752 (`c9bc4d75f`):** `docs(h04-r03): separate current retention status from historical snapshot`.
- **PR #845 (`120b8dc90`):** AArch64 QEMU boot demo setup, foreground terminal handling, and CI GPT tooling verification (`.github/workflows/qemu-verification.yml`).
- **PR #848 (`5a005b2de`):** `fix(canonfs): keep denied directory exports off the host`.
- **PR #849 (`82395ae27`):** `test(canonfs): reject absolute and nested unsafe export paths` + fixture download retry on `curl` recv errors.
- **PR #850 (`2e6e78056`):** `Record 2026-11-23 cloud VM benchmark suite evidence`.
- **PR #847 (`bef0c7e44`):** `fix(mcp): confine serve paths and gate WeightsLoad`.
- **PR #858:** `ci(docs): add bounded lychee retries for transient HTTP 503 responses`.
- **PR #857:** `fix,test(canonfs): flush export writes and cover target failure reasons`.
- **PR #856:** `ci(qemu): verify interactive Axion serial round-trip in AArch64 boot demo`.
- **PR #855 (`b2248e457`):** `docs(ai-context): refresh handoff and AGENTS.md to v2.2.0 state`.

## 4b. Coexistence & TernaryOS v0.2 Reconciliation (`feat/ternaryos-v0.2-nonblocking`)

Reconciled with [T81_RESEARCH_STATUS_2026-11-23_BY_AI_FOR_AI_COEXISTENCE.md](T81_RESEARCH_STATUS_2026-11-23_BY_AI_FOR_AI_COEXISTENCE.md), [TERNARYOS_ARCHITECTURE_v0.2.md](../research/proposals/TERNARYOS_ARCHITECTURE_v0.2.md), [TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md](../research/proposals/TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md), and [TERNARYOS_MULTI_AGENT_CAPABILITY_BUDGET_v0.2.md](../research/proposals/TERNARYOS_MULTI_AGENT_CAPABILITY_BUDGET_v0.2.md):

- **Routing Ledger & `R1`/`R2` Rules:** `10` accepted (`+1`) / `16` negative (`-1`) constraints; `M == 1` with `s >= 90%` → `CPU_1T_SBDR128_SIMD`; `s >= 75%` for `M >= 4` or `75% <= s < 90%` at `M == 1` → `GPU_R1_BitmaskSkip_Pop`; dense/moderate `R1` → `GPU_R1_DualRail_Pop`; `R2` `M = 1..4` → `CPU_4T_INT4W`, `M >= 16` → `CPU_4T_INT8W`; `k_star_prepacked == 0`.
- **Explicit `{-1, 0, +1}` Semantics:** `+1` verified with valid `SHA3-512` receipts, `0` strictly for inert / zero-skip (`dualrail_sparse128_blocks_skipped`) / unmeasured states (including native `x86_64` AVX2 `BM_MemBound64_(DualRail_Dense|SBDR128_SIMD|INT8_Sparse128)` until measured on `ubuntu-24.04`), `-1` for fail-closed denials, conflicts, or tamper breaches.
- **Immutable Policy Root & Multi-Agent Isolation:** `axion_root.apl` mutation rejected fail-closed (`ERR_IMMUTABLE_POLICY_ROOT`); `tx_begin`/`tx_commit`/`tx_abort` enforce `ERR_TX_CONFLICT`, `ERR_PARENT_RECEIPT_MISMATCH`, `ERR_AGENT_MISMATCH`, unified `CanonFS` fsck + `SBDR1T2D` (`K* = 0`) + double-run `T81VM` gating with `0` orphan `.stage`/`.bak` artifacts.
- **Verified Digests:**
  - `boot_receipt_sha3_512`: `sha3-512:737e6dc910757ff96e6a1207bf47ca9edc4f9f0f31ba46464463a003ac9405d7c95f0feb1d0c2147432ebc365a4f2c491c0ceba0d33b23706adad5b080e1aca7`
  - `boot_image_sha3_512`: `sha3-512:26bda4ae26a9fd74c4aff8e3c633c8b54174b9913742fd40d9228fe76a1eebfcc1717ac68824be3a1c9640a2aa2fc6803ed31667614254e439a3f93d83c0c55d`
  - `serial_transcript_sha3_512`: `sha3-512:b052306fc0fe3c1f6175e9d0241d6688b43cf877b9d0e99b25c8b32855e926b9fabccd06263a954c38379ea3c463d9336b1b5ff3eed1bb4d63c92198d0be9248`
  - `ledger_sha3_512` (at `4fc40e24b`): `sha3-512:bf21b0bbadec67cf2ff1dde4b09c192bd4274734e0e969dfe4ae741b7eeeacf747688ec985baf493b05768810cf3d3d26e02d32e2cf3217d1dc7a1beff3ad42b`
  - Black-box 4-tier AI-usability harness (`examples/research/agent-receipt-sidecar/ai_usability_eval.py`, PR `#861`): `19/19` trials passing (`+1: 11`, `0: 3`, `-1: 5`, `0` orphans, `eval_report_sha3_512`: `sha3-512:243081f8967badce79c07d8c1a6150262858622cb488e7503d3acc084e8ea7710556c4b9581f503bac504bece8ea2ba92a1865d0e6cfdd7cf9abb03976b1ce1f`).

## 3. CI & Determinism Gate Check (`2026-11-23`)

- **`82395ae27` (PR #849 merge):**
  - `Cross-Platform Determinism Verification` (`#45183755676`), `runtime-contract` (`#45183755796`), `Format Check` (`#45183756141`), `Documentation` (`#45183756014`), and `CodeQL` (`#45183755751`) all **passed**.
  - `T81 Foundation CI` (`#45183755892`) failed solely in `lint / spec & docs` due to transient HTTP `503 Service Unavailable` responses from `github.com` during `lychee` link checking (not a code or determinism regression).
- **`2e6e78056` (PR #850 merge, current `origin/main`):**
  - `runtime-contract` (`#45183847680`), `Format Check` (`#45183847788`), `CodeQL` (`#45183847764`), and `pages-build-deployment` (`#45183847406`) **passed**.
  - `T81 Foundation CI` (`#45183847780`) and `Cross-Platform Determinism Verification` (`#45183848015`) were queued/in-progress at observation time (`2026-11-23T14:59Z`).
- **`CURRENT_STATE.json` note:** Intentionally retains its contract-tested `selected_view` (`2026-11-17-pr824-reconciled`) for `tests/ci/test_current_state_contract.py` compatibility; use this dated overlay for live `v2.2.0` + `main` orientation.

## 4. TernaryOS Alpha Track — Scoped Next Target

- **Current Verified Baseline:** `.github/workflows/qemu-verification.yml` (PR #845) now boots the freestanding AArch64 image in Ubuntu CI (`clang-18`, `lld-18`, `qemu-system-arm`, `qemu-efi-aarch64`) to the `[axion@T81 tier=N]$ ` prompt and verifies `canonfs: I/O probe OK (LBA0 magic=CST1, LBA1 round-trip pass)`. The cooperative EL0 scheduler (`ternaryos/`, RFC-00B0–RFC-00C6) is graduated with `WaitForDevice`/`ExitThread` and per-thread L3 isolation.
- **Recommended Next Narrow Bring-Up Target (kept separate from `v2.2.x` runtime maintenance):** Add one deterministic, CI-asserted serial check inside `qemu-verification.yml` that exercises a single interactive shell or EL0 scheduler/Axion-policy round-trip after the CanonFS LBA1 probe (rather than broadening hardware driver coverage before boot/serial assertions are locked).

## 5. Preserved Research Holds

All prior negative findings and holds remain intact without modification: `F01` (`M1–M6 STOP`), `SCG-08 Gate 2` (`NO-GO`), `H01`/10A (`PHYSICAL EVALUATION BLOCKED`), `AXION-AI-001` (`HD01–HD06` open), `INT4×Ternary` 4-plane compute crossover, 1-stage ternary KV-cache tie collisions, dense VSA vs fair 1-bit binary, and M2/M5 Harley-Seal CSA-3. No new experiment, merge, RFC acceptance, or governance activation authority is granted by this overlay.
