# CLI Exercise Findings - 2026-03-14

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI Exercise Findings - 2026-03-14](#cli-exercise-findings---2026-03-14)
  - [🧪 **Command-Line Utility Stress Test and Audit**](#🧪-**command-line-utility-stress-test-and-audit**)
    - [**❌ Identified Bugs**](#**❌-identified-bugs**)
      - [**1. Lossy Base81 Round-trip (CRITICAL)**](#**1-lossy-base81-round-trip-critical**)
    - [**⚠️ Inconsistencies & Behavioral Observations**](#**⚠️-inconsistencies-&-behavioral-observations**)
      - [**1. Redundant Compilation in `project run`**](#**1-redundant-compilation-in-`project-run`**)
      - [**2. IR Metadata Gaps**](#**2-ir-metadata-gaps**)
    - [**✅ Verified Working Surfaces**](#**✅-verified-working-surfaces**)
      - [**🎯 Project Lifecycle**](#**🎯-project-lifecycle**)
      - [**🔧 Diagnostics & Environment**](#**🔧-diagnostics-&-environment**)
      - [**🔍 Trace & Determinism**](#**🔍-trace-&-determinism**)
    - [**🚀 Conclusion**](#**🚀-conclusion**)

<!-- T81-TOC:END -->


## 🧪 **Command-Line Utility Stress Test and Audit**

I have performed a comprehensive exercise of the `t81` CLI utility, identifying several critical bugs and areas for improvement while confirming the overall robustness of the core workflow.

### **❌ Identified Bugs**

#### **1. Lossy Base81 Round-trip (CRITICAL)**
- **Issue**: The round-trip between binary TISC and Base81 symbolic representation is lossy.
- **Evidence**:
  - `t81 tisc decode hello.tisc -o hello.base81`
  - `t81 tisc encode hello.base81 -o hello_restored.tisc`
  - `t81 tisc diff hello.tisc hello_restored.tisc` -> **1 instruction(s) differ**
- **Root Cause**: The `base81_view.cpp` implementation only packs the opcode and operands A, B, and C. It completely ignores the `literal_kind` field of the instruction and does not serialize the symbol pool, float pool, or tensor metadata associated with the program.
- **Impact**: Any tool relying on Base81 for human-editable bytecode modification will produce corrupted or non-functional binaries.

### **⚠️ Inconsistencies & Behavioral Observations**

#### **1. Redundant Compilation in `project run`**
- **Behavior**: Running `t81 project run` always recompiles the source to a temporary `.tisc` file, even if a valid `main.tisc` exists in the project directory.
- **Recommendation**: Implement a dependency check (timestamp comparison) or a `--no-build` flag to allow running existing artifacts directly.

#### **2. IR Metadata Gaps**
- **Behavior**: The `t81 ir show` command reports the primitive kind as `[Unknown]` and boolean result as `false` for almost all instructions, including standard `LOADI` and `CALL` operations.
- **Impact**: Reduced utility for developers debugging the compiler frontend or IR lowering.

### **✅ Verified Working Surfaces**

#### **🎯 Project Lifecycle**
- `t81 project init`: Correctly scaffolds `main.t81` and `README.md`.
- `t81 project build`: Successfully compiles project source to a local `.tisc` artifact.

#### **🔧 Diagnostics & Environment**
- `t81 env doctor`: Provides accurate readiness reports and remediation steps.
- `t81 internal memory-stats`: Correctly reports peak stack/heap usage and runtime footprint.
- `t81 env clean`: Safely removes build metadata while preserving the `t81` tool itself.

#### **🔍 Trace & Determinism**
- `t81 trace replay`: Successfully verifies TISC execution against a recorded trace.
- `t81 determinism baseline`: Correctly generates canonical hash baselines for TISC corpora.

---

### **🚀 Conclusion**

While the `t81` CLI is a powerful and well-architected tool, the **lossy Base81 round-trip** is a significant regression that undermines the auditability and inspectability of the stack. Fixing the `base81_view` serialization to include literal metadata and program pools should be a priority.

**Audit Completed**: 2026-03-14
**Status**: ⚠️ BUGS IDENTIFIED
**Stability**: Core compilation/execution paths are stable (71/71 tests passing).
