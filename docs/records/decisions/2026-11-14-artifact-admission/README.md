# ADR-0007 decision-and-freeze package

Status: **documentation-only draft; design approval recorded; implementation blocked**.
Input: reviewed revision 07968a9a307415bf8a7aa6f04056caa22db01c78 (#789).
Assessed main: 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2. This package is stacked on
#789's branch, preserving its head and complete prior evidence. ADR-0007 was the
next available sequential number at the metadata refresh.

## Retained decision and exact contract

- [ADR-0007](../../../architecture/adr/ADR-0007-saved-program-admission-boundary.md)
  records scope, alternatives, consequences and prerequisites.
- [SESSION_APPROVAL.md](SESSION_APPROVAL.md) transcribes actual design authorization
  and the sole-CODEOWNER clarification, without fabricating a signed GitHub approval.
- [Exact container contract](../../../../spec/tisc/saved-program-container-v1.md):
  64-byte little-endian header, exact four-key ASCII manifest, digest coverage and
  framing/codec/identity/budget rejection order.
- execution-descriptor.json and identities.json finalize **draft identity**;
  [BUILD_ASSOCIATION.md](BUILD_ASSOCIATION.md) defines how an actual build must be
  separately approved. The template is deliberately null/unapproved; there are
  zero trusted runtime builds in this package.
- caps.json records the approved revision-2 values: 1 KiB manifest, 8 MiB payload,
  65,536 instructions/scalar pool values, 16,384 structural entries, 1 MiB metadata,
  rank 16, BigInt 256 B/token and 64 KiB aggregate, 32 MiB cumulative allocations.
  Unsupported/unqualified variants reject before construction. Caps do not bound
  VM execution or whole-process RSS.
- RFC-00D6's DM-09-only design disposition is recorded. **DM-01–DM-08 and formal
  checkboxes/signature are unchanged.** No semantic-profile expansion is approved.

## Break and witness artifacts, kept separate

[BREAK_REQUEST.md](BREAK_REQUEST.md), the exact draft spec,
[draft migration guide](../../../migration/saved-program-container-v1-draft.md),
registry planned entry, implementation-matrix planned row and
[DUAL_WITNESS_PLAN.md](DUAL_WITNESS_PLAN.md) prepare the required review surfaces.
The breaking-RFC intake, final exact-package review and major-version/lifecycle
execution remain outstanding. Draft breaking notes: strict unversioned-loader
rejection, explicit writer binding, eventual envelope compiler output and resource/
variant restrictions are incompatible defaults; raw13, ISA/numeric/VM semantics
remain unchanged. No version metadata or published changelog is altered.

Historical saved18/raw13 witnesses and all prior evidence remain intact. The old
whole-file hash is not updated. The new witness is unknown until actual approved
implementation runs; the framing vector is **not** that witness and not compiler
output. Existing required proof contexts cannot be skipped or silently repointed.
The registry/matrix say planned / Experimental / not implemented, not Verified.

## Review and outstanding prerequisites

reviewers.json records @t81dev as sole known CODEOWNER; a second eligible maintainer
is **unassigned**. Actual refreshed #789 reviews were empty. Both formal approval
slots remain unsatisfied. #789 is authored by @t81dev; if the package PR uses that
same author, the owner cannot approve their own PR as a required GitHub review.
[GitHub's review rules](https://docs.github.com/en/pull-requests/how-tos/review-pull-requests/reviewing-proposed-changes-in-a-pull-request)
confirm that limitation. Session design approval remains a separate record.
No AI role, duplicated owner approval or invented second reviewer satisfies it.

Before implementation: resolve eligible reviewer staffing; obtain actual reviews
of the exact package/implementation scope and scoped freeze-exception controls;
complete required breaking-RFC/spec/migration/version decisions. Before any output
switch: qualify budgeted variants/build association and approve the dual-witness
required-context transition with raw multi-platform evidence. These remain blocked.
No open freeze exception may be treated as release GO.

## Verification and authorship

Local validation checks canonical descriptor/caps digests, exact example header/
manifest/envelope identity, preserved historical files/hashes, DM-01–DM-08 identity,
JSON and local Markdown links. It is documentation validation, not runtime parser,
full CLI/VM, allocation or cross-platform verification. Exact prior source-loader
results retain their original limits. Codex authored this package and earlier
Windows/capture/#787 contributions; this is not independent certification.

No runtime changes, compiler-output switch, historical golden updates, tag,
release publication, merge or scientific-gate advancement. v2.0.0 recommended /
HOLD and #780's exclusion pending integration remain unchanged.
