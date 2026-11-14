# Scoped breaking-change request — ADR-0007, DM-09 only

Status: design direction accepted in session; exact package and freeze reviews pending.
This is the breaking-RFC intake artifact; no unassigned RFC number is fabricated.
Owner must decide whether the scoped RFC-00D6 DM-09 addendum plus this contract
satisfies RFC intake, or assign a separate breaking-RFC identifier before implementation.

Problem: current saved/raw readers accept valid but reassigned old opcode bytes
under new meanings. Missing producer/profile evidence cannot be reconstructed from
bytes, extension or runtime version. Wrapping alone does not bound nested parsing.

Unavoidable breaks proposed: strict persisted loading rejects unversioned files;
unbound save calls reject before output creation; new compiler output eventually
adds an explicit envelope; supported-variant/ABI/resource rules narrow admission.
Raw13 APIs, numeric payload encodings, opcode assignments and VM semantics remain
unchanged. No exception is requested for them; if implementation crosses them,
stop and prepare an independent decision.

Affected surfaces: public include/t81 file helpers, all inventoried CLI consumers,
compiler whole-file emission and canonical serialization/fixture proof lanes.
Prepared artifacts: ADR-0007; exact container spec; migration guide; registry and
implementation-matrix planned entries; caps/descriptor/build contract; dual-witness
plan; draft breaking notes and reviewer record. No entry is marked Verified.

Freeze controls: dedicated future implementation PR scope and freeze-exception
label, two eligible reviewer approvals, registry/matrix updates, approved spec and
migration/version plan. This docs package alone does not grant an exception.
@t81dev is the sole identified CODEOWNER; second reviewer is unassigned. No policy
change, duplicated approval or AI role resolves that missing reviewer.

Closure requires: reviewed exact implementation SHA and raw tests; preserved
historical byte/hash evidence; new separately named witness and applicable CI;
source/target build associations; all deviations dispositioned; actual reviewers'
records retained. Required context failures are not bypassed. Release requires no
open exception per RELEASE_DISCIPLINE.

Version/lifecycle work: runtime v2.0.0 remains recommended / HOLD. No metadata bump,
tag, release publication or previous-major deprecation is executed now. Before any
approved output switch/release, owner must approve exact SemVer/migration/changelog,
prior-major support disposition under VERSIONING.md, and perform the required
break procedure in the authorized implementation/release scope.
