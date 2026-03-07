# Migration Map

This document outlines the planned movement of files and directories to achieve the new governance structure.

| Current Path | New Path | Rationale |
| :--- | :--- | :--- |
| `/benchmarks/` | `/benchmarks/` | Retain, but strictly for harness code. |
| `/benchmark_*.txt` | `/benchmarks/results/archive/` | Move raw artifacts out of root. |
| `/dummy.*` | `(Delete or Ignore)` | Generated artifacts should not be committed. |
| `/policy/` (historical) | `/docs/governance/archive/policy/` | Consolidate governance docs. |
| `/HANOIVM_OPCODE_REFERENCE.md` | `spec/tisc/opcode-unified-reference.md` | Canonical spec location (consolidated). |
| `/README.*.md` | `/docs/developer-guide/book/book-*/README.md` | (Optional) Can stay at root if policy allows, but preferably mirrored. |
| `/CODE_OF_CONDUCT.md` | `/docs/governance/CODE_OF_CONDUCT.md` | Centralize governance. |
| `/CONTRIBUTING.md` | `/docs/governance/CONTRIBUTING.md` | Centralize governance. |
| `/SECURITY.md` | `/docs/governance/SECURITY.md` | Centralize governance. |
| `/generate_dummy_safetensors.cpp` | `/tools/generators/safetensors.cpp` | Move ad-hoc scripts to tools. |
| `/docs/tutorials/` | `/docs/guides/tutorials/` | Consolidate learning materials. |
| `/docs/how-to/` | `/docs/guides/how-to/` | Consolidate learning materials. |
| `/docs/explanation/` | `/docs/architecture/` | Merge deep dives into architecture. |
| `/docs/reference/` | `/docs/spec/` | Merge reference material into specs. |
| `/docs/standards/` | `/docs/spec/standards/` | Consolidate standards. |
| `/docs/rfcs/` | `/docs/rfc/` | Rename for singular consistency. |

## Execution Strategy

1.  **Phase 1 (Docs)**: Move governance and policy files. Update links in `README.md`.
2.  **Phase 2 (Artifacts)**: `git mv` benchmark results and script files.
3.  **Phase 3 (Cleanup)**: Update `.gitignore` to prevent recurrence.
