# AI Promotion Audit Report

## AI Promotion Matrix

| Component | Location | Category | Reason | Target Location |
| --------- | -------- | -------- | ------ | --------------- |
| ternary_codec.cpp | experiments/ai/quantization/ | MODIFY THEN PROMOTE | Implements standardized ternary quantization codecs with deterministic Base-81 packing, but currently uses floating-point math for calculating MSE/PSNR metrics. Needs to replace floating-point operations with fixed-point or remove them from core paths. | core/math/quantization/ |
| backend_adapter.cpp | experiments/ai/llm_backend/ | MODIFY THEN PROMOTE | Uses external ML backends (llama.cpp, onnx_runtime) that introduce non-determinism. Needs a pure T81 VM backend for deterministic execution. | core/vm/ai_backend/ |
| model_manager.cpp | experiments/ai/model_provenance/ | MODIFY THEN PROMOTE | Manages model loading but does not explicitly use Axion's canonical hash verification pathway (`TLOADHASH`). Needs Axion integration. | Axion subsystem / CanonFS |
| axion_hooks.cpp | experiments/ai/policy_hooks/ | PROMOTE | Extends Axion policy system with AI events. Follows deterministic policy evaluation logic. | Axion subsystem |
| benchmark_runner.cpp | experiments/ai/benchmarks/ | KEEP EXPERIMENTAL | Relies on hardware-dependent metrics (latency, memory usage) and OS/hardware information gathering. Non-deterministic by nature. | experiments/ai/benchmarks/ |
| evidence_collector.cpp | experiments/ai/determinism/ | PROMOTE | Collects and verifies deterministic evidence. Uses SHA256 and deterministic metric comparison. | tests/determinism/ |
| t81_ai_cli.cpp | experiments/ai/ux_tools/ | MODIFY THEN PROMOTE | Provides CLI interface but uses mock data and `std::this_thread::sleep_for` for simulation. Needs integration with actual deterministic implementations. | tools/cli/ai/ |
| t81_ai_minimal.cpp | experiments/ai/ux_tools/ | KEEP EXPERIMENTAL | Minimal implementation using mock data and hardcoded responses. Not suitable for core. | experiments/ai/ux_tools/ |
| promotion_gates.cpp | experiments/ai/sandbox/ | KEEP EXPERIMENTAL | Validates experiment promotion using simulated CI and audit checks. Not part of the runtime core. | experiments/ai/sandbox/ |
| IMPLEMENTATION_REPORT.md | experiments/ai/opcodes/ | PROMOTE | Documents AI-native opcodes (ATTN, QMATMUL, EMBED) with proven deterministic phase-1 conformance. | docs/architecture/ |

## Promotion Roadmap

Phase 1 — Deterministic Tensor Primitives & Codecs
- Modify `ternary_codec.cpp` to replace floating point operations with deterministic integer or fixed-point equivalents.
- Promote modified `ternary_codec.cpp` to `core/math/quantization/`.
- Ensure all fixed-point and integer math operations are thoroughly tested in `tests/determinism/`.

Phase 2 — AI Opcode Integration
- Integrate AI opcodes (ATTN, QMATMUL, EMBED) documented in `IMPLEMENTATION_REPORT.md` into the core ISA (`core/isa/`) and VM (`core/vm/`).

Phase 3 — Axion Model Governance & Policy
- Modify `model_manager.cpp` to enforce `TLOADHASH` for model loading.
- Promote `axion_hooks.cpp` and the modified `model_manager.cpp` to the Axion subsystem.

Phase 4 — Deterministic VM Backend
- Refactor `backend_adapter.cpp` to strictly use the T81 deterministic VM backend, removing dependencies on `llama.cpp` and `onnx_runtime` for core promotion.

Phase 5 — CLI Exposure & Testing
- Promote `evidence_collector.cpp` to `tests/determinism/`.
- Refactor `t81_ai_cli.cpp` to interact with the real, promoted subsystems and promote to `tools/cli/ai/`.

## Determinism Risk Report

- **External ML Dependencies:** `backend_adapter.cpp` uses `llama.cpp` and `onnx_runtime`, which can introduce non-deterministic execution and float math.
  - *Remediation:* Create a pure T81 VM backend that strictly adheres to the Deterministic Core Profile. Only use this backend for promoted code.
- **Hardware/Time-Dependent Behavior:** `benchmark_runner.cpp` measures latency, TPOT, and memory usage, relying on `std::chrono` and hardware specifics. `t81_ai_cli.cpp` uses `std::this_thread::sleep_for`.
  - *Remediation:* Keep benchmarking tools in the experimental directory. Remove all sleep calls and mock delays from code intended for core promotion. Use deterministic step counting for progress instead of real-time metrics.
- **Floating Point Math:** `ternary_codec.cpp` uses `float`, `double`, `std::sqrt`, and `std::log10` for metric calculation (MSE, PSNR).
  - *Remediation:* Replace floating-point metrics with fixed-point equivalents or restrict these calculations to off-chain/offline analysis tools. Core tensor math must exclusively use integer or fixed-point representations.
- **Model Loading without Axion:** `model_manager.cpp` implements its own hash verification but needs to formally integrate with the Axion kernel's `TLOADHASH` mechanism.
  - *Remediation:* Update the model loading pathway to invoke Axion policy evaluation and strictly use `TLOADHASH`. Unverified models must trigger a hardware halt or equivalent deterministic error state.