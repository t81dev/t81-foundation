# Proposed maintainer decision record — not approved

Status: **PENDING — INVESTIGATION ONLY — NO GOVERNANCE CORRECTION**.
Baseline: `f4ffa85bf0bcaf028b51462113bf19c2eebb0091` (#578).
No signatures, approvals, exception grants or effective dates are asserted here.

## D1 — Authority for restoring the three active directories

Exact policy text, `docs/governance/SPEC_AUTHORITY_MODEL.md` §4.F:

> Adding new files to the repository root is **strictly prohibited** without unanimous maintainer approval.

This explicitly governs new root files. It does **not** explicitly state whether
directories, restoration of existing entries to a validator, or a new typed
contract are covered. Treating that silence as either authorization or an
unambiguous prohibition would overstate the text. §6 assigns subsystem ownership
but retains obsolete paths. Active CMake use is technical evidence, not a
retrospective grant of root authority. CODEOWNERS identifies `@t81dev` as an
owner; it does not prove unanimity, define the complete maintainer electorate,
or record approval of this contract.

Requested decision: record whether §4.F applies to directory permissions and
restoration of existing entries. If it does, identify the maintainers and obtain
their explicit approvals for `src/`, `experiments/`, and `development_tests/`.
If it does not, explicitly confirm that classifying those existing directories
as required structural inputs is an authorized corrective alignment, naming the
exact proposed contract digest. Either decision must distinguish root placement
from promotion of research code or approval of every descendant file.

Recommendation: approve the narrow structural correction only, preserving
research maturity and output-retention decisions separately. No filesystem
addition or scientific change is needed. This recommendation is **not active**.

## D2 — Obsolete manifests and inherited entries

Requested decision: approve removing `package.json` and `package-lock.json` from
required root files, citing explicit deletion in `cb54c175` and the actual search
workflow working directory. No placeholder manifests should be created.

Approve or amend the proposed inherited required files/directories individually;
their existing allowlist membership is an operational precedent, not proof of
independent normative approval. Specifically resolve whether existing generated
containers `artifacts/` and `logs/` remain required or receive explicit optional
or preservation-only treatment. They must not authorize arbitrary generated
files at root or expansion of evidence directories by implication.

## D3 — Non-destructive historical preservation

Requested decision: approve or reject each candidate exception listed in
`ENTRY_MATRIX.md`, using its baseline Git mode and object ID. Until then, every
candidate remains unresolved and the proposed activated grandfather list is
empty. Keeping bytes in the repository during investigation grants no validator
exception. A blanket “all existing entries are approved” decision is not the
recommended resolution.

For any approved temporary preservation exception, record the responsible owner,
reason, immutable baseline and expected content object, permitted operation
(preserve only), review trigger, and explicit scope. Do not permit new content,
replacement symlinks, renamed aliases, or automatic exception regeneration.
Review triggers must be deterministic, such as the next proposed change to the
entry or contract; wall-clock expiry must not make identical commits validate
differently. A Git subtree ID pins all descendant names, modes and bytes.

The exception may allow absence only if a separate authorized disposition has
removed it. It must not itself authorize deletion. The proposed first exception
form requires continued presence and exact bytes, so disposition is a separate
review. Preservation exceptions should be reported distinctly from ordinary
approved entries, and must never imply validated research claims.

## D4 — Generated artifacts and research witnesses

Requested decision: distinguish disposable outputs from intentional fixtures or
provenance evidence for `build.log`, the root `Makefile`, tracked executables
inside mixed directories, both root policies, `record_hash.txt`, the original
and exported TISC pair, and the test input/record. Preserve hashes and references
before considering a later disposition. Equal TISC bytes do not establish
redundancy: the original/export relationship may itself be evidence.

Recommendations and exact per-entry questions are in `ENTRY_MATRIX.md`. Nothing
in this record authorizes deletion, relocation, untracking or changes to these
entries. Existing research results and numerical exclusions remain intact.

## D5 — Protect the contract's own authority

A JSON `approved: true` field or a digest stored alongside mutable policy is not
authorization. Requested decision: define the trusted approval authority and
review mechanism for the contract **and validator implementation**, including
who can change its trust anchor. A proposed implementation should compare the
committed contract and checker against externally trusted digests/attestations
bound to the approved policy revision. The approved expected digests must come
from a protected base or independently controlled CI configuration, not from the
candidate branch. Updating a digest in the same unreviewed change must fail.

No protection change or approval service is introduced here. Existing protection
must stay intact. If an external authorization check is not available, report
that unauthorized policy editing cannot be proved absent by the validator
alone; do not claim a self-authorizing JSON contract solves it. Content-addressed
approval and human authority are distinct properties.

## Decision completion fields

For each decision, the reviewer must supply: decision ID, accept/reject/amend,
exact paths and revision digest, governing authority, maintainer identities and
required quorum, approval evidence URL/signature, allowed operations, and the
next review trigger. These fields are deliberately unfilled.

Implementation remains blocked pending D1–D5 as applicable. This record is the
requested concrete review artifact, not an invitation to bypass a red gate.
