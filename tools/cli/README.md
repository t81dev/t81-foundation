# `tooling/cli`

Command-line entry points and interactive debugger implementation.

## Scope
- `t81` CLI main program and command dispatch.
- Compile/run/disasm/debug command orchestration.
- Interactive debugger loop for VM execution.

## Key Files
- `main.cpp`: process entry point and top-level CLI wiring.
- `driver.cpp`: command handling and workflow logic.
- `debugger.cpp` / `debugger.hpp`: breakpoint-driven debugger over `IVirtualMachine`.

## Debugger Capabilities
- Register and memory inspection.
- Current-instruction display.
- Breakpoints (address and policy-related sets).

## Notes
- Keep CLI output stable where scripts/CI parse it.
- New user-facing flags should be reflected in root `README.md` and docs guides.
