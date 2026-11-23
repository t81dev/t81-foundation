# T81 Engineering & Research Status Overlay — `v2.2.0` Release & Post-Merge `main` Reconciliation (2026-11-23)

**Status:** Non-normative dated operational snapshot (supersedes `2026-11-20` overlay for release and `main` layout; preserves all historical research holds, negative findings, and `CURRENT_STATE.json` contract-test compatibility).  
**Pinned Release Tag:** `v2.2.0` (`7aa734ea4`, published `2026-11-22T13:31:48Z`) · **Pinned `origin/main`:** `2e6e78056`

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
