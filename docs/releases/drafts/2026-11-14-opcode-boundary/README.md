# Artifact boundary proposal — current entry point

**Revision 2: proposed / not authorized for implementation.** This revision answers
maintainer requests for explicit decisions, trust boundaries, the minimum current
execution descriptor, defensible resource limits, and separated freeze/witness work.

Read:

- [Proposed decision record](../../../architecture/adr/ADR-PROPOSED-artifact-admission-boundary.md)
- [Trust boundaries](revision-2/TRUST_BOUNDARIES.md)
- [Descriptor scope](revision-2/DESCRIPTOR.md) and [candidate JSON](revision-2/CURRENT_EXECUTION_DESCRIPTOR.json)
- [Resource limits and rationale](revision-2/RESOURCE_CAPS.md)
- [Freeze exceptions and witnesses](revision-2/FREEZE_AND_WITNESS_CHANGES.md)

The proposed decision record and revision-2 documents supersede the **proposal**
sections of DESIGN.md, including its provisional 16 KiB/64 MiB caps and illustrative
capitalized admission-error wording. They do not supersede the reproduced loader
observations or expand the tested scope. Admission failures are pre-VM errors;
no new Trap enum member, code or VM fault identifier is proposed.

DESIGN.md, REPRODUCE.md, original evidence/ and SHA256SUMS are preserved byte for
byte as revision-1 history at 6db79cb8cc8f6fa0156fe4ead064ac33f00e7f3d. The original
manifest still validates. Revision-2 has separate source inspection and hashes.

Main assessed: 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2. #788's proposal remains
**v2.0.0 recommended / HOLD**, with #780 excluded pending integration. New header
inspection supplies opcode names/object sizes and fixture-size arithmetic only;
there is no new CLI/VM, cross-platform, memory-usage or performance run.
Codex authored both revisions and has prior implementation contributions; this is
not independent certification. No runtime, raw13 API, golden or historical bytes change.
