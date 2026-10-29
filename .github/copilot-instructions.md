This file is an adapter; the canonical rules live in AGENTS.md and docs/explanation/ARCHITECTURE.md

# GitHub Copilot Instructions for T81 Foundation

You are assisting with an experimental computing architecture and systems research platform called **T81 Foundation**. Preserve the specified contracts of existing deterministic surfaces; no general performance or security superiority is assumed.

When generating code or suggestions in this repository:

1. **Respect the spec hierarchy**

   - Treat files in `spec/` as normative. Do not introduce behavior that contradicts them.
   - If behavior is unclear, recommend clarifying the spec before assuming semantics.

1. **Follow the architecture**

   - Public APIs live in `include/t81/*.hpp`.
   - Implementations include `vm/`, `fs/`, `kernel/axion/`, `lang/`, and `tools/`; `src/` assembles build targets. Consult `PROJECT_STRUCTURE.md`.
   - Tests live in `tests/cpp/`.
   - Preserve historical Hanoi/CWEB source and negative results; historical code is not a correctness oracle.

1. **Coding style**

   - Use modern C++20/23 features judiciously.
   - Prefer clear, deterministic code over clever tricks.
   - Add or update tests when changing behavior.

1. **Safety and Axion**

   - Any change touching Axion, cognitive tiers, or safety-critical paths should be conservative and well-commented.
   - Point humans back to `spec/axion-kernel.md` and `spec/cognitive-tiers.md` for major changes.

1. **Documentation**

   - When you create new types or behaviors, keep `spec/` and `docs/` in sync with the implementation.
   - Use `README.md`, `AGENTS.md`, `docs/explanation/ARCHITECTURE.md`, and `spec/index.md` as your starting context for planning larger changes.

1. **Research integrity**

   - Read the Master Continuity Charter and Research Charter under `docs/research/`; preserve canonical H01–H08 and distinguish later investigation identifiers.
   - Inspect evidence before implementation. Generated code and model-authored summaries are not independent validation.
   - Never invent benchmarks, citations, executed checks, or CI results. Report commands, environment, uncertainty, negative findings, and untested assumptions.
   - Prefer minimal reviewable changes and equivalent conventional controls over expanding the architecture.
