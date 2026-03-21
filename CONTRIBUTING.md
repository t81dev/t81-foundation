# Contributing to T81 Foundation

Thanks for contributing to T81 Foundation.

This repository is **spec-first** and **determinism-first**. Changes must preserve canonical behavior and reproducible outputs.

______________________________________________________________________

## 1. Ground Rules

- Specs in `spec/` are normative.
- Do not change normative spec language (`MUST`, `SHOULD`, etc.) without an RFC.
- Keep behavior deterministic; no hidden non-determinism.
- Add tests for any behavior change.
- Follow the Code of Conduct.

______________________________________________________________________

## 2. Contribution Workflow

1. Open an issue (bug, clarification, or RFC proposal).
2. Discuss scope and acceptance criteria.
3. Open a PR linked to the issue.
4. Run required local ritual before requesting review.
5. Address review feedback; merge when checks are green.

______________________________________________________________________

## 3. Required Local Ritual

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Single-threaded safe mode:

```bash
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure -j1
```

Optional extended suite:

```bash
ctest --test-dir build -R "fuzz|property|axion" --schedule-random
```

See `docs/ci.md` for full CI parity commands.

______________________________________________________________________

## 4. RFC Process (Spec Changes)

For spec/governance changes:

- Add/update RFC under `spec/rfcs/`.
- Link RFC in PR.
- Keep behavior and wording changes traceable.

______________________________________________________________________

## 5. Code and Docs Expectations

- Public API: `include/t81/`
- Implementation: `src/`
- Tests: `tests/cpp/`
- Docs: `docs/`
- Root-file policy: new root-level files are prohibited without explicit approval (see `docs/governance/SPEC_AUTHORITY_MODEL.md`).
- Records/archive policy: operational reports and snapshots belong under `docs/records/` rather than repository root.

When changing code:
- update tests,
- update relevant docs,
- keep examples runnable.
- Ensure code is formatted according to `.clang-format`.

______________________________________________________________________

## 6. Determinism and Safety

All contributions must preserve:

- encode/decode round-trip invariants,
- overflow trap behavior (Axion-visible),
- canonical serialization/replay behavior,
- reproducibility gates (T81Lang/T3_K, where applicable).

______________________________________________________________________

## 7. Formatting

We enforce a consistent coding style using `clang-format`. Please run the following command before submitting a PR:

```bash
# Format all C++ source files
find src include tests examples -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs clang-format -i
```

Ensure your `clang-format` version is 18 or higher.

______________________________________________________________________

## 8. License

By contributing, you agree your changes are licensed under the repository license terms.

______________________________________________________________________

## 9. Publishing Python Wheels to PyPI

The [`python-wheels`](.github/workflows/python-wheels.yml) workflow publishes binary wheels for CPython 3.9–3.13 on Linux (x86\_64, ARM64), macOS (arm64, x86\_64), and Windows on every `v*.*.*` release tag.

**Authentication** uses [OIDC Trusted Publishing](https://docs.pypi.org/trusted-publishers/) — no stored API tokens are needed. Before the first release, a maintainer must register the trusted publisher once on PyPI:

1. Log in to [pypi.org](https://pypi.org) as an owner of the `t81` project.
2. Navigate to **Your projects → t81 → Publishing**.
3. Click **Add a new pending publisher** and fill in:
   - **Owner:** `t81dev`
   - **Repository:** `t81-foundation`
   - **Workflow filename:** `python-wheels.yml`
   - **Environment name:** `pypi` (matches the `environment:` key in the workflow)
4. Save. No further configuration is needed — GitHub Actions will authenticate automatically on the next tagged release.

To trigger a release:

```sh
git tag v1.9.3
git push origin v1.9.3
```

The workflow runs cibuildwheel across all platforms, then uploads via `pypa/gh-action-pypi-publish`.
