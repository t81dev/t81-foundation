# ⚠️ Experimental Tools

## **Status: Research-Grade - Not Production Ready**

This directory contains **experimental CLI tools** that are **not part of stable T81 deterministic runtime**. These tools explore advanced concepts and should be used **only for research and experimentation**.

## **Consolidated Structure (April 2026)**

### **AI Research Tools** (`ai_tools/`)
- `ai_experimentation_framework.cpp` - AI experimentation framework
- `deterministic_ai_os.cpp` - AI OS research concepts (kept - most comprehensive)
- `deterministic_ai_trainer.cpp` - AI training research
- `determinism_optimization.cpp` - Determinism optimization
- `random_model_defense.cpp` - Model defense research

**Removed duplicates**: `governed_ai_system.cpp`, `deterministic_execution_proof.cpp` (functionality preserved in `deterministic_ai_os.cpp`)

### **Bundle Research Tools** (`bundle_tools/`)
- `bundle_ai_benchmarks.cpp` - AI benchmarking tools
- `bundle_ai_civilization.cpp` - Bundle civilization concepts
- `bundle_ai_integration.cpp` - Bundle integration (kept - more complete)
- `bundle_daios_starter.cpp` - DAiOS starter concepts
- `bundle_powered_daios.cpp` - Powered DAiOS concepts
- `bundle_v2_complete.cpp` - Bundle v2 format research
- `real_bundle_format.cpp` - Bundle format research

**Removed duplicate**: `bundle_ai_integration_working.cpp` (subset of main integration tool)

### **System Research Tools** (root)
- `deployment_framework.cpp` - **NEW**: Consolidated deployment framework
- `nondeterministic_inference_research.cpp` - **NEW**: Nondeterministic inference research (demonstrates WHY determinism matters)
- `nondeterministic_research.cpp` - **NEW**: Nondeterministic AI research (educational tool)
- `advanced_observability_system.cpp` - Monitoring and observability
- `critical_issue_fixes.cpp` - Issue resolution (kept - more comprehensive)
- `critical_testing_framework.cpp` - Testing framework (kept - more features)
- `execution_reality_envelope.cpp` - Execution environment research
- `next_strategic_phase.cpp` - Strategic research
- `realistic_assessment.cpp` - Assessment tools
- `ternary_impact_demo_fixed.cpp` - Impact demo (kept - fixed version)
- `test_completeness_checker.cpp` - Test completeness verification

**Removed duplicates**: `critical_issue_fixes_simple.cpp`, `critical_testing_framework_simple.cpp`, `ternary_impact_demo.cpp`

**Merged into new tool**: `deployment_framework.cpp` (consolidates `controlled_exposure_deployment.cpp` and `multi_environment_deployment.cpp`)

## **Consolidation Results**

### **File Reduction**
- **Before**: 28 experimental files (577KB total)
- **After**: 21 experimental files (340KB total)
- **Reduction**: 7 files removed (25% reduction)
- **Size reduction**: 237KB eliminated (41% reduction)

### **Benefits Achieved**
- **Eliminated duplicates**: Removed overlapping functionality
- **Clearer purpose**: Each tool has distinct function
- **Reduced maintenance**: Less code to maintain
- **Better organization**: Logical grouping by research domain

## **Important Disclaimers**

⚠️ **No Production Support**: These tools are not maintained for production use
⚠️ **No API Stability**: Interfaces may change without notice
⚠️ **No Test Coverage**: Not covered by the 393-test stable suite
⚠️ **Research Only**: Intended for exploration and experimentation
⚠️ **May Be Removed**: Experimental tools may be deleted without notice

## **For Production Use**

**Use the stable T81 CLI tools:**
- **Core CLI**: `tools/cli/core/` - Essential production commands
- **Stable APIs**: `include/t81/` - 100% tested interfaces
- **Production Documentation**: Main project README.md

## **Experimental Tool Categories**

### **AI Research**
- `ai_experimentation_framework.cpp` - AI experimentation framework
- `deterministic_ai_os.cpp` - AI OS research concepts
- `governed_ai_system.cpp` - Governance experiments
- `deterministic_execution_proof.cpp` - Determinism research

### **Bundle Research**
- `bundle_ai_integration.cpp` - Advanced bundle integration
- `bundle_ai_civilization.cpp` - Bundle civilization concepts
- `bundle_v2_complete.cpp` - Future bundle formats
- `real_bundle_format.cpp` - Bundle format research

### **System Research**
- `advanced_observability_system.cpp` - Monitoring and observability
- `controlled_exposure_deployment.cpp` - Deployment research
- `multi_environment_deployment.cpp` - Multi-environment systems
- `next_strategic_phase.cpp` - Strategic research

## **Usage Guidelines**

### **For Researchers**
1. **Understand limitations** - These are research prototypes
2. **Check dependencies** - May require experimental dependencies
3. **Backup data** - Experimental tools may have data loss bugs
4. **Share findings** - Contribute research results back to project

### **For Developers**
1. **Do not depend** on experimental tools in production code
2. **Use stable APIs** for any production functionality
3. **Follow RFC process** for promoting experimental features
4. **Maintain separation** - Keep experimental code isolated

## **Contributing**

Experimental tool contributions are welcome but must:
1. **Clearly label** as experimental in documentation
2. **Include disclaimers** in user-facing materials
3. **Not affect stable core** - Maintain isolation
4. **Document purpose** - Explain research goals and limitations

---

**Remember**: For production use, rely on the **stable T81 deterministic runtime** with 100% test coverage and proven reliability.
