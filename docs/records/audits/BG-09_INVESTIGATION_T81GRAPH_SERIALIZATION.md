# BG-09 Investigation: T81Graph Serialization

**Investigation ID:** BG-09-2026-03-04  
**Surface:** `T81Graph` + collection lang runtime  
**Issue:** `serialize_canonical()` exists in C++ headers but is never called from the language runtime  
**Status:** 🔍 **INVESTIGATION COMPLETE - SOLUTION IDENTIFIED**  

---

## 📋 **Issue Analysis**

### **Problem Statement:**
The T81Graph type has a `serialize_canonical()` method in C++ headers, but the language runtime never invokes this method for collection types and T81Graph.

### **Root Cause Identified:**
The current `std.collections.graph_canonical` function works with a `Vector[T81String]` representation `[from0,to0,from1,to1,...]` rather than actual T81Graph objects. The implementation manually constructs edge strings `"from->to"` and joins them with commas, completely bypassing the T81Graph's `serialize_canonical()` method.

### **Expected Outcome:**
- Language runtime invokes `serialize_canonical` for collection types and T81Graph
- Stable serialization signature verified by test

---

## 🔍 **Investigation Findings**

### **Phase 1: T81Graph Implementation ✅**
- **Location**: `/include/t81/types/T81Graph.hpp`
- **Method**: `serialize_canonical()` exists and is properly implemented
- **Format**: Returns structured JSON-like format with sorted edges
- **Template Support**: Supports different weight types with proper serialization

### **Phase 2: Language Runtime Analysis ✅**
- **Location**: `/include/t81/frontend/ir_generator.hpp`
- **Function**: `collections_graph_canonical` implemented
- **Issue**: Works with `Vector[T81String]` representation, not T81Graph objects
- **Problem**: Manual string construction bypasses T81Graph.serialize_canonical()

### **Phase 3: VM Format Value Analysis ✅**
- **Location**: `/core/vm/vm.cpp`
- **Function**: `format_value()` handles `ValueTag::SymbolicGraphHandle`
- **Issue**: Returns placeholder `"<graph#" + std::to_string(val_data) + ">"`
- **Missing**: No call to serialize_canonical() for graph objects

---

## 🎯 **Solution Strategy**

### **Option 1: Fix VM Format Value (Recommended)**
Modify the VM's `format_value()` function to call `serialize_canonical()` for `SymbolicGraphHandle` objects.

### **Option 2: Fix IR Generator**
Modify the `collections_graph_canonical` implementation to work with actual T81Graph objects.

### **Option 3: Hybrid Approach**
Fix both the VM format value and enhance the IR generator for better graph support.

---

## 🔧 **Implementation Plan**

### **Phase 1: VM Format Value Fix**
1. **Add serialize_canonical to SymbolicGraph**: Add method to `t81::cog::v1::SymbolicGraph`
2. **Update VM format_value**: Call serialize_canonical for SymbolicGraphHandle
3. **Test**: Verify graph printing works correctly

### **Phase 2: Enhanced Graph Support**
1. **Improve IR Generator**: Better integration with T81Graph objects
2. **Add Graph Type Support**: Proper T81Graph type handling in language runtime
3. **Test**: Verify std.collections.graph_canonical works with T81Graph

### **Phase 3: Determinism Verification**
1. **Create Tests**: Comprehensive tests for graph serialization
2. **Verify Stability**: Ensure serialization signature is stable
3. **Integration**: Test with existing graph test suite

---

## 🎯 **Next Steps**

**Immediate Action**: Fix VM format_value to call serialize_canonical for SymbolicGraph objects.

This will enable proper T81Graph serialization in the language runtime and resolve BG-09.
