# ADR-0007: Saved-program admission boundary

Status: **Proposed freeze package; maintainer design approval recorded; formal reviews pending**
Date: 2026-11-14
Version: 1.0.0 design record (not a runtime version)

## Purpose and context

Assign the next available sequential ADR number (0001–0006 present on refreshed
main) and retain the decision for reviewed proposal
07968a9a307415bf8a7aa6f04056caa22db01c78. Its predecessor
ADR-PROPOSED-artifact-admission-boundary.md remains unchanged as history.
Actual source-built loaders reinterpret old byte194 as TWMATMUL; raw13 is distinct
from saved18+metadata. Full CLI/VM execution was not reproduced in that work.

## Decision

The maintainer approved revision-2 design direction, resource caps and fail-closed
unsupported variants in this session. The [actual instruction record](../../records/decisions/2026-11-14-artifact-admission/SESSION_APPROVAL.md)
states its provenance and limits. It is not two GitHub freeze approvals.

1. Preserve raw13 public APIs/bytes, opcode assignments, numeric encodings and VM
   behavior. No translator, new trap identifiers, atomic-fault model, legacy VM or
   new instruction semantics is authorized.
2. Admit persisted executable programs only via explicitly bound, supported
   container/codec/implementation descriptor; ambiguous unversioned input rejects.
   Admission failures occur before VM construction/loading and are ordinary
   loader/CLI failures, not new VM traps.
3. Finalize the documentation contract in [saved-program-container-v1.md](../../../spec/tisc/saved-program-container-v1.md).
   Its exact bytes, descriptor and association terms are draft implementation
   requirements for this package's review; no runtime build is registered by it.
4. Accept revision-2 caps exactly, as recorded in the package caps.json. Budget
   checks must cover nested allocations; unsupported/unqualified variants reject
   before construction. This is not a measured RSS or execution-time guarantee.
5. Disposition **DM-09 only** as a scoped binding mechanism: a 64-byte envelope for
   full saved programs, unchanged raw13 helpers, no default foreign raw execution,
   and explicit conflict rejection. This is a bounded revision of options B/C,
   not the RFC's illustrative 16-byte header or tisc-v2.0 semantic profile.
6. Stage strict public file wrappers and explicit writer binding only after freeze
   controls and dual-witness transition are approved. No compiler-output switch
   now. Never overwrite a historical expected hash to make a new format pass.

## Alternatives considered

Opcode guessing/remapping, raw13 header insertion, a wrapper over unbounded nested
parsers, silently permissive public helper alternatives, and a broad portable
codec/legacy interpreter are rejected or deferred as recorded in revision 2.
An artifact's self-declared profile and checksum establish neither authority nor
producer truth. Host-approved build associations remain separate trust inputs.

## Consequences and prerequisites

The public default loader/writer behavior and whole-file compiler output would
break compatibility. A scoped breaking-change request, spec contract, migration
plan, registry/matrix documentation and draft breaking notes are included in the
[decision-and-freeze package](../../records/decisions/2026-11-14-artifact-admission/README.md).
Actual version metadata, previous-major lifecycle changes and new witnesses are
not performed. Runtime v2.0.0 remains recommended / HOLD, not published.

Formal freeze review needs **two reviewer approvals** under the ownership charter.
@t81dev is the sole identified CODEOWNER; the second eligible maintainer is
unassigned. Neither formal review slot is approved. No AI reviewer substitutes
for a maintainer. Exact package acceptance, signed RFC disposition as required,
freeze-exception controls, build qualification and gate integration remain pending.

## References and versioning

SPEC_AUTHORITY_MODEL, FREEZE_ENFORCEMENT, MODULE_OWNERSHIP_CHARTER,
APPROVAL_DELEGATION and RELEASE_DISCIPLINE remain binding. No policy is relaxed.
The RFC-00D6 partial record leaves DM-01–DM-08 unchanged and does not accept the
whole RFC. This documentation package itself changes no runtime version, VM,
raw encoding, historical golden, release state or scientific gate.
