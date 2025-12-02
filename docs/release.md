---
layout: page
title: Release & Versioning
---

# T81 Foundation: Release & Versioning

This document spells out how we version binaries, prepare changelog entries, and cut releases so that the next maintainer can ship with confidence.

______________________________________________________________________

## 1. Versioning Policy

- We use **semantic versioning** (`MAJOR.MINOR.PATCH`).
- Non-breaking fixes increment `PATCH`; new features or significant improvements increment `MINOR`; API- or spec-breaking changes require a `MAJOR` bump and RFC-level approval.
- Tags follow the `vMAJOR.MINOR.PATCH` pattern (e.g., `v1.0.0`). Never force-push an annotated release tag.

## 2. Release Preparation Checklist

1. **Update Changelog & Release Notes:** Document the CLI diagnostic improvements (semantic/parsing errors now include `file:line:column`) in `CHANGELOG.md` and the GitHub release notes so downstream users know what to expect.
2. **Sync Docs:**
   - Refresh `docs/benchmarks.md`, `docs/onboarding.md`, and `docs/cpp-quickstart.md` if relevant (especially when CLI, tests, or build workflows change).
   - While running the release checklist, run `./build/t81 compile path/to/invalid.t81` (with a file that triggers a semantic error) to ensure diagnostics still print the `file:line:column` context described above and update the docs if the output format evolves.
3. **Run the Core Suite:**
   - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
   - `cmake --build build --parallel`
   - `ctest --test-dir build --output-on-failure`
   - Optionally, run `ctest --test-dir build -R "fuzz|property|axion" --schedule-random` before a major release.
4. **Generate Docs:** `cmake --build build --target docs` ensures `docs/api` is current.
5. **Tag & Push:** After verifying, create an annotated tag (`git tag -a vX.Y.Z -m "Release vX.Y.Z"`) and push both branch and tags.

## 3. Release Automation

- `.github/workflows/release.yml` runs on tag pushes and builds `t81`, publishes docs PDFs, and packages any release artifacts. Ensure the workflow passes before calling a release “done.”
- `release.yml` uses the results of `ci.yml` plus Bench and Docs workflows; the human reviewer should confirm these workflows in GitHub Actions before tagging.

## 4. Post-Release

- Create or update the GitHub release notes summarizing highlights and linking to changelog sections.
- Notify downstream users if there are breaking changes to Axion, CanonFS, or the `weights` tooling.
- Close any RFCs tied to the release iterations and update `spec/rfcs/` if the release introduces normative changes.

## 5. Questions for the Next Maintainer

- If Axion or CanonFS behavior changes, does the release pipeline require extra verification (e.g., hardware regression)? Document any manual steps now.
- Are there additional artifacts (benchmarks, PDFs) the release workflow should publish? If so, extend `release.yml` and note it here.
