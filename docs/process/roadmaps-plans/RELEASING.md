# Release Policy

> **Source of Truth:** This document defines the **release process, artifact integrity, and signing policy**.

**Last Updated:** February 10, 2026

## 1. Release Types

- **Production Releases (vX.Y.Z):** Stable, semver-compliant releases.
- **Release Candidates (vX.Y.Z-rcN):** Pre-release builds for final validation.
- **Nightly/Dev Builds:** Automated builds from `main`.

## 2. Artifact Integrity

We guarantee the integrity of our release artifacts:

- **Reproducible Builds:** Release binaries are built using the [Reproducibility Guide](../../reference/REPRODUCIBILITY.md).
- **Checksums:** All artifacts are accompanied by SHA256 checksums (`SHA256SUMS.txt`).
- **Provenance:** We use GitHub Actions with `id-token: write` for attestations (future).

## 3. Signing Policy

Release tags and commits are signed by the Project Lead (`@t81dev`) or designated Release Managers.

- **GPG Key:** `DA3E...` (Public key available on keyservers)
- **Verification:** `git tag -v v1.1.0`

## 4. Release Process

1.  **Freeze:** Code freeze on `main` branch.
2.  **Validate:** Run full `ci` suite + manual QA.
3.  **Tag:** Create signed tag `vX.Y.Z`.
4.  **Build:** CI builds release artifacts.
5.  **Publish:** Create GitHub Release with changelog and checksums.

## 5. Artifact Retention

- **Production Releases:** Retained indefinitely.
- **Release Candidates:** Retained for at least 1 year.
- **Nightly Builds:** Retained for 30 days.
