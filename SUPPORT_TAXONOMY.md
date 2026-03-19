# T81 Foundation Support Taxonomy

This taxonomy explicitly categorizes all top-level directories within the T81 Foundation repository into three strict tiers, governing their relationship to the Deterministic Core Profile (DCP) and standard maintenance guarantees.

## Authoritative (DCP Core)
These directories define the core, bit-exact determinism guarantees of the T81 stack. They are strictly governed, and changes here require formal review and verification.

* `/core` - The verified execution core (Data Types, ISA, VM).
* `/spec` - The normative specifications (ISA, VM, Lang, Axion).
* `/lang` - The T81Lang compiler frontend and standard library.

## Maintained Support
These directories provide essential infrastructure, build tools, and verification pathways. They are fully supported but do not define execution semantics.

* `/tooling` - Official command-line tools, debuggers, and TUI frontends.
* `/benchmarks` - Regresssion and performance tracking workloads.
* `/tests` - Comprehensive test suites (unit, e2e, conformance).
* `/docs` - System documentation, status boards, and process guides.
* `/scripts` - CI, governance, and development scripts.
* `/include` - Public C++ headers for the core API.

## Experimental / Research (Non-DCP)
These directories contain research prototypes, advanced conceptual models, and aspirational work. They are explicitly excluded from the Deterministic Core Profile and carry no stability or reproducibility guarantees.

* `/experimental` - Unverified cognitive tiers, distributed logic, and HanoiVM stubs.
* `/runtime` - The experimental Trace-JIT compiler and related optimizations.
* `/experiments` - Exploratory AI and inference integration paths.
* `/kernel` - Early-stage Axion governance implementation paths and prototypes.
* `/qemu` - Virtualization targets for TernaryOS boot testing.
* `/examples` - Non-normative usage demonstrations.
* `/src` - Partially decoupled stubs (CanonFS, codec logic).
* `/legacy` - Archived historical code.
* `/artifacts` - Build output and trace cache.
* `/assets` - Binary assets (images, logos).
* `/book` - Narrative user guide.
* `/cmake` - Build system modules.
* `/contracts` - Experimental smart-contract mappings.
* `/internal` - Internal tooling.
* `/logs` - Audit output.
* `/notebooks` - Research notebooks.
* `/pdf` - Rendered documentation.
* `/third_party` - Embedded dependencies (llama.cpp).
* `/tools` - Miscellaneous utility scripts.
* `.devcontainer` - Development environment configuration.
* `.git`, `.github`, `.t81` - Project management and VCS metadata.
