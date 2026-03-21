# T81 AI Experiment Sandbox - RFC-00A0

This directory contains the core infrastructure for AI experimentation in T81.

## Components

### Experiment Manager (`t81_ai_sandbox`)
CLI tool for managing AI experiments with core protection enforcement.

**Usage:**
```bash
# List available experiments
./t81_ai_sandbox list

# Enable specific experiment
./t81_ai_sandbox enable determinism

# Validate experiment isolation
./t81_ai_sandbox validate

# Generate promotion report
./t81_ai_sandbox promotion-report
```

### Promotion Gates (`t81_ai_promotion`)
CLI tool for validating experiments before promotion.

**Usage:**
```bash
# Validate experiment for promotion to extension
./t81_ai_promotion determinism experimental

# Validate extension for promotion to core
./t81_ai_promotion quantization extension
```

## Core Protection Features

- **Directory Validation**: Prevents modification of core directories
- **Isolation Verification**: Ensures experiments don't depend on core modifications
- **Promotion Tracking**: Generates reports for experiment promotion
- **Safety Checks**: Validates experiment boundaries before enabling
- **Gate Validation**: Automated validation of promotion criteria

## Acceptance Criteria

- [x] Directory structure implemented with proper CMake integration
- [x] Core protection rules enforced via build system
- [x] Experimental builds compile and run independently
- [x] No core modifications required for any experiment
- [x] Promotion gate documentation and tooling implemented

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_sandbox t81_ai_promotion

# Run sandbox manager
./build/experiments/ai/bin/t81_ai_sandbox

# Run promotion validator
./build/experiments/ai/bin/t81_ai_promotion
```

## Integration

This sandbox provides the foundation for all other AI experiments:
- **Determinism Framework** (RFC-00A1)
- **Model Provenance** (RFC-00A3)
- **Benchmark Suite** (RFC-00A2)
- **Quantization Codecs** (RFC-00A4)
- **Backend Adapters** (RFC-00A5)
- **Policy Hooks** (RFC-00A6)
- **UX Tools** (RFC-00A7)
- **VM Opcodes** (RFC-00A8)

---

**RFC Reference**: RFC-00A0  
**Tasks**: 1-2 - Sandbox infrastructure and promotion gates  
**Status**: Completed  
**Last Updated**: 2026-03-05
