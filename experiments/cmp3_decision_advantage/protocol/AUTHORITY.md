# EXP-T81-COMP3-01/r1 prospective authority

Original frozen protocol → approved r1 amendment → implementation targeting r1.

The original PR #635 commit is `b37005ceda6a3f4349ec9027b2a9be84ea176aa9`;
its protocol blob `0fbe0b4da1614867bf51b1c3eaf200f82d789a65` is preserved
byte-for-byte in [ORIGINAL_PR635_PROTOCOL.md](ORIGINAL_PR635_PROTOCOL.md).
The existing `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md` snapshot is also
unchanged; it is not silently substituted for the #635 original.

The maintainer explicitly approved M1–M4 and prospective r1 activation in the
Codex instruction titled “CODEX — EXP-T81-COMP3-01/r1 APPROVED IMPLEMENTATION
PUBLICATION”. That authorization binds all five exact hashes in
[ACTIVATION.json](ACTIVATION.json), permits integration/publication in existing
PR #637, and permits non-performance validation. It explicitly forbids full
experimental measurements, production signing provisioning, merging PRs, changes
to protections, and another PR. No experiment performance data was collected.

The immutable [r1 amendment](r1/PROPOSED_PROTOCOL_AMENDMENT.md) and four companion
files retain their approved bytes and historical draft filenames/status prose.
Their unsigned FINAL_APPROVAL_RECORD.md is preserved rather than edited in place.
The later maintainer authorization recorded here supersedes their preparation-time
“not approved” publication/activation status only; it changes no methodology.
The amended paired design, thresholds, PCG/draw order and T1–T8 requirements remain
exactly those approved. The historical provisional report/patch are retained as
pre-approval evidence, not the current authorization state.

This source-control record is not a cryptographically signed production approval,
runner delegation or eligibility attestation. No production keys/trust anchors
have been configured. Measurement requires the separately authenticated production
chain and intended-Linux-host validation specified by T1–T8, and separate
measurement authorization. All production collection/verification entry points
remain disabled. The implementation targets r1 but retains the documented Linux
and trusted-runner defects; prospective protocol activation does not certify full
implementation conformance or measurement eligibility.
