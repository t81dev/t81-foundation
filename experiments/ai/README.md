# T81 AI Experiments

This directory contains experimental AI integration work for the T81 Foundation.

## Directory Structure

```
/experiments/ai/
├── sandbox/           # RFC-00A0: Experiment infrastructure
├── determinism/        # RFC-00A1: Deterministic evidence protocol
├── benchmarks/         # RFC-00A2: AI benchmark specification
├── model_provenance/  # RFC-00A3: Model artifact provenance
├── quantization/       # RFC-00A4: Ternary quantization codecs
├── llm_backend/        # RFC-00A5: LLM backend adapter
├── policy_hooks/       # RFC-00A6: Axion policy hooks
├── ux_tools/          # RFC-00A7: UX integration
└── vm_opcodes/         # RFC-00A8: AI-native VM opcodes
```

## Core Protection Rules

- **No core modifications**: Experiments must not modify `/src`, `/include/t81`, `/spec`, `/tests`
- **Opt-in only**: Experimental features require explicit enablement
- **Isolated builds**: Each experiment builds independently
- **Determinism required**: All experiments must pass determinism validation

## Build Integration

Experimental AI features are enabled with:
```bash
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make -j$(nproc)
```

## Promotion Path

1. **Experimental** → `/experiments/ai/` - Initial implementation
2. **Extension** → `/extensions/ai/` - Stable API, comprehensive testing
3. **Core** → `/src`, `/include/t81/` - Proven architectural necessity

## Safety Guidelines

- All experiments must maintain T81's deterministic guarantees
- Security policies must be enforced for AI operations
- Performance impact must be measured and documented
- Comprehensive testing required before promotion

---

**RFC References**: RFC-00A0 through RFC-00A8  
**Last Updated**: 2026-03-05
