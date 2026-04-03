# tools

Developer/inspection tools that are not part of the main `t81` command path.

## Available Tools

- **ir_inspector.cpp**: IR introspection helper
- **tisc_base81.cpp**: TISC/base81 utility tool
- **axion_policy_validator.py**, **validator.py**: policy/validation helpers
- **vscode-t81/**: VS Code extension assets for syntax support
- **cli/**: CLI-related tools and utilities
- **model/**: Model-related tools and adapters
- **diagnostics/**: Diagnostic and metrics tools
- **docker/**: Docker-related utilities
- **generate_dummy_safetensors.cpp**: Utility for generating test tensors

## VS Code Extension

The `vscode-t81/` directory contains a VS Code extension for T81 bundle language support:

### Features
- Syntax highlighting for `.t81-bundle` files
- Auto-closing brackets and quotes
- Code folding support with `#region`/`#endregion` markers
- Comment support (`//` for line comments, `/* */` for block comments)

### Installation
1. Copy the `vscode-t81` directory to your VS Code extensions folder
2. Reload VS Code
3. The extension will automatically activate for `.t81-bundle` files

## Guidelines
- Keep tools deterministic and script-friendly.
- If a tool becomes core workflow, consider routing it through `t81` CLI surface.
