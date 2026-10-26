# RFC-0057: Root Layout Authority, Preservation Exceptions, and Contract Revision Approval

**Status:** draft
**Type:** standards-track
**Applies-To:** Root governance; SPEC_AUTHORITY_MODEL.md §4.F
**Created:** 2026-10-26
**Updated:** 2026-10-26
**Discussion:** https://github.com/t81dev/t81-foundation/pull/579#issuecomment-6419311331

---

## Summary

Propose a typed committed-tree root contract with exact preservation exceptions
and an independent revision-approval boundary. The sole active maintainer,
Project Lead @t81dev, authorized D1–D5 implementation preparation in the linked
comment, tied to proposal 71bb2c0e937dbcad73c9421e5465651b6e0d3ec3. This RFC remains
**draft**. Preparation approval is not RFC acceptance or policy activation.

## Motivation

The existing exact-name root gate requires deleted npm manifests and rejects
active source directories. PRs #578–#579 establish the bounded correction and
historical evidence; this RFC does not repeat or change scientific findings.

## Proposal

### Root authority and retrospective reconciliation

For the eventual accepted policy, §4.F approval applies to both root files and
directories, including changes reconciling historical permissions. The active
maintainer roster defines the electorate; the current documented and confirmed
roster contains @t81dev only. Required unanimous consent is recorded through an
authenticated decision tied to exact scope and artifact identities. CODEOWNERS
alone does not define the electorate. This draft does not amend §4.F in place.

The preparation contract has 31 required directories and 29 regular files. It
recognizes existing src/, experiments/ and development_tests/ as structural
inputs and removes obsolete root npm manifest requirements. artifacts/ and logs/
remain required containers, not permission to create arbitrary root outputs.
No new subsystem, descendant maturity or DCP guarantee follows from placement.

### Committed structural validation

Resolve one commit and tree, then read contract, schema, root modes and CMake
inputs exclusively from that tree. Require exact regular-file or tree modes;
reject symlinks, gitlinks, unknown entries, missing inputs, malformed data,
unsupported schemas and file/directory substitutions. Sort diagnostics.

Disable Git replacement objects and require the checkout's actual root. Read raw
root tree modes and ordering rather than ls-tree's normalized modes, and independently
verify the existence and actual kinds of referenced root objects. Pathspecs are literal
and tree reads are independent of the caller's cwd prefix. Reject non-standard
JSON constants as well as duplicate keys.

The implementation inventories literal reachable add_subdirectory edges and
literal target source paths, including semicolon lists, PROJECT_SOURCE_DIR,
CMAKE_SOURCE_DIR and CMAKE_CURRENT_SOURCE_DIR prefixes and optional branches.
Referenced root directories must be required, not merely preserved.
Computed subdirectory edges fail rather than being guessed. It is not a complete
CMake evaluator: include-loaded factories, indirect source variables and macro
invocations need additional approved coverage before broader drift guarantees.

Adversarial fixtures confirm that a local include, invoked macro, or indirect
source variable can reference a preserved root without detection by this bounded
inventory. Structural success must not be used as a complete CMake dependency
certificate. Review of an approval bound to a candidate commit must cover those
constructs; accepting this limited profile does not authorize their automatic
approval. Extending or rejecting such constructs is separate from implementing a
complete CMake interpreter and must be settled before broader claims/activation.

### Preservation-only exceptions

The contract records the exact 17 exceptions from the approved proposal, including
path, Git mode and object identity. Directories pin complete subtrees. Owner is
@t81dev; review is required before any proposed change to the entry or contract.
Continued presence and exact identity are mandatory. No replacement, new child,
automatic renewal, relocation, deletion, untracking or deduplication is permitted.
Equal original/exported TISC identities do not authorize collapsing the pair.
Preservation establishes neither scientific truth nor permanent placement.

### Independent contract and validator approval

Structural conformity and authorization are different results. An external
trusted evaluator must verify approval for exact contract, schema, validator,
legacy gate and workflow/action revisions, bound to an exact candidate commit.
Neither candidate-controlled metadata nor candidate-local hashes grant approval.

The working prototype consumes an explicit operator-provided JSON approval file
outside the candidate checkout and checks exact coverage/digests. Its fields are
version, purpose (scope-a-preparation-review), approved_commit, approver and
artifacts (path-to-SHA-256 map). The operator must independently authenticate and
protect that file, the evaluator, its invocation and result publication. Its
location and an approver string are **not** cryptographic authentication.
An attacker allowed to supply a forged external manifest or run a modified
candidate evaluator can manufacture apparent success; the prototype does not
prevent either action. Independence of the input, evaluator and invocation is
an integration prerequisite, not a property established by an external pathname.
The prototype has no signing keys, credentials, approval-service deployment or
required-check integration. It must never be treated as human approval merely
because a generated fixture passes it.

A production integration needs an approved independently controlled evaluator
and approval source, such as a verified signed manifest with its key configured
outside candidate control, or a trusted-base review service. Existing main has
required checks but the investigated protection does not enforce approving
reviews or administrator restrictions. No new protection is established here.

## Determinism / Safety Considerations

Git modes/objects and sorted diagnostics make structural results depend on the
selected committed tree rather than dirty files or clock time. Trust input is a
separate explicit input. Tests use synthetic operator approvals, not fabricated
maintainer stamps. Runtime, Axion policy enforcement, ISA, CanonFS witnesses and
scientific results remain unchanged. No universal determinism claim is added.

## Compatibility

The existing no-argument CI command continues the frozen pre-activation exact-set
check through check_root_structure_legacy.py. New --structural and --authorization
modes are standalone preparation tools. No required workflow is changed. A
successful structural result is explicitly not authorization or activation.

## Implementation Plan

Implement contract/schema, structural and authorization checks, committed mutation
tests and verification evidence in one draft descendant of #579. Then separately
review final artifact identities and independent enforcement. Do not merge or
activate this RFC through an implementation-preparation decision.

## Open Questions

- Which independently controlled approval source/evaluator will be deployed?
- Who controls its trusted reference and result publication, including bootstrap?
- What additional CMake computed/include constructs need future coverage?

## Acceptance Criteria

Acceptance requires maintainer approval through the governance process, explicit
resolution of independent enforcement and a separate activation decision tied
to final implementation identities. Tests must reject unauthorized policy/checker
revisions as well as root mutations. All preservation objects must match the
approved manifest. Run all existing regressions and the independent 27-command
inventory, distinguishing standalone results from unchanged required gates.
Scope B cleanup, protection changes, frozen semantics and scientific changes
remain outside this RFC's implementation preparation.
