# tools

Developer and inspection tools for the T81 Foundation ecosystem.

## **Structure Overview**

### **🔧 Stable Production Tools**
- **cli/core/**: Essential production CLI commands (100% tested)
- **model/**: Model-related tools and adapters (production-ready)
- **diagnostics/**: Diagnostic and metrics tools (production-ready)
- **generate_dummy_safetensors.cpp**: Utility for generating test tensors

### **⚠️ Experimental Research Tools**
- **experimental/**: Research-grade CLI tools (not production-ready)
  - `experimental/ai_tools/`: AI research frameworks
  - `experimental/bundle_tools/`: Advanced bundle concepts
  - `experimental/*.cpp`: System research prototypes

### **🛠️ Development Tools**
- **ir_inspector.cpp**: IR introspection helper
- **tisc_base81.cpp**: TISC/base81 utility tool
- **axion_policy_validator.py**: Policy validation helper
- **validator.py**: General validation utilities
- **vscode-t81/**: VS Code extension assets for syntax support
- **docker/**: Docker-related utilities

## **Production vs Experimental**

### **✅ Production Tools (Use for Systems)**
- **Location**: `cli/core/`, `model/`, `diagnostics/`
- **Test Coverage**: 100% (393/393 tests passing)
- **API Stability**: Stable with backward compatibility
- **Support**: Production-ready with documentation
- **Purpose**: Deterministic AI execution and governance

### **⚠️ Experimental Tools (Research Only)**
- **Location**: `experimental/`
- **Test Coverage**: Not covered by stable test suite
- **API Stability**: May change without notice
- **Support**: Research-grade only
- **Purpose**: AI OS research and advanced concepts

## **VS Code Extension**

The `vscode-t81/` directory contains a VS Code extension for T81 bundle language support:

### Features
- Syntax highlighting for `.t81-bundle` files
- Auto-closing brackets and quotes
- Code folding support with `#region`/`#endregion` markers
- Comment support (`//` for line comments, `/* */` for block comments)

### Installation
1. Copy `vscode-t81` directory to your VS Code extensions folder
2. Reload VS Code
3. The extension will automatically activate for `.t81-bundle` files

## **Usage Guidelines**

### **For Production Systems**
- Use tools in `cli/core/` for stable, tested functionality
- Rely on tools in `model/` and `diagnostics/` for production workflows
- Follow main `t81` CLI for integrated operations

### **For Research and Experimentation**
- Use tools in `experimental/` for exploring advanced concepts
- Understand that experimental tools have no production guarantees
- Contribute research findings back to the project

## **Development Guidelines**
- Keep tools deterministic and script-friendly.
- If a tool becomes core workflow, consider routing it through `t81` CLI surface.
- Maintain clear separation between production and experimental tools.
- Document experimental tool limitations and research goals.
