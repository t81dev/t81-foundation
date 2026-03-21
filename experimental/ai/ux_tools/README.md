# T81 AI UX Tools

Minimal AI UX CLI surface used by AI CI contract checks.

## Binary

- `t81_ai` (built from `t81_ai_minimal.cpp` when this target is enabled in the active CMake graph)

## Supported Commands

```bash
# help
t81_ai --help

# model inspection + verification
t81_ai model inspect model.gguf
t81_ai verify model.gguf
t81_ai verify determinism model.gguf

# workflow replay artifacts (RFC-00A7 baseline runtime binding)
t81_ai workflow run smoke --seed 0 --out replay.json
t81_ai workflow replay replay.json
t81_ai workflow report replay.json

# observability trace artifact (RFC-00A7 baseline runtime binding)
t81_ai observability trace trace.json
```

## Notes

- This is intentionally a minimal implementation for deterministic CI contract gating.
- `workflow run` emits schema `t81.ai.workflow-replay.v1`.
- `observability trace` emits schema `t81.ai.trace.v1` with required reason fields and a live WLOAD allow-path runtime binding used by the AI policy/WLOAD CI gates.
- `backend capabilities` / `backend select` expose the governed `t81_reference_vm` strict-deterministic reference backend for the `gguf`/`t3k` lanes used by RFC-0026 readiness gating.
