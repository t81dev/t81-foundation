# T81 Foundation v2.0.0 — Audit Findings & Horizon Suggestions (2026-11-18)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation v2.0.0 — Audit Findings & Horizon Suggestions (2026-11-18)](#t81-foundation-v200-—-audit-findings-&-horizon-suggestions-2026-11-18)
  - [1. Observed Baseline (`Tier A` / `Tier B`)](#1-observed-baseline-`tier-a`--`tier-b`)
  - [2. Release-Readiness Gaps & Actionable Patches (`Tier A` / `Tier B`)](#2-release-readiness-gaps-&-actionable-patches-`tier-a`--`tier-b`)
    - [Concrete Patches for Immediate Blockers & High Gap](#concrete-patches-for-immediate-blockers-&-high-gap)
    - [Live CLI & Governance Receipts (`Tier A`)](#live-cli-&-governance-receipts-`tier-a`)
  - [3. Horizon Capabilities & Immediate Engineering Next Steps (`Tier E` Proposals)](#3-horizon-capabilities-&-immediate-engineering-next-steps-`tier-e`-proposals)

<!-- T81-TOC:END -->


**Observed `main` head:** `b7b1345ed162129133a7948ead7ed52e8fa5fc60` (evaluated with local edits in `docs/releases/v2.0.0.md`, `docs/research/SYSTEMS_AND_SUBSYSTEMS_REPORT.md`, `docs/research/T81_CAPABILITIES_REPORT.md`)  
**Host / Toolchain:** macOS `Darwin 25.5.0` ARM64 (`arm64-apple-darwin25.5.0`), AppleClang `21.0.0`, CMake `4.3.0`, Ninja `1.13.2`, Python `3.14.3`  
**Classification:** Non-normative engineering audit and horizon-planning index. Grants no scientific gate advancement, RFC ratification, release publication, or merge authority. Preserves all active holds (`RFC-00D7-F01` `M1–M6 STOP`, `SCG-08` Gate 2 `NO-GO`, `H01`/10A `BLOCKED`, `AXION-AI-001` `HD01–HD06` open, canonical `H01–H08`).

---

## 1. Observed Baseline (`Tier A` / `Tier B`)

- **Clean Release CI Build (`Tier A`):** `cmake -B build-audit -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_BENCHMARKS=OFF -DT81_ENABLE_TERNARYOS=ON -DT81_ENABLE_DPE=ON` configured in `9.4s` and built all `1,154` targets with `0` compiler warnings in core/VM/Axion/CanonFS/T81Lang/TernaryOS (`8` `-Wc++17-extensions` warnings in `experiments/radix_comparison/` and `experiments/packed_ternary_matvec/`).
- **CTest & Repro Gates (`Tier A`):**
  - Parallel CTest (`-j8`): `428/429` passed (`275.56s` real); `1` failure: `t81_cli_stress_test_runner` (`tests/cpp/cli_stress_test.cpp:225`).
  - Single-threaded determinism slice (`scripts/ci/run_determinism_slice.sh build-audit`): `128/128` passed (`317.68s`).
  - Single-threaded `spec_conformance`: `33/33` passed (`2.13s`).
  - Single-threaded CanonFS/Axion slice: `45/45` passed (`2.86s`).
  - `t81lang_repro_gate.py` (`21` fixtures, hash `ea52b9edf85a7392f2fb49814253d4f2972a79ee205a8be8246f1c40356dff71`) and `t3k_repro_gate.py` (`sha256=c8ee8a402c8d0cc10a2a9f559a729856266c443ce9e93f40a512a93243889e0e`) passed.
  - `tests/python/test_simd_regression.py` (`5/5`), `scripts/ci/validate_scg08_gate1_evidence.py` (`94` negative + `57` JSON-file cases), `scripts/ci/check_tisc_freeze_integrity.py` (`209` opcodes), `scripts/ci/check_ai_experiment_boundary.py`, `scripts/governance/check_determinism_claims.py`, `scripts/ci/audit_workflow_actions.py` (`112/112` SHA-pinned), and `scripts/ci/audit_workflow_permissions.py` (`22/22` explicit) passed.
- **Not Run on This Host:** Linux x86_64/ARM64 native runs, Windows MSVC/`clang-cl`, `asan` preset, Docker (`DOCKER_UNAVAILABLE`), and `cibuildwheel` (`_t81` Python extension).

---

## 2. Release-Readiness Gaps & Actionable Patches (`Tier A` / `Tier B`)

| Priority | Surface | Observed Defect / Discrepancy | Suggested Fix |
| :--- | :--- | :--- | :--- |
| **Blocker** | `tests/cpp/cli_stress_test.cpp:225` | Asserts `"1."` or lowercase `"t81"` in `t81 version` output, which fails against `T81 Foundation 2.0.0`. | Update check to match `"T81 Foundation"` or `"t81"`. |
| **Blocker** | `scripts/ci/check_architecture_targets.py:42` & `docs/explanation/ARCHITECTURE.md:38` | Script only scans root `CMakeLists.txt` and `benchmarks/CMakeLists.txt`, missing targets moved into `src/*/CMakeLists.txt`; `ARCHITECTURE.md` still lists `t81_python` while `CMakeLists.txt:1877` declares `_t81`. | Include `*sorted((root / "src").rglob("CMakeLists.txt"))` in `check_architecture_targets.py` and rename `t81_python` → `_t81` in `ARCHITECTURE.md`. |
| **Blocker** | `scripts/ci/check_architecture_coherence.py:20-23` | Hardcodes `core/vm/vm.cpp`, `core/vm/tensor_helpers.cpp`, `core/vm/policy_trace_bridge.cpp` and root `CMakeLists.txt`, crashing with `FileNotFoundError`. | Update paths to `vm/vm.cpp`, `vm/tensor_helpers.cpp`, `vm/policy_trace_bridge.cpp`, and scan `CMakeLists.txt` + `src/**/CMakeLists.txt`. |
| **High** | `tools/cli/core/main.cpp:242` | `print_version()` prints `Licensed under MIT and GPL-3.0`, contradicting `LICENSE`, `pyproject.toml`, and `Dockerfile` (`Apache-2.0`). | Update `print_version()` to `Copyright © 2026 T81 Foundation` / `Licensed under Apache-2.0`. |
| **Medium** | `experiments/radix_comparison/CMakeLists.txt`, `experiments/packed_ternary_matvec/CMakeLists.txt` | Missing `target_compile_features(... PRIVATE cxx_std_17)`, causing 8 `-Wc++17-extensions` warnings in Release builds. | Add `target_compile_features(... PRIVATE cxx_std_17)`. |
| **Medium** | `spec/axion-kernel.md` §1.1 & `kernel/axion/nondeterminism_detector.cpp` | Spec still marks AX-M5 as unimplemented; `DeterminismDetector::hash_event` uses `std::hash` (intra-process only). | Update spec §1.1 status note; replace `std::hash` with canonical little-endian `t81::crypto::sha3_256` framing if cross-process/persisted digests are needed. |
| **Medium** | `.github/workflows/release.yml` & `docs/releases/v2.0.0.md` | `release.yml` emits `sbom.spdx.json` but no `SHA256SUMS`; `v2.0.0.md` omits `T81Lang`/`T3_K` repro hashes required by `docs/product/RELEASE_DISCIPLINE.md` §3. | Add `SHA256SUMS` generation to `release.yml` and record verified `v2.0.0` repro hashes in `v2.0.0.md`. |
| **Medium** | `ffi/python/` & `tools/cli/core/main.cpp` | `ffi/python/__init__.py` is not nested under `ffi/python/t81/` for zero-install local imports; `tools/cli/core/main.cpp` is an 11,359-line monolith. | Nest `ffi/python/t81/` (or build-tree symlink) and split CLI subcommands into per-command TUs post-`v2.0.0`. |

### Concrete Patches for Immediate Blockers & High Gap

```diff
--- a/tests/cpp/cli_stress_test.cpp
+++ b/tests/cpp/cli_stress_test.cpp
@@ -222,7 +222,8 @@ int main(int argc, char** argv) {
     for (const std::string alias : {"version", "--version", "-V"}) {
       const auto r = run_cli(t81_bin, {alias});
       T81_TEST_CHECK(r.exit_code == 0);
-      T81_TEST_CHECK(contains(r.stdout_text, "1.") || contains(r.stdout_text, "t81"));
+      T81_TEST_CHECK(contains(r.stdout_text, "T81 Foundation") ||
+                     contains(r.stdout_text, "t81"));
     }
   }
```

```diff
--- a/scripts/ci/check_architecture_targets.py
+++ b/scripts/ci/check_architecture_targets.py
@@ -39,7 +39,11 @@ def cmake_targets(cmake_files: list[Path]) -> set[str]:
 def main() -> int:
     root = Path(__file__).resolve().parents[2]
     architecture_md = root / "docs/explanation/ARCHITECTURE.md"
-    cmake_files = [root / "CMakeLists.txt", root / "benchmarks/CMakeLists.txt"]
+    cmake_files = [
+        root / "CMakeLists.txt",
+        root / "benchmarks/CMakeLists.txt",
+        *sorted((root / "src").rglob("CMakeLists.txt")),
+    ]
```

```diff
--- a/scripts/ci/check_architecture_coherence.py
+++ b/scripts/ci/check_architecture_coherence.py
@@ -17,12 +17,15 @@ def main() -> int:
     errors: list[str] = []
 
     opcodes_h = (repo / "include/t81/isa/opcodes.hpp").read_text(encoding="utf-8")
-    vm_cpp = (repo / "core/vm/vm.cpp").read_text(encoding="utf-8")
-    tensor_helpers = (repo / "core/vm/tensor_helpers.cpp").read_text(encoding="utf-8")
+    vm_cpp = (repo / "vm/vm.cpp").read_text(encoding="utf-8")
+    tensor_helpers = (repo / "vm/tensor_helpers.cpp").read_text(encoding="utf-8")
     axion_reasons = (repo / "include/t81/axion/reasons.hpp").read_text(encoding="utf-8")
-    policy_trace_bridge = (repo / "core/vm/policy_trace_bridge.cpp").read_text(encoding="utf-8")
+    policy_trace_bridge = (repo / "vm/policy_trace_bridge.cpp").read_text(encoding="utf-8")
     tensor_native = (repo / "include/t81/tensor/native.hpp").read_text(encoding="utf-8")
-    cmake = (repo / "CMakeLists.txt").read_text(encoding="utf-8")
+    cmake = "\n".join(
+        p.read_text(encoding="utf-8")
+        for p in [repo / "CMakeLists.txt", *sorted((repo / "src").rglob("CMakeLists.txt"))]
+    )
     ci = (repo / ".github/workflows/ci.yml").read_text(encoding="utf-8")
```

```diff
--- a/tools/cli/core/main.cpp
+++ b/tools/cli/core/main.cpp
@@ -239,7 +239,7 @@ void print_version() {
   std::cout << T81_FULL_VERSION << R"(
 Ternary-Native Computing Stack
-Copyright © 2025 T81 Foundation
-Licensed under MIT and GPL-3.0
+Copyright © 2026 T81 Foundation
+Licensed under Apache-2.0
 )";
 }
```

### Live CLI & Governance Receipts (`Tier A`)

- `t81 code check/run examples/core-language/hello_world.t81`: exit `0`, `10 axion event(s)`.
- `t81 determinism verify-run` & `t81 determinism multi-run --count 3`: `"ok": true`, identical SHA3-512 output & trace hashes.
- `t81 axion simulate`: `Trace entries=11 axion_events=10`.
- `t81 canonfs import` + `t81 canonfs import --policy-profile deny-all`: allow path emits `t81.canonfs-import.v1` JSON; `deny-all` fails closed (exit `1`) with `canonfs-policy-denied` before storage writes.
- `t81vm-spec.md` §1.1 `FW-02` (`get_execution_mode()`): confirmed `NOT_FOUND` in `include/`, `vm/`, `src/`.

---

## 3. Horizon Capabilities & Immediate Engineering Next Steps (`Tier E` Proposals)

Grounded in `v2.0.0` live-probe observations (including the observation that `canonfs fsck` currently checks object readability and snapshot manifests rather than re-hashing every `.blk` file on disk):

1. **Self-Verifying CanonFS (`canonfs fsck --verify-hashes`)** — *High leverage / Very low cost*
   - Add an opt-in `--verify-hashes` mode to `canonfs fsck` that re-hashes every `h1-<hex>.blk` payload with `t81::crypto::sha3_256_hex` and fails closed on mismatch, closing silent bit-rot/tamper blind spots without breaking existing `fsck` defaults.
2. **Portable `Axion::DeterminismDetector` Digests** — *High leverage / Low cost*
   - Replace `std::hash` in `kernel/axion/nondeterminism_detector.cpp` with canonical little-endian serialization + `t81::crypto::sha3_256` so detector digests can be stored in CanonFS and compared across hosts.
3. **Expose `T81VM::get_execution_mode()` (`FW-02`)** — *Medium leverage / Very low cost*
   - Close `spec/t81vm-spec.md` §1.1 (`FW-02`) by exposing `ExecutionMode { Interpreter, TraceJit }` and `get_execution_mode() const noexcept` on the VM without altering existing ABI or DCP semantics.
4. **Glass-Box Agent Loop (`t81 ai inference run --checkpoint-canonfs`)** — *High leverage / Modest cost*
   - Chain each bounded `t81_reference_vm` step's `forward_state`, `hidden_tensor_signature_sha256`, and Axion verdict into CanonFS objects for cross-host replayable AI audit trails (respecting the `bounded_horizon_steps: 4` posture unless explicitly extended).
5. **Zero-Drift Exact Scientific Micro-Lab (`examples/research/exact-rational-lab/`)** — *High leverage / Low cost*
   - Pair `T81Lang` / Python (`t81.Fraction`, `t81.BigInt`, `T81Float`) with `t81 determinism certify` + `CanonFS` bundle export as a cross-architecture reproducible numerical benchmark/demo (without claiming ternary speed superiority).
6. **Bit-Identical Replay Consensus & Policy-Mediated Living CanonFS Loops** — *Exploratory*
   - Exchange `(program_canon_ref, input_canon_ref, policy_canon_ref)` across nodes and compare `t81 determinism trace-hash` without requiring network determinism (`RFC-00D0` boundary); orchestrate bounded evolutionary parameter/policy selection *between* VM runs using `t81 tier gate`, `t81 axion simulate`, `canonfs snapshot-diff`, and `canonfs rollback`.
