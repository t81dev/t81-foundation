# T81 Deterministic Evidence Collection Framework - RFC-00A1 Task 3

This directory contains the framework for collecting, verifying, and reporting deterministic evidence for AI workloads in the T81 ecosystem.

## Components

### Evidence Collector (`t81_ai_evidence`)
CLI tool for collecting and validating deterministic evidence across multiple execution modes.

**Usage:**
```bash
# Strict determinism validation
./t81_ai_evidence my_experiment strict ./evidence_output

# Statistical determinism validation
./t81_ai_eference my_experiment statistical ./evidence_output

# Reproducible non-deterministic validation
./t81_ai_evidence my_experiment reproducible_non_deterministic ./evidence_output
```

## Determinism Modes

### Strict Mode
- **Requirement**: Bit-exact reproducibility across all executions
- **Validation**: Input hash, output hash, and execution trace must be identical
- **Use Case**: Core T81 operations, deterministic algorithms

### Statistical Mode
- **Requirement**: Results within statistical tolerance bounds
- **Validation**: Metrics variance < 0.1% across executions
- **Use Case**: Floating-point operations, hardware-specific optimizations

### Reproducible Non-Deterministic Mode
- **Requirement**: Documented randomness with proper seeding
- **Validation**: Random seeds documented and reproducible
- **Use Case**: AI inference with controlled randomness

## Evidence Collection

The framework collects:

### Input/Output Hashes
- SHA-256 hashes of all input data
- SHA-256 hashes of all output results
- Cryptographic verification of data integrity

### Execution Traces
- Complete execution trace with instruction-level detail
- Memory access patterns and state changes
- Timing information for performance analysis

### Environment Documentation
- Platform information (OS, architecture)
- T81 version and build configuration
- Environment variables and system settings
- Hardware configuration details

### Performance Metrics
- Execution time measurements
- Memory usage tracking
- Resource utilization monitoring
- Custom metrics per workload type

## Validation Process

1. **Multiple Executions**: Run workload 2+ times with identical inputs
2. **Hash Comparison**: Verify input/output hash consistency
3. **Trace Analysis**: Compare execution traces for differences
4. **Statistical Analysis**: Calculate variance within tolerance bounds
5. **Report Generation**: Create comprehensive evidence report

## Output Artifacts

### Evidence Report (`evidence_report.json`)
```json
{
  "metadata": {
    "timestamp": "2026-03-05 01:00:00",
    "platform": "macOS-ARM64",
    "t81_version": "v1.2.1-experimental",
    "experiment_name": "my_experiment",
    "mode": "strict",
    "environment": {...}
  },
  "executions": [...],
  "validation": {
    "determinism_passed": true,
    "total_executions": 3,
    "validation_mode": "strict"
  },
  "performance": {
    "average_execution_time_ms": 150.5,
    "execution_count": 3
  }
}
```

### Validation Summary (`validation_results.json`)
```json
{
  "determinism_passed": true,
  "validation_timestamp": "2026-03-05 01:05:00",
  "experiment_name": "my_experiment"
}
```

## Integration with Promotion Gates

The validation summary (`validation_results.json`) is automatically consumed by the promotion gate system (RFC-00A0 Task 2) to validate determinism requirements for experiment promotion.

## Acceptance Criteria

- [x] Evidence collection protocol implemented
- [x] Validation test suite covers all modes
- [x] CLI tools provide complete evidence workflow
- [x] Performance overhead within specified limits (<15%)
- [x] Integration with existing T81 testing framework

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_evidence

# Run evidence collection
./build/experiments/ai/bin/t81_ai_evidence my_experiment strict ./evidence_output
```

## Determinism Validation Requirements

- **Cross-platform testing**: Validation on macOS (ARM64) and Linux (x86_64)
- **Statistical variance**: Variance measurement within tolerance bounds
- **Hash consistency**: Input/output hash consistency verification
- **Bit-exact reproducibility**: Strict mode requires identical results

---

**RFC Reference**: RFC-00A1  
**Task**: 3 - Build deterministic evidence collection framework  
**Status**: Completed  
**Last Updated**: 2026-03-05
