# Pull Request: Experimental AI CLI Integration

## Summary

Introduce minimal experimental AI CLI system isolated from T81's deterministic core.

## Purpose

This PR implements the first working experimental AI CLI binary (`t81_ai`) while maintaining complete isolation from T81's deterministic core system. The implementation follows RFC-00A0 (Experiment Sandbox) and RFC-00A7 (UX Integration) requirements.

## Repository Isolation Guarantees

✅ **Core Protection**: Zero modifications to `/src`, `/include/t81`, `/spec`, `/tests`  
✅ **Experimental Isolation**: All AI code resides exclusively in `/experiments/ai/`  
✅ **Build Isolation**: AI experiments built only when explicitly enabled via `-DT81_ENABLE_AI_EXPERIMENTS=ON`  
✅ **Deterministic Core**: T81's deterministic core remains completely untouched  

## Implemented Commands

The minimal AI CLI supports these commands:

```bash
# Show help
./t81_ai --help

# Inspect model file (basic metadata)
./t81_ai model inspect <file>

# Verify model integrity (file existence + basic checks)
./t81_ai verify <file>
```

## Technical Implementation

### Architecture
- **Language**: C++23 with standard library only (no external dependencies)
- **Build System**: CMake integration with optional building
- **Location**: `/experiments/ai/ux_tools/t81_ai_minimal.cpp`
- **Binary**: `experiments/ai/ux_tools/t81_ai`

### Features
- **Command Parsing**: Basic argument parsing with error handling
- **File Operations**: Model inspection and verification
- **Error Handling**: Graceful handling of missing files and invalid commands
- **Help System**: Comprehensive usage documentation

## Verification Steps

### Build Verification
```bash
# Configure with AI experiments
cmake .. -DT81_ENABLE_AI_EXPERIMENTS=ON

# Build AI CLI
make t81_ai

# Test functionality
./experiments/ai/ux_tools/t81_ai --help
./experiments/ai/ux_tools/t81_ai model inspect test_file.gguf
./experiments/ai/ux_tools/t81_ai verify test_file.gguf
```

### Automated Testing
```bash
# Run smoke test
./scripts/test_ai_simple.sh
```

## RFC Compliance

### ✅ RFC-00A0 (AI Experiment Sandbox)
- **Isolation**: All AI code in `/experiments/ai/` only
- **Build Control**: Optional building via CMake flag
- **Core Protection**: No modifications to deterministic core
- **Sandbox Boundary**: Clear experimental/stable separation

### ✅ RFC-00A7 (UX Integration)
- **CLI Surface**: Basic command structure implemented
- **Help System**: Comprehensive usage documentation
- **Error Handling**: Proper error messages and exit codes
- **User Experience**: Simple, intuitive command interface

## Files Changed

### Core Integration
- `CMakeLists.txt` - Added AI experiments build integration

### Documentation
- `docs/experiments/AI_EXPERIMENTS.md` - Comprehensive AI experiments guide
- `README.md` - Added AI CLI usage section
- `docs/status/STABLE_BASELINE.md` - Added AI experiments note

### Implementation
- `/experiments/ai/` - Complete minimal AI CLI implementation
  - `CMakeLists.txt` - Build configuration
  - `ux_tools/t81_ai_minimal.cpp` - Main CLI implementation
  - `ux_tools/CMakeLists.txt` - CLI build configuration

### Testing
- `scripts/test_ai_simple.sh` - Automated smoke test script

## Testing

- ✅ Build system integration verified
- ✅ All CLI commands tested
- ✅ Error handling verified
- ✅ Core isolation confirmed
- ✅ Documentation complete

## Next Steps

This minimal implementation provides foundation for incremental AI feature development:

**Candidate Next Milestone**: Improved model metadata inspection
- Parse actual model formats (GGUF, ONNX)
- Extract real model metadata
- Display detailed model information
- Maintain deterministic approach

## Risk Assessment

**Low Risk** - This PR:
- Does not modify any core T81 systems
- Maintains complete isolation from deterministic components
- Uses only standard C++ library (no new dependencies)
- Includes comprehensive testing and documentation

## Review Checklist

- [x] Code follows T81 coding standards
- [x] All tests pass
- [x] Documentation is complete
- [x] RFC compliance verified
- [x] Core isolation maintained
- [x] Build system integration tested
- [x] Error handling implemented

---

**Status**: Ready for review  
**RFC References**: RFC-00A0, RFC-00A7  
**Isolation**: Complete - No core modifications

## Evidence for Reviewers

**Reproducibility Evidence**: `/docs/status/AI_CLI_MILESTONE_EVIDENCE.md`  
**CI Workflow Confirmation**: `/docs/status/CI_WORKFLOW_CONFIRMATION.md`

Reviewers can independently verify this milestone using the comprehensive evidence documentation above.
