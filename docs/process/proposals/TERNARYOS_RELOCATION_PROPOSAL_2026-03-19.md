# TernaryOS Relocation Proposal

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [TernaryOS Relocation Proposal](#ternaryos-relocation-proposal)
  - [Problem Statement](#problem-statement)
  - [Evidence Reviewed](#evidence-reviewed)
    - [Code / Build Wiring](#code--build-wiring)
    - [Normative / Governance Sources](#normative--governance-sources)
    - [Current Evidence / Runbooks](#current-evidence--runbooks)
    - [Local Verification Performed](#local-verification-performed)
  - [Findings](#findings)
    - [1. RFC-00B9 User Environment Is the Strongest Relocation Candidate](#1-rfc-00b9-user-environment-is-the-strongest-relocation-candidate)
    - [2. The Broader Kernel / Guest / Host Tooling Tree Is Not Ready To Move](#2-the-broader-kernel--guest--host-tooling-tree-is-not-ready-to-move)
    - [3. The Tree Is Structurally Mixed](#3-the-tree-is-structurally-mixed)
    - [4. Local Documentation Still Has Status Drift](#4-local-documentation-still-has-status-drift)
    - [5. Test Coverage Is Real but Uneven in Signal Quality](#5-test-coverage-is-real-but-uneven-in-signal-quality)
  - [Decision Summary](#decision-summary)
    - [Do Not Move the Whole `experimental/ternaryos/` Tree](#do-not-move-the-whole-`experimentalternaryos`-tree)
    - [Implemented Move: Narrow RFC-00B9 User Environment Slice](#implemented-move-narrow-rfc-00b9-user-environment-slice)
  - [Proposed Scope for a Narrow Extraction](#proposed-scope-for-a-narrow-extraction)
  - [Preconditions Before a Narrow Move](#preconditions-before-a-narrow-move)
  - [Rollout Plan](#rollout-plan)
    - [Phase 1: Status and Boundary Cleanup](#phase-1-status-and-boundary-cleanup)
    - [Phase 2: Narrow User Environment Extraction](#phase-2-narrow-user-environment-extraction)
    - [Phase 3: Follow-On Kernel Audit](#phase-3-follow-on-kernel-audit)
  - [Recommendation](#recommendation)

<!-- T81-TOC:END -->


**Date:** 2026-03-19  
**Status:** Implemented for the RFC-00B9 user-environment boundary; broader kernel-lane relocation still proposed.  
**Owner:** @t81dev

## Problem Statement

`experimental/ternaryos/` currently mixes several materially different surface
classes in one tree:

- hosted HAL and kernel-runtime implementation
- MMU, scheduler, IPC, and device-driver developer-lane proof surfaces
- DPE-backed kernel epoch tests
- an accepted RFC-00B9 user environment slice with passing acceptance tests
- guest-artifact packaging, VirtualBox/QEMU probes, and external-host runbooks

This creates two problems:

1. The directory layout suggests one uniform maturity class, even though the
   evidence shows a narrower promotion-ready subset.
2. Some local docs still describe the tree as purely experimental and
   "not governance-gated" even though the status/governance documents classify
   the user-environment slice as `Governed non-DCP`.

This proposal defined what should and should not move out of
`experimental/ternaryos/` at the current evidence level. The narrow
RFC-00B9 extraction has now been implemented as a stable boundary under
`include/t81/axion/` and `src/axion/`; the remaining proposal content still
applies to the broader kernel/guest lane.

## Evidence Reviewed

### Code / Build Wiring

- [CMakeLists.txt](/CMakeLists.txt)
- [experimental/ternaryos](/experimental/ternaryos)

### Normative / Governance Sources

- [RFC-00B9-ternary-os-user-environment.md](/spec/rfcs/RFC-00B9-ternary-os-user-environment.md)
- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
- [DETERMINISM_SURFACE_REGISTRY.md](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md)

### Current Evidence / Runbooks

- [RFC_00B9_USER_ENV_EVIDENCE_2026-03-18.md](/docs/records/status-history/RFC_00B9_USER_ENV_EVIDENCE_2026-03-18.md)
- [TERNARYOS_X86_64_BOOT_EVIDENCE_2026-03-16.md](/docs/records/audits/TERNARYOS_X86_64_BOOT_EVIDENCE_2026-03-16.md)
- [kernel_execution_plan.md](/experimental/ternaryos/docs/kernel_execution_plan.md)
- [virtualbox_x86_64_handoff.md](/experimental/ternaryos/docs/virtualbox_x86_64_handoff.md)

### Local Verification Performed

Built:

- `t81_ternaryos_hal_boot_test`
- `t81_ternaryos_mmu_test`
- `t81_ternaryos_scheduler_test`
- `t81_ternaryos_ipc_test`
- `t81_ternaryos_device_driver_test`
- `t81_ternaryos_shell_session_test`
- `t81_ternaryos_user_env_test`
- `t81_ternaryos_epoch_submission_test`
- `t81_ternaryos_epoch_audit_test`
- `t81_ternaryos_epoch_history_test`

Verified passing:

```sh
ctest --test-dir build -R 't81_ternaryos_(user_env_test|shell_session_test|hal_boot_test|mmu_test|scheduler_test|ipc_test|device_driver_test|epoch_submission_test|epoch_audit_test|epoch_history_test)' --output-on-failure
```

Observed result: `10/10` tests passed after the required targets were built.

## Findings

### 1. RFC-00B9 User Environment Is the Strongest Relocation Candidate

Evidence:

- [RFC-00B9-ternary-os-user-environment.md](/spec/rfcs/RFC-00B9-ternary-os-user-environment.md)
  is accepted.
- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
  classifies `TernaryOS User Environment` as `Accepted`, `Beta`,
  `Governed non-DCP`.
- [RFC_00B9_USER_ENV_EVIDENCE_2026-03-18.md](/docs/records/status-history/RFC_00B9_USER_ENV_EVIDENCE_2026-03-18.md)
  records passing acceptance coverage.
- Local verification reproduced the main userenv/shell tests successfully.

This slice is narrower and stronger than the rest of the tree:

- `experimental/ternaryos/userenv/`
- `experimental/ternaryos/shell/`

Judgment:

The RFC-00B9 user environment was the only credible near-term move-out
candidate inside `experimental/ternaryos/`, and that stable boundary
extraction is now complete.

### 2. The Broader Kernel / Guest / Host Tooling Tree Is Not Ready To Move

Evidence:

- [kernel_execution_plan.md](/experimental/ternaryos/docs/kernel_execution_plan.md)
  still frames the next major milestone as external `x86_64` VirtualBox host
  execution and evidence return.
- [virtualbox_x86_64_handoff.md](/experimental/ternaryos/docs/virtualbox_x86_64_handoff.md)
  is explicitly an external-host handoff runbook, not a closed in-repo
  implementation lane.
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
  still classifies `Axion OS` as a governed experimental kernel track.
- The tree contains substantial host-specific packaging/probe infrastructure and
  fixture bundles that do not belong in a stable public API boundary yet.

Judgment:

The full `experimental/ternaryos/` tree is not ready to move out wholesale.
The broader kernel/guest path still belongs to a governed experimental lane.

### 3. The Tree Is Structurally Mixed

The current layout combines:

- public-ish service/session/shell logic
- kernel ABI/runtime logic
- MMU/scheduler/IPC substrate
- guest-platform artifact generation
- host-validation scripts
- negative-fixture bundles

That is a sign the current directory is still serving as a working program
space, not a clean product boundary.

Judgment:

Any relocation should be a narrow extraction, not a folder rename.

### 4. Local Documentation Still Has Status Drift

[experimental/ternaryos/docs/README.md](/experimental/ternaryos/docs/README.md)
currently says:

- `Status: Experimental — non-DCP, not governance-gated.`

That is too weak relative to current authority:

- the user-environment slice is governed non-DCP
- the broader Axion OS lane is a governed experimental kernel track

Judgment:

That wording should be corrected immediately even if no code moves yet.

### 5. Test Coverage Is Real but Uneven in Signal Quality

The passing test slice is substantial and materially better than a prototype.
However, the scheduler suite still contains tautological checks in
[scheduler_test.cpp](/experimental/ternaryos/tests/scheduler_test.cpp)
that prove execution without strongly proving behavior:

- `check(switched || !switched, ...)`
- `check(!switched || switched, ...)`

Judgment:

The subsystem is not under-tested, but some tests should be strengthened before
using them as promotion-quality evidence.

## Decision Summary

### Do Not Move the Whole `experimental/ternaryos/` Tree

The broader Axion/TernaryOS kernel path is still a governed experimental lane.
It is too mixed with guest-artifact packaging, external-host validation, and
ongoing kernel-runtime work to treat as a relocation-ready stable boundary.

### Implemented Move: Narrow RFC-00B9 User Environment Slice

The extracted source slice was:

- `experimental/ternaryos/userenv/`
- `experimental/ternaryos/shell/`

Implemented stable destination:

- `include/t81/axion/userenv/`
- `include/t81/axion/shell/`
- `src/axion/userenv/`
- `src/axion/shell/`

The stable public headers and implementation entry points now live there, while
the matching headers under `experimental/ternaryos/` remain compatibility
shims for existing internal consumers.

## Proposed Scope for a Narrow Extraction

Move candidates:

- session manager types and behavior
- service registry types and parsing
- `t81-init` / user environment contract helpers
- `t81sh` shell/session logic

Do not move yet:

- `hal/`
- `kernel/`
- `mmu/`
- `sched/`
- `ipc/`
- `dev/`
- guest EFI build logic
- VirtualBox/QEMU packaging scripts
- host-validation fixtures and bundles

## Preconditions Before a Narrow Move

1. Replace weak scheduler tautology checks with behavior-specific assertions.
2. Split public userenv headers from kernel/private implementation headers.
3. Update docs/status pages so relocation does not imply DCP promotion.

## Rollout Plan

### Phase 1: Status and Boundary Cleanup

1. Correct local TernaryOS docs to match current governance language.
2. Add a small public-surface inventory for the RFC-00B9 slice.
3. Strengthen weak test assertions in the scheduler lane.

### Phase 2: Narrow User Environment Extraction

Status: implemented.

1. Create a stable userenv header path outside `experimental/`.
2. Move `userenv/` and `shell/` implementation files to matching stable source
   paths.
3. Update CMake libraries to point at the new locations.
4. Keep kernel, HAL, MMU, scheduler, IPC, and guest tooling under
   `experimental/`.

### Phase 3: Follow-On Kernel Audit

Only after the userenv extraction is complete should the broader Axion kernel
lane be reconsidered for relocation.

## Recommendation

At current evidence, the right move was and remains:

- **No** to moving all of `experimental/ternaryos/`
- **Yes** to the completed narrow move of the RFC-00B9 user environment slice
- **Yes** to immediately correcting status wording and strengthening weak tests

Current state:

- the RFC-00B9 user environment/shell boundary now exists under
  `include/t81/axion/` and `src/axion/`
- the broader Axion/TernaryOS kernel lane remains a governed experimental
  track
