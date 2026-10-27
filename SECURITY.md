# Security Policy

> **Source of Truth:** This document defines the **supported versions** and **vulnerability reporting process**.

**Last Updated:** October 27, 2026

## 1. Research Scope and Security Evidence

T81 Foundation is an experimental systems research platform. Its policy and integrity mechanisms have bounded implementations and tests; this document does not certify production readiness, NIST compliance, comprehensive isolation, or comparative security superiority.

Evaluate each claim against its actual execution boundary:

- **Policy enforcement:** VM pre-dispatch and configured storage authorization have dedicated denial tests. Task admission, VM engines, low-level driver hooks, and host-side effects are distinct boundaries. The VM can default to an allow-all engine; low-level drivers expose bootstrap modes. Tests of one configured path do not establish universal bypass resistance. See [VM dispatch](vm/vm.cpp), [storage contract](include/t81/canonfs/canon_driver.hpp), and [bounded evidence](docs/research/consolidation/README.md).
- **Integrity:** content hashes and read verification can detect changed bytes relative to a trusted reference. They do not authenticate the producer, establish truthful provenance, or prove execution correctness. CanonFS read verification is configuration-dependent.
- **Determinism:** reproducible execution is scoped to specified operations, inputs, companion artifacts, configuration, and platforms. It is distinct from authorization, authentication, sandbox isolation, or malicious-host resistance.
- **Release evidence:** the [release workflow](.github/workflows/release.yml) defines artifact packaging and SBOM generation. Inspect the actual release artifacts and trust chain; workflow configuration alone does not prove signing or complete supply-chain provenance for every release.

Consult the [determinism threat model](docs/governance/DETERMINISM_THREAT_MODEL.md), [surface register](docs/governance/GOVERNANCE_SURFACE_REGISTER.md), and [disclosure procedure](docs/governance/INCIDENT_RESPONSE.md). No promotion of a surface or change to the threat model is made here.

## 2. Supported Versions

The declared maintenance policy covers v1.x; older releases are unsupported as listed below. This is a maintenance commitment, not an independent security certification of every release or experiment.

| Version    | Supported | Notes                                           |
| :--------- | :-------- | :---------------------------------------------- |
| **v1.x**   | ✅ Yes    | **Active:** Critical fixes and feature updates. |
| **v0.x**   | ❌ No     | **End of Life:** No longer supported.           |
| **< v1.0** | ❌ No     | Deprecated.                                     |

## 3. Reporting a Vulnerability

Please report vulnerabilities privately through GitHub Security Advisories.

If advisory reporting is unavailable, open a private maintainer contact with:

- affected component/path,
- impact summary,
- minimal reproduction,
- suggested remediation (if known).

Do **not** publish exploit details in public issues before a fix is available.

## 4. Scope

Security-relevant areas include:

- VM execution and memory safety boundaries,
- Axion policy enforcement surfaces,
- CanonFS persistence and trace integrity,
- CLI/runtime artifact handling,
- workflow/CI supply-chain integrity.

## 5. Response Goals

- Initial triage acknowledgment: best effort within 72 hours.
- Severity assessment and mitigation plan: as quickly as practical based on impact.
- Coordinated disclosure after fix availability.

## 6. Handling Expectations

- Provide deterministic reproduction steps where possible.
- Avoid including secrets in reports.
- If the issue affects reproducibility or contract integrity, include expected vs actual deterministic outputs.
