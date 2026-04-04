# Tooling Consolidation Plan

## **Problem Statement**

The T81 project has **201+ tooling files** creating unnecessary complexity and maintenance burden. This dilutes focus from the **stable deterministic runtime** that actually works.

## **Current Tooling Inventory**

### **Stable Core Tooling (KEEP)**
```
tools/cli/core/                    ✅ Essential CLI only
├── main.cpp                     ✅ Primary CLI entry point
├── driver.cpp                    ✅ Core CLI functionality
├── debugger.cpp                   ✅ Debug interface
├── canonize_tensor.cpp            ✅ Tensor operations
└── artifact_family.cpp            ✅ Bundle management
```

### **Experimental Tooling (MOVE/CONSOLIDATE)**
```
tools/cli/experimental/              ❌ 13 experimental CLI files
tools/cli/bundle_tools/             ❌ 8 advanced bundle concepts
tools/cli/ai_tools/                ❌ 7 AI-specific tooling
tools/cli/ai/                       ❌ 4 AI interface files
```

### **Redundant/Duplicate Tooling**
```
tools/cli/ai/ vs tools/cli/ai_tools/     ❌ Overlapping AI functionality
experimental/ai/ vs experimental/ai_governance/  ❌ Duplicate governance concepts
Multiple demo directories                          ❌ Similar showcase purposes
```

## **Consolidation Strategy**

### **Phase 1: Move Experimental to Clear Boundary**

**Target Directory Structure:**
```
tools/
├── cli/
│   ├── core/                    ✅ Stable CLI (keep)
│   └── experimental/           🔄 Move from tools/cli/experimental/
├── model/                     ✅ Model handling (keep)
├── diagnostics/               ✅ Diagnostics (keep)
└── experimental/              🆕 Create new experimental boundary
    ├── ai_tools/              🔄 Move from tools/cli/ai_tools/
    ├── bundle_tools/           🔄 Move from tools/cli/bundle_tools/
    ├── demos/                  🔄 Consolidate from multiple demo dirs
    └── research/               🆕 For experimental research
```

### **Phase 2: Consolidate Overlapping Functionality**

**AI Tooling Consolidation:**
- Merge `tools/cli/ai/` and `tools/cli/ai_tools/`
- Remove duplicate AI interface implementations
- Keep only essential AI CLI commands

**Demo Consolidation:**
- Consolidate similar demo purposes
- Remove redundant showcase code
- Keep only representative examples

**Bundle Tool Consolidation:**
- Merge overlapping bundle functionality
- Remove experimental bundle concepts
- Keep only production bundle operations

### **Phase 3: Documentation and Disclaimers**

**Add Clear Disclaimers:**
- All experimental tools labeled as research-grade
- Clear separation from stable CLI
- No production guarantees for experimental

**Update Documentation:**
- Clear tooling hierarchy
- Usage guidelines for stable vs experimental
- Maintenance responsibility boundaries

## **Implementation Priority**

### **High Priority (Immediate)**
1. **Create experimental boundary** - Move experimental CLI tools
2. **Consolidate AI tooling** - Merge duplicate AI functionality
3. **Update stable CLI docs** - Clear stable tooling scope

### **Medium Priority (Next Sprint)**
1. **Demo consolidation** - Remove redundant demos
2. **Bundle tool cleanup** - Simplify bundle operations
3. **Experimental documentation** - Clear research guidelines

### **Low Priority (Future)**
1. **Advanced experimental cleanup** - Remove unused research prototypes
2. **Tooling automation** - Reduce manual tooling maintenance
3. **Integration testing** - Verify stable/experimental separation

## **Success Metrics**

### **Quantitative Goals:**
- **Reduce tooling files by 50%** (from 201+ to ~100)
- **Eliminate duplicate functionality** (AI tools, demos, bundles)
- **Clear stable/experimental boundary** (no overlap)

### **Qualitative Goals:**
- **Clearer project focus** (deterministic runtime)
- **Reduced maintenance burden** (less code to maintain)
- **Better developer experience** (simpler tooling choices)
- **Clearer user messaging** (stable vs experimental)

## **Risk Mitigation**

### **Preserve Value:**
- **Document experimental research** before moving
- **Maintain access to experimental features** for researchers
- **Keep migration paths** for experimental users

### **Avoid Disruption:**
- **Phase implementation** to avoid breaking changes
- **Clear communication** about tooling changes
- **Fallback options** for experimental tooling users

---

This consolidation will **reduce complexity**, **focus development effort**, and **clarify project identity** while preserving both the stable runtime and experimental research value.
