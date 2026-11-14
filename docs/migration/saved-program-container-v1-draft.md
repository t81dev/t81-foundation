# Draft migration: explicit saved-program admission

Status: design approved in scope; implementation, dual reviews and qualification
pending. No migration tool or compiler switch ships with this document.

Affected contracts: unversioned saved .tisc artifacts, public file helpers and
whole-file compiler hashes. Low-level raw13 encode/decode remains unchanged and
is not a general Program serializer or admission guarantee.

1. Preserve original artifacts, source/build/policy records, signatures, capabilities
   and hashes. A checksum establishes identity, not producer truth.
2. Unknown/unversioned artifacts reject by default under the proposed strict loader.
   Filename, mtime, valid opcodes and successful legacy parse cannot identify a profile.
3. Prefer recompiling known source with a separately qualified approved compiler
   under the explicit descriptor/build association. Produce a new identity; review
   policy/signature/capability dependencies. No old hash/signature is inherited.
4. No opcode translator, raw reheadering, guessed current label, automatic wrapping,
   or old-profile interpreter is provided. Unknown/unsupported legacy data remains
   opaque archival/inspection input, not executable validation success.
5. Native consumers will require the explicit binding writer API. Unbound save
   fails before opening/replacing output; strict load does not retry unsafe formats.
6. Unsupported payload ABI, variants, profile or caps reject before construction.
   Larger external models remain separately governed inputs, not embedded payload.
7. Keep historical witnesses; generate new named container witness only from actual
   reviewed implementation runs. The dual-witness plan must precede an output switch.

No concrete executable CLI migration flags are documented as existing. No version
metadata or previous-major lifecycle is changed. Runtime v2.0.0 recommendation / HOLD
and DM-01–DM-08 remain as previously recorded. See ADR-0007 and its decision package.
