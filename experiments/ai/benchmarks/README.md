# T81 AI Benchmark Suite - RFC-00A2 Task 4

This directory contains the standardized benchmark specification and reporting format for AI workloads in the T81 ecosystem.

## Components

### Benchmark Runner (`t81_ai_benchmark`)
CLI tool for running standardized AI benchmarks with reproducible execution and comprehensive reporting.

**Usage:**
```bash
# Run inference benchmark
./t81_ai_benchmark ./output inference model_12345 "Hello, world!" 10

# Run quantization benchmark
./t81_ai_benchmark ./output quantization model_12345 T3_K2

# Generate benchmark report
./t81_ai_benchmark ./output report
```

## Benchmark Classification System

### Inference Benchmarks
- **TTFT (Time To First Token)**: Time until first token generation
- **TPOT (Tokens Per Second)**: Token generation throughput
- **Throughput**: Tokens per minute sustained rate
- **Latency**: Average response time
- **Memory Usage**: Peak memory consumption

### Quantization Benchmarks
- **Memory Reduction**: Compression ratio compared to original
- **Accuracy Impact**: Quality degradation percentage
- **Compression Ratio**: Size reduction factor
- **Processing Time**: Quantization execution time

### Conversion Benchmarks
- **Conversion Speed**: Time to convert between formats
- **Size Efficiency**: Output size vs input size
- **Fidelity**: Information preservation metrics

## Standardized Metrics

### Performance Metrics
- **ttft_ms**: Time to first token in milliseconds
- **tpot_tokens_per_sec**: Tokens generated per second
- **throughput_tokens_per_min**: Sustained tokens per minute
- **latency_ms**: Average response latency
- **memory_usage_bytes**: Peak memory consumption

### Quality Metrics
- **accuracy_percent**: Model accuracy preservation
- **compression_ratio**: Size compression factor
- **fidelity_score**: Information preservation score

### Environment Documentation
- **Platform**: OS and architecture information
- **Hardware**: CPU, memory, GPU details
- **Software**: T81 version, compiler, libraries
- **Configuration**: Environment variables and settings
- **Timestamp**: Benchmark execution time

## Benchmark Report Format

### Environment Section
```json
{
  "environment": {
    "platform": "macOS-ARM64",
    "os_version": "1.0.0",
    "hardware": "CPU: 8 cores, RAM: 16GB, GPU: None",
    "t81_version": "v1.2.1-experimental",
    "timestamp": "2026-03-05 01:00:00",
    "compiler_version": "Clang 15.0.0",
    "libraries": ["t81-core", "openssl", "nlohmann_json"],
    "environment_vars": {
      "PATH": "/usr/local/bin:/usr/bin",
      "T81_DETERMINISM_MODE": "strict"
    }
  }
}
```

### Results Section
```json
{
  "results": [
    {
      "benchmark_id": "bench_1642345678900",
      "model_id": "model_12345",
      "benchmark_class": "inference",
      "status": "completed",
      "execution_time_ms": 1500,
      "memory_usage_peak_bytes": 1073741824,
      "metrics": {
        "ttft_ms": 125.50,
        "tpot_tokens_per_sec": 15.25,
        "throughput_tokens_per_min": 915.0,
        "latency_ms": 1500.0,
        "accuracy_percent": 98.5
      },
      "errors": [],
      "additional_info": {
        "warmup_time_ms": 100,
        "model_size_mb": 1100
      }
    }
  ]
}
```

### Summary Section
```json
{
  "summary": {
    "total_benchmarks": 5,
    "by_class": {
      "inference": 3,
      "quantization": 2
    },
    "successful_runs": 5
  }
}
```

## Determinism Integration

The benchmark suite integrates with the deterministic evidence collection framework (RFC-00A1):

### Reproducible Execution
- **Multiple Runs**: Each benchmark runs multiple times for statistical significance
- **Environment Capture**: Complete environment documentation for reproducibility
- **Hash Verification**: Input/output hash consistency validation
- **Determinism Mode**: Support for strict, statistical, and reproducible non-deterministic modes

### Validation Process
1. **Environment Documentation**: Capture all system and software configuration
2. **Multiple Executions**: Run each benchmark multiple times
3. **Statistical Analysis**: Calculate averages, variance, and confidence intervals
4. **Cross-Platform Testing**: Validate across different platforms
5. **Determinism Verification**: Ensure results are reproducible

## Acceptance Criteria

- [x] Standard benchmark suite implemented with reference models
- [x] Reporting format supports all specified metrics
- [x] CI/CD integration working across platforms
- [x] Determinism validation integrated with benchmarks
- [x] Performance overhead within acceptable limits

## Build Instructions

```bash
# Enable AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON
make t81_ai_benchmark

# Run benchmark suite
./build/experiments/ai/bin/t81_ai_benchmark ./output inference model_12345 "Hello, world!" 10
```

## Integration with T81 Ecosystem

The benchmark suite integrates with:
- **Determinism Framework** (RFC-00A1) for reproducible execution
- **Model Provenance** (RFC-00A3) for model integrity verification
- **CLI Tools** (RFC-00A7) for unified command interface
- **CI/CD Pipeline** for automated benchmark execution

---

**RFC Reference**: RFC-00A2  
**Task**: 4 - Create standard benchmark suite  
**Status**: Completed  
**Last Updated**: 2026-03-05
