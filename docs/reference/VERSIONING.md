# Versioning and Compatibility Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Versioning and Compatibility Policy](#versioning-and-compatibility-policy)
  - [1. Versioning Scheme](#1-versioning-scheme)
  - [2. Compatibility Matrix](#2-compatibility-matrix)
  - [3. Spec vs. Runtime Versioning](#3-spec-vs-runtime-versioning)
  - [4. Support Window](#4-support-window)
  - [5. Deprecation Policy](#5-deprecation-policy)

<!-- T81-TOC:END -->


> **Source of Truth:** This document defines the normative versioning rules and backward compatibility guarantees for the project.

**Last Updated:** February 10, 2026

## 1. Versioning Scheme

T81 follows **[Semantic Versioning 2.0.0](https://semver.org/)** (`MAJOR.MINOR.PATCH`).

- **MAJOR**: Incompatible API or behavioral changes.
- **MINOR**: Backward-compatible functionality additions.
- **PATCH**: Backward-compatible bug fixes.

## 2. Compatibility Matrix

Not all changes trigger version bumps equally. We distinguish between the **Spec** (normative definition) and the **Runtime** (implementation).

| Change Type | Spec Version Impact | Runtime Version Impact | Example |
| :--- | :--- | :--- | :--- |
| **New TISC Opcode** | MINOR | MINOR | Adding `FSin` support. |
| **Removing TISC Opcode** | MAJOR | MAJOR | Removing `Call`. |
| **Trace Format Change** | PATCH | PATCH | Optimizing trace encoding (if decoded output is identical). |
| **Trace Semantics Change** | MAJOR | MAJOR | Changing the meaning of a trace event. |
| **Performance Optimization** | NONE | PATCH | Speeding up `T81BigInt` multiplication. |
| **Bug Fix (Correctness)** | PATCH | PATCH | Fixing an incorrect subtraction result. |

## 3. Spec vs. Runtime Versioning

- **Spec Version:** Tracks the evolution of the language and VM behavior.
- **Runtime Version:** Tracks the software implementation (`t81-foundation` releases).

Ideally, Runtime v1.2.x implements Spec v1.2.x. However, the Runtime may have a higher PATCH version than the Spec (e.g., Runtime v1.2.5 implementing Spec v1.2.0).

## 4. Support Window

We support the **current MAJOR version** and the **immediate previous MAJOR version** for critical security fixes.

| Version | Status | Security Support | Bug Fixes |
| :--- | :--- | :--- | :--- |
| **v1.x** | **Active** | ✅ Yes | ✅ Yes |
| **v0.x** | **End of Life** | ❌ No | ❌ No |

## 5. Deprecation Policy

1.  **Announcement:** Deprecations are announced in a MINOR release notes and marked in documentation.
2.  **Period:** Features remain deprecated but functional for at least one MINOR release cycle.
3.  **Removal:** Features are removed in the next MAJOR release.

**Example:**
- v1.5.0: Deprecates `old_function()`.
- v1.6.0: `old_function()` still exists but warns.
- v2.0.0: `old_function()` is removed.
