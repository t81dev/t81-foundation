# Generated Artifacts Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Generated Artifacts Policy](#generated-artifacts-policy)
  - [Scope](#scope)
  - [Doxygen Output](#doxygen-output)
  - [Contribution Rules](#contribution-rules)

<!-- T81-TOC:END -->


This repository does not track generated build artifacts in Git.

## Scope

Generated artifacts include (non-exhaustive):

- Doxygen HTML output
- Build directories (`build/`, CMake/Ninja intermediates)
- Local test logs and temporary files

## Doxygen Output

- Canonical output path: `build/api/html`
- Entry page: `build/api/html/index.html`
- Generation command:

```bash
cmake --build build --target docs
```

`docs/api/` is reserved for authored documentation, not generated HTML snapshots.

## Contribution Rules

- Do not commit generated files from local builds.
- If a generated file appears in `git status`, add/update ignore rules rather than committing output.
- CI and release workflows are responsible for producing distributable artifacts when needed.

