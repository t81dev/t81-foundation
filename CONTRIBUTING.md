# Contributing to T81 Foundation

Thanks for contributing to T81 Foundation.

T81 Foundation is an experimental computing architecture and systems research platform. Changes to existing deterministic surfaces must preserve their specified canonical behavior and reproducible outputs. Experiments may investigate other execution models when their boundaries are explicit.

Read the [research charter](docs/research/RESEARCH_CHARTER.md), [Master Continuity Charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md), and [hypothesis register](docs/research/continuity/HYPOTHESIS_REGISTER_v1.json) before proposing research. Canonical H01–H08 identifiers and Hanoi/CWEB history must remain traceable; numbered Scientific Investigations do not redefine them.

______________________________________________________________________

## 1. Ground Rules

- Specs in `spec/` are normative.
- Do not change normative spec language (`MUST`, `SHOULD`, etc.) without an RFC.
- Keep behavior deterministic; no hidden non-determinism.
- Add tests for any behavior change.
- Follow the Code of Conduct.
- Preserve negative and inconclusive findings. A proposed architecture, generated implementation, passing test, and comparative advantage are different evidence levels.

______________________________________________________________________

## 2. Contribution Workflow

1. Open an issue (bug, clarification, or RFC proposal).
1. Discuss scope and acceptance criteria.
1. Open a PR linked to the issue.
1. Run required local ritual before requesting review.
1. Address review feedback and applicable governance requirements. Required checks support review; a green badge alone does not establish correctness, research validity, or merge authorization.

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

Use a C++23 toolchain, CMake (3.25+ recommended), and Python 3.10+ for the test/research tooling. Ninja is required for the research runner. Default configuration may download FTXUI, Asio, and benchmark dependencies; optional experiments have additional prerequisites. Consult [build options](cmake/Options.cmake) and [CI guidance](docs/reference/ci.md), and report the configuration actually tested. Test counts vary with enabled options.

The opt-in research slice uses a separate build directory and requires OpenSSL 3 with Ed25519 support:

```bash
python3 scripts/ci/run_research_regressions.py --build-dir build-research
```

For offline research configuration, append `--asio-source /path/to/asio-1-30-2`. This runner executes existing regression cases; it does not reproduce historical benchmark measurements. Documentation-only changes may use relevant link, formatting, configuration, and research checks; list omitted checks and limitations explicitly.

______________________________________________________________________

## 4. RFC Process (Spec Changes)

For spec/governance changes:

- Add/update RFC under `spec/rfcs/`.
- Link RFC in PR.
- Keep behavior and wording changes traceable.

______________________________________________________________________

## 5. Code and Docs Expectations

- Public API: `include/t81/`
- Implementation: `vm/`, `fs/`, `kernel/axion/`, `lang/`, `tools/`, and other subsystem directories; `src/` primarily assembles CMake targets (see [project structure](PROJECT_STRUCTURE.md)).
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

## Research and Reproduction Contributions

- **Bug fixes:** include a minimal reproducer, affected commit/configuration, expected contract, and a regression test for changed behavior.
- **Research hypotheses:** identify the canonical parent hypothesis or explicitly separate question, domain, assumptions, strongest feasible baseline, possible falsifier, and stopping criteria before implementation.
- **Experimental implementations:** identify build flags and integration boundaries; do not promote experimental code to a public contract merely because it builds.
- **Benchmarks:** establish equivalent computation, accuracy and authorization/evidence requirements; record setup, model/input identity, hardware, OS, toolchain, build flags, warm/cold boundaries, repetitions, uncertainty, and raw observations. Include conversion and persistence costs. Do not compare a cold CLI launch with an in-process lookup as architectural evidence.
- **Reproduction evidence:** distinguish commands actually run from reported historical results, mathematical deductions, models, and untested assumptions. Independent verification must describe what is independent.
- **Documentation corrections:** link the source, accepted specification, or corrective experiment supporting the change. Qualify historical reports rather than silently rewriting their results.
- **Negative or inconclusive findings:** retain failed cases and explain the tested scope, confounders, and what remains open. Hypothetical hardware is not evidence of current performance.

See the [consolidation ledger](docs/research/consolidation/README.md) for bounded evidence and the [root alignment audit](docs/research/ROOT_DOCUMENTATION_ALIGNMENT_AUDIT.md) for current entry-point limitations.

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

The [`python-wheels`](.github/workflows/python-wheels.yml) workflow publishes binary wheels for CPython 3.9–3.13 on Linux (x86_64, ARM64), macOS (arm64, x86_64), and Windows on every `v*.*.*` release tag.

**Authentication** uses [OIDC Trusted Publishing](https://docs.pypi.org/trusted-publishers/) — no stored API tokens are needed. Before the first release, a maintainer must register the trusted publisher once on PyPI:

1. Log in to [pypi.org](https://pypi.org) as an owner of the `t81` project.
1. Navigate to **Your projects → t81 → Publishing**.
1. Click **Add a new pending publisher** and fill in:
   - **Owner:** `t81dev`
   - **Repository:** `t81-foundation`
   - **Workflow filename:** `python-wheels.yml`
   - **Environment name:** `pypi` (matches the `environment:` key in the workflow)
1. Save. No further configuration is needed — GitHub Actions will authenticate automatically on the next tagged release.

To trigger a release:

```sh
git tag v1.9.3
git push origin v1.9.3
```

The workflow runs cibuildwheel across all platforms, then uploads via `pypa/gh-action-pypi-publish`.
