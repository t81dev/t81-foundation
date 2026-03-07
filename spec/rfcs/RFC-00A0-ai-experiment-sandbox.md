# RFC-00A0: AI Experiment Sandbox and Repository Boundaries

Version 0.1 — Standards Track\
Status: Draft\
Author: T81 Foundation Architecture Team\
Applies to: Repository Structure, Build System, Experimental Workflows

______________________________________________________________________

## Summary

This RFC establishes formal boundaries for AI experimentation within the T81 Foundation repository, defining sandbox directories, access controls, and promotion gates to protect the deterministic core while enabling safe AI research and development.

______________________________________________________________________

## Motivation

The T81 Foundation's defining property is bit-exact deterministic computing. Recent AI integration attempts caused widespread instability by modifying core components without proper boundaries. This RFC creates a systematic approach to AI experimentation that preserves deterministic guarantees while allowing innovation.

______________________________________________________________________

## Proposal

### Technical Details

#### 1. Directory Structure and Boundaries

```
/experiments/ai/           # Primary AI experimentation sandbox
├── quantization/          # Ternary quantization research
├── inference/             # LLM backend adapters
├── opcodes/               # AI-native VM opcode experiments
├── tooling/              # AI-specific developer tools
└── benchmarks/           # AI performance experiments

/research/ai/              # Long-term research projects
├── papers/               # Research papers and analysis
├── prototypes/           # Early-stage prototypes
└── collaborations/       # External research partnerships

/extensions/ai/           # Promoted experimental features
├── quantization/         # Stable quantization codecs
├── inference/            # Production-ready inference adapters
└── tooling/              # Verified AI developer tools
```

#### 2. Core Protection Rules

**Protected Core Directories:**
- `/src` - Core implementation
- `/include/t81` - Public APIs
- `/spec` - Formal specifications
- `/tests` - Core test suite

**Access Rules:**
- Experimental code MAY read from core directories
- Experimental code MUST NOT modify core directories
- Experimental code MAY link against core libraries
- All experimental builds must be opt-in via CMake flags

#### 3. CMake Integration

```cmake
# Experimental AI features (disabled by default)
option(T81_ENABLE_AI_EXPERIMENTS "Enable AI experimental features" OFF)

if(T81_ENABLE_AI_EXPERIMENTS)
    add_subdirectory(experiments/ai)
    add_subdirectory(extensions/ai)
endif()
```

#### 4. Promotion Gates

**Stage 1: Experiment** (`/experiments/ai/`)
- Proof of concept implementation
- Basic functionality demonstration
- No core modifications required

**Stage 2: Extension** (`/extensions/ai/`)
- Stable API surface
- Comprehensive test coverage
- Determinism validation passed
- Documentation complete

**Stage 3: Core Integration** (promoted to `/src`, `/include/t81`)
- Proven architectural value
- No determinism regression
- Performance benefits demonstrated
- Full RFC approval process

### Corner Cases

#### Core Dependency Conflicts
- Experiments must declare minimum core version compatibility
- Version conflicts must be resolved at build time
- Fallback to core-only behavior when experiments disabled

#### Cross-Experiment Dependencies
- Experiments may depend on other experiments
- Circular dependencies are forbidden
- Dependency graph must be acyclic

## Impact

### Backward Compatibility

No impact on existing T81Lang code or TISC binaries. All experimental features are opt-in.

### Performance

No impact on core performance when experiments are disabled. Experimental code runs only when explicitly enabled.

### Security

Enhanced security through explicit boundaries. Experimental code cannot compromise deterministic core without passing promotion gates.

______________________________________________________________________

## Alternatives Considered

1. **Branch-based isolation**: Rejected due to maintenance overhead and difficulty tracking experimental progress
2. **Plugin architecture**: Rejected due to complexity and potential for runtime instability
3. **No formal boundaries**: Rejected due to previous instability incidents

______________________________________________________________________

## References

- [Stable Baseline Policy](../../docs/records/STABLE_BASELINE.md)
- [T81 Determinism Guarantees](RFC-0002-deterministic-execution-contract.md)
- [Axion Safety Model](RFC-0003-axion-safety-model.md)
