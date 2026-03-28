# Examples

Small, self-contained samples for the C++ API.

## Build & Run (CMake)

```bash
cmake -S . -B build -DT81_BUILD_EXAMPLES=ON
cmake --build build
./build/t81_demo
./build/t81_tensor_ops
./build/t81_ir_roundtrip
```

## Files

- `demo.cpp` — basic BigInt + Tensor dot example.
- `tensor_ops.cpp` — transpose, slice, reshape on a small matrix.
- `ir_roundtrip.cpp` — encode/decode a tiny IR program and print it.
- `canonfs-interchange/` — golden RFC-00D1 import/export example with a small
  host directory, expected JSON result shapes, and one policy-profile denial
  path.
- `model-load-canonfs/` — tiny CanonFS-backed `--weights-model` example using a
  synthetic `.t81w`, a generated SafeTensors source plus `t81 weights import`,
  a tiny native `T3_K` GGUF quantize/import lane, a real downloaded tiny
  Hugging Face model, a one-command rerun script, `canonfs put-file`, and
  allow/deny policy checks.
- `tisc/` — sample precompiled `.tisc` binaries used for runtime/disasm/debug examples.
- `system-integration/` — Comprehensive examples demonstrating the coalescence of T81Lang, TISC, HanoiVM, Axion, and CanonFS.
  - `distributed_inference.t81` / `distributed_policy.apl` — Sharded tensor loading, distributed matmul, and self-tuning via reflection.
  - `evolutionary_db.t81` / `db_safety.apl` — Database logic with `T81Tree` and reflective optimization of indexing.
  - `autonomous_agent.t81` / `agent_ethics.apl` — Agent loop with `T81Agent`, neural decision making, and ethical reflection.
  - `accumulator.t81` / `strict_resource.apl` — Auditable summation loop.
  - `inference.t81` / `secure_model.apl` — Policy-gated model inference.
  - `cognition.t81` / `learning_safety.apl` — Self-refining cognition loop.
