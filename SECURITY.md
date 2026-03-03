# Security Policy

> **Source of Truth:** This document defines the **supported versions** and **vulnerability reporting process**.

**Last Updated:** February 10, 2026

## 1. NIST Compliance & High-Assurance AI Infrastructure

We are aligning the T81 Foundation stack with **NIST SP 800-218 (SSDF)** and **NIST SP 800-53** controls. This is critical for maintaining a high-assurance, auditable AI infrastructure.

Our core security postures include:
- **Deterministic Execution:** Bit-exact reproducibility enforcing strict supply-chain and execution trace verification.
- **Opcode-Level Policy Enforcement:** The Axion kernel enforces security and alignment policies directly at the VM instruction level.
- **Signed Releases & SBOM:** Release artifacts are cryptographically signed and accompanied by Software Bill of Materials (SBOM) for complete supply-chain provenance.
- **Formal Threat Model:** A robust, continually updated threat model (`docs/governance/DETERMINISM_THREAT_MODEL.md`) paired with a Coordinated Vulnerability Disclosure (CVD) policy.

## 2. Supported Versions

We support the **current MAJOR version** and the **immediate previous MAJOR version** for critical security fixes.

| Version | Supported | Notes |
| :--- | :--- | :--- |
| **v1.x** | ✅ Yes | **Active:** Critical fixes and feature updates. |
| **v0.x** | ❌ No | **End of Life:** No longer supported. |
| **< v1.0** | ❌ No | Deprecated. |

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
