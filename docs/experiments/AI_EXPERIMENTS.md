# T81 AI Experiments

## Overview

This document describes the experimental AI CLI system introduced under `/experiments/ai/`.

## Purpose

The T81 AI Experiments provide a **minimal, isolated command-line interface** for AI-related operations while maintaining the deterministic guarantees of the T81 core system.

## Architecture

### Isolation Guarantees

- **No Core Integration**: All AI code resides in `/experiments/ai/` only
- **No Core Modifications**: Zero changes to `/src`, `/include/t81`, `/spec`, `/tests`
- **Build Isolation**: AI experiments built only when explicitly enabled
- **Deterministic Core**: T81's deterministic core remains untouched

### Experimental Status

All AI features are **experimental** and may change without notice. They are not part of the stable T81 foundation.

## Building AI Experiments

### Prerequisites

- CMake 3.16+
- C++23 compatible compiler
- No external dependencies required

### Compiler Compatibility

**Supported Compilers:**
- **Apple Clang 17.0.0**: ✅ Verified and tested
- **GCC**: ⚠️ Untested (not available on test systems)

**Known Issues:**
- **Homebrew Clang 21.1.8**: ⚠️ Linker compatibility issues with newer libstdc++
- **Recommendation**: Use system Apple Clang on macOS for best compatibility

### Build Commands

```bash
# Configure build with AI experiments enabled
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON

# Build AI CLI
make t81_ai

# Alternative: Build all experiments
make ai_experiments
```

### Installation

The AI CLI is installed to: `experiments/ai/bin/t81_ai`

## Usage

### Current Commands

The minimal AI CLI supports these commands:

```bash
# Show help
./t81_ai --help

# Inspect model file
./t81_ai model inspect <file>

# Verify model integrity
./t81_ai verify <file>
```

### Examples

```bash
# Get help
./t81_ai --help

# Inspect a model file
./t81_ai model inspect model.gguf

# Verify model integrity
./t81_ai verify model.gguf
```

## Current Implementation Status

### ✅ Completed Features

- **Minimal CLI Framework**: Command parsing and error handling
- **Model Inspection**: Basic file metadata and size reporting
- **Model Verification**: File existence and integrity checks
- **Build Integration**: Proper CMake integration with optional building
- **Error Handling**: Graceful handling of missing files and invalid commands

### 🚧 Future Features (Not Implemented)

- **Real Model Parsing**: Actual GGUF/ONNX format support
- **Model Loading**: Integration with T81 model provenance system
- **Inference**: AI model execution with deterministic guarantees
- **Quantization**: T81 ternary quantization codecs
- **Benchmarking**: Performance measurement and reporting
- **Policy Enforcement**: Axion policy system integration

## Development Guidelines

### Adding New Features

1. **Stay Isolated**: All new code must remain in `/experiments/ai/`
2. **No Core Changes**: Never modify T81 core systems
3. **Incremental Development**: Add features one at a time
4. **Maintain Determinism**: Ensure all operations are deterministic
5. **Document Changes**: Update this document for new features

### Build System Integration

New AI features should:

1. Add subdirectories to `/experiments/ai/CMakeLists.txt`
2. Use `if(T81_ENABLE_AI_EXPERIMENTS)` guards
3. Link only to `t81::core` when needed
4. Avoid external dependencies unless absolutely necessary

## Security Considerations

- **Sandboxed Execution**: All AI operations run in experimental sandbox
- **No Core Impact**: AI experiments cannot affect deterministic core
- **File System Access**: Limited to explicit user-provided files
- **Network Access**: No network access in current implementation

## Troubleshooting

### Build Issues

**Issue**: `t81_ai` target not found
**Solution**: Ensure `-DT81_ENABLE_AI_EXPERIMENTS=ON` is set during CMake configuration

**Issue**: CMake version errors
**Solution**: Ensure CMake 3.16+ is installed

### Runtime Issues

**Issue**: Command not found
**Solution**: Check if binary exists in `experiments/ai/bin/t81_ai`

**Issue**: Permission denied
**Solution**: Ensure binary has execute permissions

## Related RFCs

- **RFC-00A0**: AI Experiment Sandbox - Provides isolation framework
- **RFC-00A7**: UX Integration - Defines CLI interface requirements

## Version History

- **v0.1.0** (2026-03-05): Initial minimal AI CLI implementation
  - Basic command framework
  - Model inspection and verification
  - Build system integration
  - Documentation and examples

---

**Status**: Experimental - Use at your own risk  
**Maintainer**: T81 Development Team  
**Last Updated**: 2026-03-05
