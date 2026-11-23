# Evidence-Linked Architecture Assessment (`t81-foundation`)

**Pinned inspection revision:** `origin/main` at `c2ab68e582c9f773ce94a9e13bcb805016f07dd4`.
**Classification legend:** `[specified]` (normative RFC/spec text), `[implemented]` (present in source/build), `[tested]` (directly executed with cited run/receipt), `[measured]` (backed by recorded benchmark/telemetry), `[proposed]` (planned investigation or unimplemented design).

---

## 1. Structural Strengths & Bounded Invariants

* **Spec-First Layering:**
  * `[specified]` Core execution, fault, policy, and data contracts are defined in `spec/rfcs/RFC-0002-deterministic-execution-contract.md`, `spec/rfcs/RFC-0009-axion-policy-language.md`, and `docs/architecture/README.md`.
  * `[implemented]` Canonical types (`include/t81/types/T81Float.hpp`, `Base243` in `include/t81/codec/base243.hpp:14`), TISC ISA opcodes (`include/t81/isa/opcodes.hpp`), `T81VM` (`vm/vm.cpp`), and CanonFS drivers (`fs/block_backed_driver.cpp`) implement the hosted baseline.

* **Deterministic Trap Model (Actual Boundaries vs. Universal "Zero UB"):**
  * `[specified]` `[implemented]` Defined VM/ISA error conditions map to `Trap::DecodeFault`, `Trap::BoundsFault`, and `Trap::SecurityFault` (`include/t81/vm/traps.hpp:8-13`), with checked-in conformance fixtures (`spec/conformance/tisc/bounds-fault-contract.t81`, `spec/conformance/ai/embed-bounds-check.t81`, `spec/conformance/ai/attn-determinism.t81`; `[implemented]` test artifacts, not individually executed for this assessment).
  * **Actual boundary:** This does **not** eliminate undefined behavior or host drift across the entire C++ runtime. The hosted runtime relies on the host OS for memory allocation and file I/O (`docs/architecture/T81_ARCHITECTURE_REPORT.md:156`), and `include/t81/types/T81Float.hpp:20-21, 333-334` documents remaining host-math fallbacks where `dmath` transcendentals are incomplete.

* **Axion Pre-Side-Effect Interception (Actual Boundaries vs. Universal Security):**
  * `[specified]` `[implemented]` `[tested]` Synchronous pre-side-effect mediation is implemented and verified at specific boundaries:
    * **VM step & policy opcodes:** `eval_axion_call` (`vm/vm.cpp:616, 7354-7365`), `handle_axread` (`Opcode::AxRead = 22`, `vm/vm.cpp:7420-7443`), `handle_axset` (`Opcode::AxSet = 23`, `vm/vm.cpp:7445-7472`), and `authorize_weights_tensor` (`vm/vm.cpp:732-770, 6415, 6575`).
    * **Block-backed CanonFS metadata:** `BlockBackedDriver::publish_capability`, `revoke_capability`, and `parity_repair_subtree` consult `axion_allow` before store/index/device mutation (`fs/block_backed_driver.cpp:142-185`; `[tested]` via `canonfs_policy_boundary_test` in `#792` / `origin/main` and PR `#796` hosted CI run `43717280854`).
    * **CLI explicit-policy preflight:** `tools/cli/core/execution_policy.hpp` and `tools/cli/core/driver.cpp` validate explicit policies before `vm->load_program` (`[tested]` locally on Release macOS ARM64 in `t81_cli_execution_policy_test` on `#793` and `#794`, and in PR `#796` hosted CI).
  * **Actual boundary:** `Opcode::AgentInvoke = 246` (`include/t81/isa/opcodes.hpp:272`, `vm/vm.cpp:4010-4045`) emits an `Allow` audit event and checks tier/recursion ceilings (`Trap::TierFault`), relying on the per-step `eval_axion_call(kStep, ...)` gate (`vm/vm.cpp:616`) rather than an extra `eval_axion_call` inside `AgentInvoke`. Axion is an in-process application gate, **not** an OS or hardware sandbox.

---

## 2. Architectural Stress Points, Call Paths & Technical Corrections

* **Binary-Host Execution vs. Dual-Rail Representations vs. Differential Signaling:**
  * `[implemented]` `[measured]` The `t81-foundation` runtime executes ternary operations on binary hosts using `Base243` (`include/t81/codec/base243.hpp:14`) and 2-bit packed SWAR/SIMD kernels (`include/t81/experimental/packed_trit_vector.hpp`, `benchmarks/BM_PackedTritVector.cpp:724-802`; `BM_Kernel_TAnd_{SWAR,AVX2}` `[measured]` in PR `#796` job `132822088107`).
  * **Dual-rail representations (`[proposed]` / research-only):** Bit-sliced dual-bitplane representations (`pos_mask`/`neg_mask` or `is_neg`/`is_zero`/`is_pos` bitplanes in `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md:23, 77, 140` and Verilog dual-bitplane modules in `docs/architecture/visualization/architecture_nodes.json:949`) are **not** integrated into `T81VM`.
  * **Differential electrical signaling (`[proposed]` / modeled hardware context):** CMOS dual-rail differential signaling / 2-bit standard-cell synthesis (`docs/research/RESEARCH_GAPS.md:99`, modeling a `1.4x–1.8x` area and `1.3x–1.5x` power penalty vs. binary gates) is a physical-circuit research constraint, distinct from software bitplane layouts.

* **Axion Call Paths on VM & Tensor Dispatch (`TWMATMUL = 0xC2`, `TATTN = 0xC4`):**
  * `[implemented]` Tensor policy checks occur around instruction dispatch and weight-handle access, **not** around every inner-loop multiply-accumulate:

| Boundary | Pinned Path (`vm/vm.cpp`) | What to record |
|---|---|---|
| Per VM step | `eval_axion_call(kStep, current_pc, insn.opcode)` (`lines 616-625`, `7354-7365`) | Policy evaluation frequency (once per dispatched instruction), `make_syscall_context(...)` construction cost, `Deny` (`SecurityFault`) / `Warn` branching. |
| Tensor access | `authorize_weights_tensor(reg, handle)` (`lines 732-770`; called in `TWMATMUL` `line 6415` and `TATTN` `line 6575`) | Handle lookup (`DecodeFault` if missing), `allowed_ternary_model_hashes` / `ternary_weight_domain_check` evaluation, and failure ordering before `native_tensor_twmatmul_direct` / `native_tensor_tattn_direct`. |
| Audit event | `record_axion_event(...)` (`lines 619, 624, 759, 4023, 6429, 6589`) | Recording point (pre-dispatch on step `Deny`/`Warn`, on weight-check `Deny`, and post-direct-kernel `Allow` before `alloc_tensor`), plus observable trace/audit-log effects. |

* **OS vs. Runtime & Authoritative Surface Maturity:**
  * `[implemented]` `[tested]` TernaryOS bare-metal QEMU boot lanes exist (`.github/workflows/qemu-boot.yml`, `.github/workflows/qemu-boot-x86.yml`; `Build BOOTX64.EFI and validate OVMF boot` passed on PR `#796`), while `docs/architecture/T81_ARCHITECTURE_REPORT.md:148-152, 170-175` treats TernaryOS as peripheral to the runtime-first hosted lane.
  * **Authoritative DCP maturity:** Following `AGENTS.md` and `spec/rfcs/` status rather than loose summary prose, `RFC-00D1` (CanonFS foreign interchange) is **draft / partially implemented**, and `RFC-00D0` (Base-81 TCP/IP) is **draft / design-led**. Neither draft surface is promoted to "Frozen/Stable" DCP status.

---

## 3. Recommendations & Immediate Scope

1. **Align Hardware Interop with `RFC-0055` & `RFC-0051`:**
   * `[specified]` `spec/rfcs/RFC-0055-native-ternary-hardware-target-and-interop-contract.md` (Status: `accepted`) governs general-purpose native ternary targets (`DMA and device-visible memory rules` at `line 176`; trace/Axion/CanonFS preservation at `lines 197-205`), while `spec/rfcs/RFC-0051-deterministic-heterogeneous-acceleration.md` (Status: `accepted`) governs narrow accelerators.
   * **Correction:** Neither `RFC-0055` nor `include/`/`vm/`/`src/` defines `t81.dev` co-processors or MMIO/DMA ring-buffer drivers; any concrete hardware-target implementation remains `[proposed]`.

2. **Preserve Policy-Overhead Profiling as a Proposed Investigation:**
   * `[proposed]` Use the call-path table above as the evidence base for a later Axion-overhead profiling experiment (measuring `eval_axion_call`, `authorize_weights_tensor`, and `record_axion_event` on `TWMATMUL`/`TATTN` dispatch), pending call-path inspection.
   * **Constraints:** Do **not** recommend or implement policy caching or ahead-of-time authorization until capability revocation (`revoke_capability`), dynamic policy changes, and pre-side-effect ordering are addressed (note `spec/rfcs/RFC-0028-deterministic-trace-jit.md:76` also rejects AOT compilation due to dynamic tiering and dynamic Axion policy evaluation). **No new benchmark campaign or optimization is authorized yet.**

3. **Maintain Surface Separation:**
   * `[specified]` `[implemented]` Keep Frozen/Accepted DCP surfaces (`TISC` ISA, `T81VM`, `Axion` policy engine) isolated from Draft/Experimental surfaces (`RFC-00D1`, `RFC-00D0`, TernaryOS boot paths, Tier 4+ cognitive extensions, and unimplemented hardware targets).
