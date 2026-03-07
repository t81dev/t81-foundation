# Governance Model

> **Source of Truth:** This document defines the **decision-making process** for the project.

**Last Updated:** February 10, 2026

## 1. Overview

T81 Foundation follows a **Maintainer-Led Governance Model** (similar to BDFL-light).

- **Project Lead:** Has final say on architectural and strategic decisions.
- **Maintainers:** Manage day-to-day operations, review PRs, and guide contributors.
- **Contributors:** Submit code, documentation, and RFCs.

## 2. Decision Making

Most decisions are made through **consensus** on GitHub Issues and Pull Requests.

- **Minor Changes:** Merged by Maintainers after review.
- **Significant Changes:** require an **RFC (Request for Comments)** process.
- **Disagreements:** Resolved by the Project Lead if consensus cannot be reached.

## 3. RFC Process

Significant changes to the language, VM semantics, or governance must go through the RFC process:

1.  **Draft:** Create a PR in `spec/rfcs/` with the proposal.
2.  **Discuss:** Community and Maintainers provide feedback.
3.  **Approve/Reject:** Maintainers decide to accept or reject the RFC.
4.  **Implement:** Code changes are made based on the accepted RFC.

See `spec/README.md` for more details.

## 4. Code of Conduct

We enforce the [Contributor Covenant Code of Conduct](../../../CODE_OF_CONDUCT.md). All interactions must be respectful and constructive.
